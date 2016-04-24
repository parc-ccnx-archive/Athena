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
#include <ccnx/forwarder/athena/athena_Fragmenter.h>
#include <ccnx/forwarder/athena/athena_TransportLinkModuleFragmenter_BEFS.h>

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
    size_t reassembledSize;
    bool idle;
} _BEFS_fragmenterData;

_BEFS_fragmenterData *
_BEFS_CreateFragmenterData()
{
    _BEFS_fragmenterData *fragmenterData = parcMemory_AllocateAndClear(sizeof(_BEFS_fragmenterData));
    fragmenterData->fragments = parcDeque_Create();
    fragmenterData->idle = true;
    return fragmenterData;
}

_BEFS_fragmenterData *
_BEFS_GetFragmenterData(AthenaFragmenter *athenaFragmenter)
{
    return (_BEFS_fragmenterData*)athenaFragmenter->fragmenterData;
}

void
_BEFS_ClearFragmenterData(AthenaFragmenter *athenaFragmenter)
{
    _BEFS_fragmenterData *fragmenterData = _BEFS_GetFragmenterData(athenaFragmenter);
    while (parcDeque_Size(fragmenterData->fragments) > 0) {
        PARCBuffer *wireFormatBuffer = parcDeque_RemoveFirst(fragmenterData->fragments);
        parcBuffer_Release(&wireFormatBuffer);
    }
    fragmenterData->idle = true;
    fragmenterData->reassembledSize = 0;
}

void
_BEFS_DestroyFragmenterData(AthenaFragmenter *athenaFragmenter)
{
    _BEFS_fragmenterData *fragmenterData = _BEFS_GetFragmenterData(athenaFragmenter);
    _BEFS_ClearFragmenterData(athenaFragmenter);
    parcDeque_Release(&(fragmenterData->fragments));
    parcMemory_Deallocate(&fragmenterData);
}

bool
_BEFS_IsFragment(PARCBuffer *wireFormatBuffer)
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
static PARCBuffer *
_BEFS_ReceiveAndReassemble(AthenaFragmenter *athenaFragmenter, PARCBuffer *wireFormatBuffer)
{
    // If it's not a fragment type we recognize, send it back for others to process
    if (!_BEFS_IsFragment(wireFormatBuffer)) {
        return wireFormatBuffer;
    }

    _BEFS_fragmenterData *fragmenterData = _BEFS_GetFragmenterData(athenaFragmenter);
    assertTrue(wireFormatBuffer != NULL, "Fragmenter reassembly called with a null buffer");

    // Verify the type, and move the buffer beyond the header.
    _HopByHopHeader *header = parcBuffer_Overlay(wireFormatBuffer, sizeof(_HopByHopHeader));
    assertTrue(header->packetType == METIS_PACKET_TYPE_HOPFRAG, "BEFS Unknown fragment type (%d)", header->packetType);
    uint32_t seqnum = _hopByHopHeader_GetSeqnum(header);

    if (fragmenterData->idle) {
        if (_hopByHopHeader_GetBFlag(header)) {
            _BEFS_ClearFragmenterData(athenaFragmenter);
            fragmenterData->idle = false;
        }
    } else {
        // If it's not a sequence number we were expecting, clean everything out and start over.
        if (seqnum != fragmenterData->receiveSequenceNumber) {
            parcBuffer_Release(&wireFormatBuffer);
            _BEFS_ClearFragmenterData(athenaFragmenter);
            return NULL;
        }
    }

    // Gather buffers until we receive an end frame
    parcDeque_Append(fragmenterData->fragments, wireFormatBuffer);
    fragmenterData->reassembledSize += parcBuffer_Remaining(wireFormatBuffer);
    fragmenterData->receiveSequenceNumber = seqnum + 1; // next to expect

    if (_hopByHopHeader_GetEFlag(header)) {
        PARCBuffer *reassembledBuffer = parcBuffer_Allocate(fragmenterData->reassembledSize);
        // Currently we cannot decode from an IO vector, so must copy into a buffer (See BugzID: 903)
        while (parcDeque_Size(fragmenterData->fragments) > 0) {
            wireFormatBuffer = parcDeque_RemoveFirst(fragmenterData->fragments);
            const uint8_t *array = parcBuffer_Overlay(wireFormatBuffer, 0);
            size_t arrayLength = parcBuffer_Remaining(wireFormatBuffer);
            parcBuffer_PutArray(reassembledBuffer, arrayLength, array);
            parcBuffer_Release(&wireFormatBuffer);
        }
        _BEFS_ClearFragmenterData(athenaFragmenter);
        return reassembledBuffer;
    }

    // If it's an Idle frame, make sure we're clear and ready.
    if (_hopByHopHeader_GetIFlag(header)) {
        _BEFS_ClearFragmenterData(athenaFragmenter);
    }

    return NULL;
}

