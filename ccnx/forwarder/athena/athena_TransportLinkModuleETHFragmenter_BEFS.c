/*
 * Copyright (c) 2016, Xerox Corporation (Xerox)and Palo Alto Research Center (PARC)
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
 * @copyright 2016, Xerox Corporation (Xerox)and Palo Alto Research Center (PARC).  All rights reserved.
 */
#include <config.h>

#include <LongBow/runtime.h>

#include <errno.h>
#include <net/ethernet.h>

#include <parc/algol/parc_Deque.h>
#include <ccnx/forwarder/athena/athena_TransportLinkModule.h>
#include <ccnx/forwarder/athena/athena_EthernetFragmenter.h>
#include <ccnx/forwarder/athena/athena_TransportLinkModuleETHFragmenter_BEFS.h>

#include <ccnx/common/codec/schema_v1/ccnxCodecSchemaV1_FixedHeader.h>

/*
 * Fragmenter module supporting the point to point fragmentation described in:
 *
 *    ICN "Begin-End" Hop by Hop Fragmentation (draft-mosko-icnrg-beginendfragment-00)
 */

/*
 * Private data used to sequence and reassemble fragments.
 */
typedef struct _BEFS_fragmenterData {
    uint32_t sendSequenceNumber;
    uint32_t receiveSequenceNumber;
    PARCDeque *fragments;
} _BEFS_fragmenterData;

_BEFS_fragmenterData *
_ETH_BEFS_CreateFragmenterData()
{
    _BEFS_fragmenterData *fragmenterData = parcMemory_Allocate(sizeof(_BEFS_fragmenterData));
    fragmenterData->fragments = parcDeque_Create();
    return fragmenterData;
}

_BEFS_fragmenterData *
_ETH_BEFS_GetFragmenterData(AthenaEthernetFragmenter *athenaEthernetFragmenter)
{
    return (_BEFS_fragmenterData*)athenaEthernetFragmenter->fragmenterData;
}

void
_ETH_BEFS_DestroyFragmenterData(AthenaEthernetFragmenter *athenaEthernetFragmenter)
{
    _BEFS_fragmenterData *fragmenterData = _ETH_BEFS_GetFragmenterData(athenaEthernetFragmenter);
    while (parcDeque_Size(fragmenterData->fragments) > 0) {
        PARCBuffer *wireFormatBuffer = parcDeque_RemoveFirst(fragmenterData->fragments);
        parcBuffer_Release(&wireFormatBuffer);
    }
    parcDeque_Release(&(fragmenterData->fragments));
    parcMemory_Deallocate(&fragmenterData);
}

bool
_ETH_BEFS_IsFragment(PARCBuffer *wireFormatBuffer)
{
    bool result = false;
    _HopByHopHeader *header = parcBuffer_Overlay(wireFormatBuffer, 0);
    if (header->packetType == METIS_PACKET_TYPE_HOPFRAG) {
        result = true;
    }
    return result;
}

/*
 * Methods for setting up and reading information from the hop by hop header
 */
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
 * Determine if it's a packet that we own, and if so, perform the fragmentation protocol, otherwise
 * send the packet back so that it can be handled by someone else.  If we return null, we've retained
 * the message along with it's ownership.
 */
