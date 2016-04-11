/*
 * Copyright (c) 2015-2016, Xerox Corporation (Xerox)and Palo Alto Research Center (PARC)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Patent rights are not granted under this agreement. Patent rights are
 *       available under FRAND terms.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL XEROX or PARC BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/**
 * @author Kevin Fox, Palo Alto Research Center (Xerox PARC)
 * @copyright 2015-2016, Xerox Corporation (Xerox)and Palo Alto Research Center (PARC).  All rights reserved.
 */
#include <config.h>

#include <LongBow/runtime.h>

#include <poll.h>
#include <fcntl.h>
#include <errno.h>
#include <netdb.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <net/ethernet.h>
#include <stdio.h>
#ifdef __linux__
#include <netinet/ether.h>
#endif

#include <parc/algol/parc_Network.h>
#include <parc/algol/parc_Deque.h>
#include <parc/algol/parc_HashCodeTable.h>
#include <parc/algol/parc_Hash.h>
#include <ccnx/forwarder/athena/athena_TransportLinkModule.h>
#include <ccnx/forwarder/athena/athena_TransportLinkModuleETH.h>
#include <ccnx/forwarder/athena/athena_EthernetFragmenter.h>
#include <ccnx/forwarder/athena/athena_Ethernet.h>

#include <ccnx/common/codec/ccnxCodec_TlvPacket.h>
#include <ccnx/common/codec/schema_v1/ccnxCodecSchemaV1_FixedHeader.h>

#define METIS_PACKET_TYPE_HOPFRAG 4
#define T_HOPFRAG_PAYLOAD  0x0005

typedef struct hopbyhop_header {
    uint8_t version;
    uint8_t packetType;
    uint16_t packetLength;
    uint8_t blob[3];
    uint8_t headerLength;
    uint16_t tlvType;
    uint16_t tlvLength;
} __attribute__((packed)) _HopByHopHeader;

/*
 * The B bit value in the top byte of the header
 */
#define BMASK 0x40

/*
 * The E bit value in the top byte of the header
 */
#define EMASK 0x20

/*
 * Sets the B flag in the header
 */
#define _hopByHopHeader_SetBFlag(header) ((header)->blob[0] |= BMASK)

/*
 * Sets the E flag in the header
 */
#define _hopByHopHeader_SetEFlag(header) ((header)->blob[0] |= EMASK)

bool
_ETH1990_IsFragment(PARCBuffer *wireFormatBuffer)
{
    bool result = false;
    _HopByHopHeader *header = parcBuffer_Overlay(wireFormatBuffer, 0);
    if (header->packetType == METIS_PACKET_TYPE_HOPFRAG) {
        result = true;
    }
    return result;
}

static void
_hopByHopHeader_SetPayloadLength(_HopByHopHeader *header, size_t payloadLength)
{
    const uint16_t packetLength = sizeof(_HopByHopHeader) + payloadLength;

    header->version = CCNxTlvDictionary_SchemaVersion_V1;
    header->packetType = METIS_PACKET_TYPE_HOPFRAG;
    header->packetLength = htons(packetLength);
    header->headerLength = sizeof(CCNxCodecSchemaV1FixedHeader);
    header->tlvType = htons(T_HOPFRAG_PAYLOAD);
    header->tlvLength = htons(payloadLength);
}

static uint32_t
_hopByHopHeader_GetSeqnum(const _HopByHopHeader *header)
{
    uint32_t seqnum = ((uint32_t) header->blob[0] & 0x0F) << 16 | (uint32_t) header->blob[1] << 8 | header->blob[2];
    return seqnum;
}

static void
_hopByHopHeader_SetSequenceNumber(_HopByHopHeader *header, uint32_t seqnum)
{
    header->blob[2] = seqnum & 0xFF;
    header->blob[1] = (seqnum >> 8) & 0xFF;

    header->blob[0] &= 0xF0;
    header->blob[0] |= (seqnum >> 16) & 0x0F;
}

/*
 * non-zero if the B flag is set
 */
#define _hopByHopHeader_GetBFlag(header) ((header)->blob[0] & BMASK)

/*
 * non-zero if the E flag is set
 */
#define _hopByHopHeader_GetEFlag(header) ((header)->blob[0] & EMASK)