static CCNxCodecEncodingBufferIOVec *
_BEFS_CreateFragment(AthenaFragmenter *athenaFragmenter, PARCBuffer *message, size_t mtu, int fragmentNumber)
{
    CCNxCodecEncodingBufferIOVec *fragmentIoVec = NULL;
    _BEFS_fragmenterData *fragmenterData = _BEFS_GetFragmenterData(athenaFragmenter);

    const size_t maxPayload = mtu - sizeof(_HopByHopHeader);
    size_t payloadLength = maxPayload;

    size_t length = parcBuffer_Remaining(message);
    size_t offset = maxPayload * fragmentNumber;
    ssize_t remaining = length - offset;

    if (remaining <= 0) {
        return NULL;
    }

    // Create fragmentation header
    PARCBuffer *fragmentHeaderBuffer = parcBuffer_Allocate(sizeof(_HopByHopHeader));
    _HopByHopHeader *fragmentHeader = parcBuffer_Overlay(fragmentHeaderBuffer, 0);

    if (fragmentNumber == 0) {
        _hopByHopHeader_SetBFlag(fragmentHeader);
    }

    _hopByHopHeader_SetSequenceNumber(fragmentHeader, fragmenterData->sendSequenceNumber++);

    if (remaining < maxPayload) {
        payloadLength = remaining;
        _hopByHopHeader_SetEFlag(fragmentHeader);
    }

    _hopByHopHeader_SetPayloadLength(fragmentHeader, payloadLength);

    // Create slice of buffer to send
    CCNxCodecEncodingBuffer *encodingBuffer = ccnxCodecEncodingBuffer_Create();
    ccnxCodecEncodingBuffer_AppendBuffer(encodingBuffer, message);

    CCNxCodecEncodingBuffer *encodingBufferSlice = NULL;
    // If NULL there's no fragment that matches the offset/length we asked for
    encodingBufferSlice = ccnxCodecEncodingBuffer_Slice(encodingBuffer, offset, maxPayload);
    ccnxCodecEncodingBuffer_Release(&encodingBuffer);

    if (encodingBufferSlice) {
        // Prepend our hop by hop header to the slice
        ccnxCodecEncodingBuffer_PrependBuffer(encodingBufferSlice, fragmentHeaderBuffer);

        fragmentIoVec = ccnxCodecEncodingBuffer_CreateIOVec(encodingBufferSlice);
        ccnxCodecEncodingBuffer_Release(&encodingBufferSlice);
    }

    parcBuffer_Release(&fragmentHeaderBuffer);
    return fragmentIoVec;
}


static void
_athenaFragmenter_BEFS_Fini(AthenaFragmenter *athenaFragmenter)
{
    _BEFS_DestroyFragmenterData(athenaFragmenter);
}

AthenaFragmenter *
athenaFragmenter_BEFS_Init(AthenaFragmenter *athenaFragmenter)
{
    athenaFragmenter->fragmenterData = _BEFS_CreateFragmenterData();
    athenaFragmenter->createFragment = (AthenaFragmenter_CreateFragment *)_BEFS_CreateFragment;
    athenaFragmenter->receiveFragment = (AthenaFragmenter_ReceiveFragment *)_BEFS_ReceiveAndReassemble;
    athenaFragmenter->fini = (AthenaFragmenter_Fini *)_athenaFragmenter_BEFS_Fini;
    return athenaFragmenter;
}