PARCBuffer *
_ETH_BEFS_ReceiveAndReassemble(AthenaEthernetFragmenter *athenaEthernetFragmenter, PARCBuffer *wireFormatBuffer)
{
    // If it's not a fragment type we recognize, send it back for others to process
    if (!_ETH_BEFS_IsFragment(wireFormatBuffer)) {
        return wireFormatBuffer;
    }

    _BEFS_fragmenterData *fragmenterData = _ETH_BEFS_GetFragmenterData(athenaEthernetFragmenter);
    assertTrue(wireFormatBuffer != NULL, "Fragmenter reassembly called with a null buffer");

    // Verify the type, and move the buffer beyond the header.
    _HopByHopHeader *header = parcBuffer_Overlay(wireFormatBuffer, sizeof(_HopByHopHeader));
    assertTrue(header->packetType == METIS_PACKET_TYPE_HOPFRAG, "ETH_BEFS Unknown fragment type (%d)", header->packetType);

    // If it's not a sequence number we were expecting, clean everything out and start over.
    uint32_t seqnum = _hopByHopHeader_GetSeqnum(header);
    if (seqnum != fragmenterData->receiveSequenceNumber) {
        parcBuffer_Release(&wireFormatBuffer);
        while (parcDeque_Size(fragmenterData->fragments) > 0) {
            wireFormatBuffer = parcDeque_RemoveFirst(fragmenterData->fragments);
            parcBuffer_Release(&wireFormatBuffer);
        }
        fragmenterData->receiveSequenceNumber = 0;
        return NULL;
    }

    assertTrue(parcDeque_Size(fragmenterData->fragments) == seqnum, "Queue size, sequence number mis-match");

    // Gather buffers until we receive an end frame
    parcDeque_Append(fragmenterData->fragments, wireFormatBuffer);

    if (_hopByHopHeader_GetBFlag(header)) {
        fragmenterData->receiveSequenceNumber++;
    } 
    if (_hopByHopHeader_GetEFlag(header)) {
        PARCBuffer *reassembledBuffer = parcBuffer_Allocate(0);
        while (parcDeque_Size(fragmenterData->fragments) > 0) {
            wireFormatBuffer = parcDeque_RemoveFirst(fragmenterData->fragments);
            const uint8_t *array = parcBuffer_Overlay(wireFormatBuffer, 0);
            size_t arrayLength = parcBuffer_Remaining(wireFormatBuffer);
            parcBuffer_Resize(reassembledBuffer, parcBuffer_Capacity(reassembledBuffer) + arrayLength);
            parcBuffer_PutArray(reassembledBuffer, arrayLength, array);
            parcBuffer_Release(&wireFormatBuffer);
        }
        fragmenterData->receiveSequenceNumber = 0;
        return reassembledBuffer;
    } else if (!_hopByHopHeader_GetBFlag(header)) {
        while (parcDeque_Size(fragmenterData->fragments) > 0) {
            wireFormatBuffer = parcDeque_RemoveFirst(fragmenterData->fragments);
            parcBuffer_Release(&wireFormatBuffer);
        }
        fragmenterData->receiveSequenceNumber = 0;
    }

    return NULL;
}

static int
_ETH_BEFS_FragmentAndSend(AthenaEthernetFragmenter *athenaEthernetFragmenter,
                         AthenaEthernet *athenaEthernet,
                         size_t mtu, struct ether_header *etherHeader,
                         CCNxMetaMessage *ccnxMetaMessage)
{
    _BEFS_fragmenterData *fragmenterData = _ETH_BEFS_GetFragmenterData(athenaEthernetFragmenter);
    _HopByHopHeader fragmentHeader = {0};
    const size_t maxPayload = mtu - sizeof(_HopByHopHeader);
    _hopByHopHeader_SetBFlag(&fragmentHeader);

    PARCBuffer *wireFormatBuffer = athenaTransportLinkModule_GetMessageBuffer(ccnxMetaMessage);
    size_t length = parcBuffer_Remaining(wireFormatBuffer);
    size_t offset = 0;
    fragmenterData->sendSequenceNumber = 0;

    while (offset < length) {
        struct iovec iov[3];
        int iovcnt = 3;

        size_t payloadLength = maxPayload;
        const size_t remaining = length - offset;

        _hopByHopHeader_SetSequenceNumber(&fragmentHeader, fragmenterData->sendSequenceNumber++);

        if (remaining < maxPayload) {
            payloadLength = remaining;
            _hopByHopHeader_SetEFlag(&fragmentHeader);
        }

        _hopByHopHeader_SetPayloadLength(&fragmentHeader, payloadLength);

        iov[0].iov_len = sizeof(struct ether_header);
        iov[0].iov_base = etherHeader;
        iov[1].iov_len = sizeof(_HopByHopHeader);
        iov[1].iov_base = &fragmentHeader;
        iov[2].iov_len = payloadLength;
        iov[2].iov_base = parcBuffer_Overlay(wireFormatBuffer, payloadLength);

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
_athenaEthernetFragmenter_BEFS_Fini(AthenaEthernetFragmenter *athenaEthernetFragmenter)
{
    _ETH_BEFS_DestroyFragmenterData(athenaEthernetFragmenter);
}

AthenaEthernetFragmenter *
athenaEthernetFragmenter_BEFS_Init(AthenaEthernetFragmenter *athenaEthernetFragmenter)
{
    athenaEthernetFragmenter->fragmenterData = _ETH_BEFS_CreateFragmenterData();
    athenaEthernetFragmenter->send = (AthenaEthernetFragmenter_Send *)_ETH_BEFS_FragmentAndSend;
    athenaEthernetFragmenter->receive = (AthenaEthernetFragmenter_Receive *)_ETH_BEFS_ReceiveAndReassemble;
    athenaEthernetFragmenter->fini = (AthenaEthernetFragmenter_Fini *)_athenaEthernetFragmenter_BEFS_Fini;
    return athenaEthernetFragmenter;
}