PARCBuffer *
_ETH1990_ReceiveAndReassemble(AthenaEthernetFragmenter *athenaEthernetFragmenter, PARCBuffer *wireFormatBuffer)
{
    assertTrue(wireFormatBuffer != NULL, "Fragmenter reassembly called with a null buffer");

    // If it's not our fragment, send it back
    if (!_ETH1990_IsFragment(wireFormatBuffer)) {
        return wireFormatBuffer;
    }

    // Verify the type, and move the buffer beyond the header.
    _HopByHopHeader *header = parcBuffer_Overlay(wireFormatBuffer, sizeof(_HopByHopHeader));
    assertTrue(header->packetType == METIS_PACKET_TYPE_HOPFRAG, "ETH1990 Unknown fragment type (%d)", header->packetType);

    // If it's not a sequence number we were expecting, clean everything out and start over.
    uint32_t seqnum = _hopByHopHeader_GetSeqnum(header);
    if (seqnum != athenaEthernetFragmenter->receiveSequenceNumber) {
        parcBuffer_Release(&wireFormatBuffer);
        while (parcDeque_Size(athenaEthernetFragmenter->fragments) > 0) {
            wireFormatBuffer = parcDeque_RemoveFirst(athenaEthernetFragmenter->fragments);
            parcBuffer_Release(&wireFormatBuffer);
        }
        athenaEthernetFragmenter->receiveSequenceNumber = 0;
        return NULL;
    }

    assertTrue(parcDeque_Size(athenaEthernetFragmenter->fragments) == seqnum, "Queue size, sequence number mis-match");

    // Gather buffers until we receive an end frame
    parcDeque_Append(athenaEthernetFragmenter->fragments, wireFormatBuffer);

    if (_hopByHopHeader_GetBFlag(header)) {
        athenaEthernetFragmenter->receiveSequenceNumber++;
    } 
    if (_hopByHopHeader_GetEFlag(header)) {
        PARCBuffer *reassembledBuffer = parcBuffer_Allocate(0);
        while (parcDeque_Size(athenaEthernetFragmenter->fragments) > 0) {
            wireFormatBuffer = parcDeque_RemoveFirst(athenaEthernetFragmenter->fragments);
            const uint8_t *array = parcBuffer_Overlay(wireFormatBuffer, 0);
            size_t arrayLength = parcBuffer_Remaining(wireFormatBuffer);
            parcBuffer_Resize(reassembledBuffer, parcBuffer_Capacity(reassembledBuffer) + arrayLength);
            parcBuffer_PutArray(reassembledBuffer, arrayLength, array);
            parcBuffer_Release(&wireFormatBuffer);
        }
        athenaEthernetFragmenter->receiveSequenceNumber = 0;
        return reassembledBuffer;
    } else if (!_hopByHopHeader_GetBFlag(header)) {
        while (parcDeque_Size(athenaEthernetFragmenter->fragments) > 0) {
            wireFormatBuffer = parcDeque_RemoveFirst(athenaEthernetFragmenter->fragments);
            parcBuffer_Release(&wireFormatBuffer);
        }
        athenaEthernetFragmenter->receiveSequenceNumber = 0;
    }

    return NULL;
}

static int
_ETH1990_FragmentAndSend(AthenaEthernetFragmenter *athenaEthernetFragmenter,
                         AthenaEthernet *athenaEthernet,
                         size_t mtu, struct ether_header *etherHeader,
                         CCNxMetaMessage *ccnxMetaMessage)
{
    _HopByHopHeader fragmentHeader = {0};
    const size_t maxPayload = mtu - sizeof(_HopByHopHeader);
    _hopByHopHeader_SetBFlag(&fragmentHeader);

    PARCBuffer *wireFormatBuffer = athenaTransportLinkModule_GetMessageBuffer(ccnxMetaMessage);
    size_t length = parcBuffer_Remaining(wireFormatBuffer);
    size_t offset = 0;
    athenaEthernetFragmenter->sendSequenceNumber = 0;

    while (offset < length) {
        struct iovec iov[3];
        int iovcnt = 3;

        size_t payloadLength = maxPayload;
        const size_t remaining = length - offset;

        _hopByHopHeader_SetSequenceNumber(&fragmentHeader, athenaEthernetFragmenter->sendSequenceNumber++);

        if (remaining < maxPayload) {
            payloadLength = remaining;
            _hopByHopHeader_SetEFlag(&fragmentHeader);
        }

        _hopByHopHeader_SetPayloadLength(&fragmentHeader, payloadLength);

        iov[0].iov_base = &etherHeader;
        iov[0].iov_len = sizeof(etherHeader);
        iov[1].iov_base = &fragmentHeader;
        iov[1].iov_len = sizeof(_HopByHopHeader);
        iov[2].iov_base = parcBuffer_Overlay(wireFormatBuffer, payloadLength);
        iov[2].iov_len = payloadLength;

        ssize_t writeCount = athenaEthernet_Send(athenaEthernet, iov, iovcnt);

        if (writeCount == -1) {
            parcBuffer_Release(&wireFormatBuffer);
            errno = EIO;
            return -1;
        }

        offset += payloadLength;
    }

    parcBuffer_Release(&wireFormatBuffer);

    return 0;
}

static void
_athenaEthernetFragmenter_1990_Fini(AthenaEthernetFragmenter *athenaEthernetFragmenter)
{
    while (parcDeque_Size(athenaEthernetFragmenter->fragments) > 0) {
        PARCBuffer *wireFormatBuffer = parcDeque_RemoveFirst(athenaEthernetFragmenter->fragments);
        parcBuffer_Release(&wireFormatBuffer);
    }
    parcDeque_Release(&(athenaEthernetFragmenter->fragments));
}

AthenaEthernetFragmenter *
athenaEthernetFragmenter_1990_Init(AthenaEthernetFragmenter *athenaEthernetFragmenter)
{
    athenaEthernetFragmenter->fragments = parcDeque_Create();
    athenaEthernetFragmenter->send = (AthenaEthernetFragmenter_Send *)_ETH1990_FragmentAndSend;
    athenaEthernetFragmenter->receive = (AthenaEthernetFragmenter_Receive *)_ETH1990_ReceiveAndReassemble;
    athenaEthernetFragmenter->fini = (AthenaEthernetFragmenter_Fini *)_athenaEthernetFragmenter_1990_Fini;
    return athenaEthernetFragmenter;
}
