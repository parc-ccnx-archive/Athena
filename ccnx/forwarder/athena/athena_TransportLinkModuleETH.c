/*
 * Copyright (c) 2015-2016, Xerox Corporation (Xerox) and Palo Alto Research Center, Inc (PARC)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * * Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL XEROX OR PARC BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * ################################################################################
 * #
 * # PATENT NOTICE
 * #
 * # This software is distributed under the BSD 2-clause License (see LICENSE
 * # file).  This BSD License does not make any patent claims and as such, does
 * # not act as a patent grant.  The purpose of this section is for each contributor
 * # to define their intentions with respect to intellectual property.
 * #
 * # Each contributor to this source code is encouraged to state their patent
 * # claims and licensing mechanisms for any contributions made. At the end of
 * # this section contributors may each make their own statements.  Contributor's
 * # claims and grants only apply to the pieces (source code, programs, text,
 * # media, etc) that they have contributed directly to this software.
 * #
 * # There is no guarantee that this section is complete, up to date or accurate. It
 * # is up to the contributors to maintain their portion of this section and up to
 * # the user of the software to verify any claims herein.
 * #
 * # Do not remove this header notification.  The contents of this section must be
 * # present in all distributions of the software.  You may only modify your own
 * # intellectual property statements.  Please provide contact information.
 *
 * - Palo Alto Research Center, Inc
 * This software distribution does not grant any rights to patents owned by Palo
 * Alto Research Center, Inc (PARC). Rights to these patents are available via
 * various mechanisms. As of January 2016 PARC has committed to FRAND licensing any
 * intellectual property used by its contributions to this software. You may
 * contact PARC at cipo@parc.com for more information or visit http://www.ccnx.org
 */
/**
 * @author Kevin Fox, Palo Alto Research Center (Xerox PARC)
 * @copyright (c) 2015-2016, Xerox Corporation (Xerox) and Palo Alto Research Center, Inc (PARC).  All rights reserved.
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
#include <ccnx/forwarder/athena/athena_Ethernet.h>
#include <ccnx/forwarder/athena/athena_Fragmenter.h>

#include <ccnx/common/codec/ccnxCodec_TlvPacket.h>

//
// Private data for each link connection
//
typedef struct _connectionPair {
    struct ether_addr myAddress;
    socklen_t myAddressLength;
    struct ether_addr peerAddress;
    socklen_t peerAddressLength;
    size_t mtu;
} _connectionPair;

//
// Private data for each link instance
//
typedef struct _ETHLinkData {
    _connectionPair link;
    PARCDeque *queue;
    PARCHashCodeTable *multiplexTable;
    struct {
        size_t receive_DecodeFailed;
        size_t receive_NoLinkDestination;
        size_t receive_PeerNotConfigured;
        size_t send_Error;
        size_t send_Retry;
        size_t send_ShortWrite;
    } _stats;
    AthenaEthernet *athenaEthernet;
    AthenaFragmenter *fragmenter;
} _ETHLinkData;

static int
_stringToEtherAddr(struct ether_addr *address, const char *buffer)
{
    int count = sscanf(buffer, "%hhx%*[:-]%hhx%*[:-]%hhx%*[:-]%hhx%*[:-]%hhx%*[:-]%hhx",
                       &(address->ether_addr_octet[0]), &(address->ether_addr_octet[1]),
                       &(address->ether_addr_octet[2]), &(address->ether_addr_octet[3]),
                       &(address->ether_addr_octet[4]), &(address->ether_addr_octet[5]));
    if (count != 6) {
        if (ether_hostton(buffer, address) != 0) {
            errno = EINVAL;
            return -1;
        }
    }
    return 0;
}

static void
_addressToString(const u_int8_t *address, char *buffer)
{
    if (ether_ntohost(buffer, (struct ether_addr *) address) == -1) {
        sprintf(buffer, "%2.2hhx:%2.2hhx:%2.2hhx:%2.2hhx:%2.2hhx:%2.2hhx",
                address[0], address[1], address[2],
                address[3], address[4], address[5]);
    }
}

static _ETHLinkData *
_ETHLinkData_Create()
{
    _ETHLinkData *linkData = parcMemory_AllocateAndClear(sizeof(_ETHLinkData));
    assertNotNull(linkData, "Could not create private data for new link");
    return linkData;
}

static void
_ETHLinkData_Destroy(_ETHLinkData **linkData)
{
    if ((*linkData)->athenaEthernet) {
        athenaEthernet_Release(&((*linkData)->athenaEthernet));
    }
    // remove any queued messages
    if ((*linkData)->queue) {
        while (parcDeque_Size((*linkData)->queue) > 0) {
            CCNxMetaMessage *ccnxMetaMessage = parcDeque_RemoveFirst((*linkData)->queue);
            ccnxMetaMessage_Release(&ccnxMetaMessage);
        }
        parcDeque_Release(&((*linkData)->queue));
    }
    if ((*linkData)->multiplexTable) {
        parcHashCodeTable_Destroy(&((*linkData)->multiplexTable));
    }
    if ((*linkData)->fragmenter) {
        athenaFragmenter_Release(&((*linkData)->fragmenter));
    }
    parcMemory_Deallocate(linkData);
}

/**
 * @abstract create link name based on linkData
 * @discussion
 *
 * @param [in] linkData
 * @param [in] listener flag
 * @return allocated name, must be released with parcMemory_Deallocate()
 *
 * Example:
 * @code
 * {
 *
 * }
 * @endcode
 */
static const char *
_createNameFromLinkData(_ETHLinkData *linkData, bool listener)
{
    char nameBuffer[MAXPATHLEN];
    const char *protocol = "eth";

    // Get our local hostname and port
    char myMACString[ETHER_ADDR_LEN * 3];
    char peerMACString[ETHER_ADDR_LEN * 3];

    _addressToString(linkData->link.myAddress.ether_addr_octet, myMACString);

    if (listener) {
        sprintf(nameBuffer, "%s://%s:%s", protocol, athenaEthernet_GetName(linkData->athenaEthernet), myMACString);
    } else {
        _addressToString(linkData->link.peerAddress.ether_addr_octet, peerMACString);
        sprintf(nameBuffer, "%s://%s:%s<->%s", protocol, athenaEthernet_GetName(linkData->athenaEthernet), myMACString, peerMACString);
    }

    return parcMemory_StringDuplicate(nameBuffer, strlen(nameBuffer));
}

static int
_sendIoVector(AthenaTransportLink *athenaTransportLink, struct ether_header *header, const struct iovec *iovec, size_t iovcnt)
{
    struct _ETHLinkData *linkData = athenaTransportLink_GetPrivateData(athenaTransportLink);

    // A new iovec to contain the header and packet data
    struct iovec iov[iovcnt + 1];

    // Prepend the header
    iov[0].iov_len = sizeof(struct ether_header);
    iov[0].iov_base = header;

    // Append message content
    size_t messageLength = sizeof(struct ether_header);
    for (int i = 0; i < iovcnt; i++) {
        iov[i + 1].iov_len = iovec[i].iov_len;
        iov[i + 1].iov_base = iovec[i].iov_base;
        messageLength += iov[i + 1].iov_len;
    }
    iovcnt++; // increment for the header
    parcLog_Debug(athenaTransportLink_GetLogger(athenaTransportLink),
                  "sending message (size=%d)", messageLength);

    ssize_t writeCount = 0;
    writeCount = athenaEthernet_Send(linkData->athenaEthernet, iov, iovcnt);

    // on error close the link, else return to retry a zero write
    if (writeCount == -1) {
        if ((errno == EAGAIN) || (errno == EINTR)) {
            parcLog_Info(athenaTransportLink_GetLogger(athenaTransportLink), "send retry");
            linkData->_stats.send_Retry++;
            return -1;
        }
        athenaTransportLink_SetEvent(athenaTransportLink, AthenaTransportLinkEvent_Error);

        parcLog_Error(athenaTransportLink_GetLogger(athenaTransportLink),
                      "send error (%s)", strerror(errno));
        linkData->_stats.send_Error++;
        return -1;
    }

    // Short write
    if (writeCount != messageLength) {
        linkData->_stats.send_ShortWrite++;
        parcLog_Debug(athenaTransportLink_GetLogger(athenaTransportLink), "short write (%u < %u)", writeCount, messageLength);
        errno = EIO;
        return -1;
    }

    return 0;
}

static int
_ETHSend(AthenaTransportLink *athenaTransportLink, CCNxMetaMessage *ccnxMetaMessage)
{
    struct _ETHLinkData *linkData = athenaTransportLink_GetPrivateData(athenaTransportLink);

    // Construct the ethernet header to prepend
    struct ether_header header;
    memcpy(header.ether_shost, &(linkData->link.myAddress), ETHER_ADDR_LEN * sizeof(uint8_t));
    memcpy(header.ether_dhost, &(linkData->link.peerAddress), ETHER_ADDR_LEN * sizeof(uint8_t));
    header.ether_type = htons(athenaEthernet_GetEtherType(linkData->athenaEthernet));

    if (ccnxTlvDictionary_GetSchemaVersion(ccnxMetaMessage) == CCNxTlvDictionary_SchemaVersion_V0) {
        parcLog_Debug(athenaTransportLink_GetLogger(athenaTransportLink),
                      "sending deprecated version %d message\n", ccnxTlvDictionary_GetSchemaVersion(ccnxMetaMessage));
    }

    CCNxCodecNetworkBufferIoVec *messageIoVector = athenaTransportLinkModule_GetMessageIoVector(ccnxMetaMessage);
    size_t messageLength = ccnxCodecNetworkBufferIoVec_Length(messageIoVector);
    PARCBuffer *message = NULL;

    int fragmentNumber = 0;
    CCNxCodecEncodingBufferIOVec *ioFragment = NULL;
    const struct iovec *iovec = NULL;
    size_t iovcnt = 0;
    size_t maxPayloadSize = linkData->link.mtu;

    // Get initial IO vector message or message fragment if we need, and have, fragmentation support.
    if (messageLength <= maxPayloadSize) {
        iovec = ccnxCodecNetworkBufferIoVec_GetArray(messageIoVector);
        iovcnt = ccnxCodecNetworkBufferIoVec_GetCount(messageIoVector);
    } else {
        if (linkData->fragmenter != NULL) {
            // Right now we work with the message in a contiguous buffer.
            // We can optimize the copy out by working directly with the IO Vector.
            message = athenaTransportLinkModule_CreateMessageBuffer(ccnxMetaMessage);
            ioFragment = athenaFragmenter_CreateFragment(linkData->fragmenter, message,
                                                         maxPayloadSize, fragmentNumber);
            if (ioFragment) {
                iovec = ioFragment->iov;
                iovcnt = ioFragment->iovcnt;
                fragmentNumber++;
            }
        } else { // message too big and no fragmenter provided
            ccnxCodecNetworkBufferIoVec_Release(&messageIoVector);
            parcLog_Debug(athenaTransportLink_GetLogger(athenaTransportLink),
                          "message larger than mtu and no fragmention support (size=%d)", messageLength);
            errno = EMSGSIZE;
            return -1;
        }
    }

    int sendResult = 0;
    while (iovec) {
        sendResult = _sendIoVector(athenaTransportLink, &header, iovec, iovcnt);
        iovec = NULL;
        if (ioFragment) {
            ccnxCodecEncodingBufferIOVec_Release(&ioFragment);
        }
        if (sendResult == -1) {
            break;
        }
        if (messageLength > maxPayloadSize) {
            ioFragment = athenaFragmenter_CreateFragment(linkData->fragmenter, message,
                                                         maxPayloadSize, fragmentNumber);
            if (ioFragment) {
                iovec = ioFragment->iov;
                iovcnt = ioFragment->iovcnt;
                fragmentNumber++;
            }
        }
    }

    ccnxCodecNetworkBufferIoVec_Release(&messageIoVector);
    if (message) {
        parcBuffer_Release(&message);
    }

    return sendResult;
}

static void
_ETHClose(AthenaTransportLink *athenaTransportLink)
{
    parcLog_Info(athenaTransportLink_GetLogger(athenaTransportLink),
                 "link %s closed", athenaTransportLink_GetName(athenaTransportLink));
    _ETHLinkData *linkData = athenaTransportLink_GetPrivateData(athenaTransportLink);
    _ETHLinkData_Destroy(&linkData);
}

//
// Return a message which has been queued on this link from an ethernet listener
//
static CCNxMetaMessage *
_ETHReceiveProxy(AthenaTransportLink *athenaTransportLink)
{
    CCNxMetaMessage *ccnxMetaMessage = NULL;
    struct _ETHLinkData *linkData = athenaTransportLink_GetPrivateData(athenaTransportLink);
    if (parcDeque_Size(linkData->queue) > 0) {
        ccnxMetaMessage = parcDeque_RemoveFirst(linkData->queue);
        if (parcDeque_Size(linkData->queue) > 0) { // if there's another message, post an event.
            athenaTransportLink_SetEvent(athenaTransportLink, AthenaTransportLinkEvent_Receive);
        }
    }
    return ccnxMetaMessage;
}

static void
_setConnectLinkState(AthenaTransportLink *athenaTransportLink, _ETHLinkData *linkData)
{
    assertNull(athenaTransportLink_GetPrivateData(athenaTransportLink), "Private data has already been set.");
    athenaTransportLink_SetPrivateData(athenaTransportLink, linkData);

    // Register file descriptor to be polled.  This must be set before adding the link (case ???).
    athenaTransportLink_SetEventFd(athenaTransportLink, athenaEthernet_GetDescriptor(linkData->athenaEthernet));

    // Determine and flag the link cost for forwarding messages.
    // Messages without sufficient hop count collateral will be dropped.
    // Local links will always be allowed to be taken (i.e. localhost).
    bool isLocal = false;
    if (memcmp(&(linkData->link.peerAddress), &(linkData->link.myAddress), sizeof(struct ether_addr)) == 0) { // a local connection
        isLocal = true;
    }
    athenaTransportLink_SetLocal(athenaTransportLink, isLocal);
}

static AthenaTransportLink *
_cloneNewLink(AthenaTransportLink *athenaTransportLink, struct ether_addr *peerAddress, socklen_t peerAddressLength)
{
    struct _ETHLinkData *linkData = athenaTransportLink_GetPrivateData(athenaTransportLink);

    _ETHLinkData *newLinkData = _ETHLinkData_Create();

    // Use the same fragmentation as our parent
    if (linkData->fragmenter) {
        newLinkData->fragmenter = athenaFragmenter_Create(athenaTransportLink, linkData->fragmenter->moduleName);
        if (newLinkData->fragmenter == NULL) {
            parcLog_Error(athenaTransportLink_GetLogger(athenaTransportLink),
                          "Failed to open/initialize %s fragmenter for new link: %s",
                          linkData->fragmenter->moduleName, strerror(errno));
        }
    }

    // Propagate our parents MTU
    newLinkData->link.mtu = linkData->link.mtu;

    // We use our parents fd to send, and receive demux'd messages from our parent on our queue
    newLinkData->athenaEthernet = athenaEthernet_Acquire(linkData->athenaEthernet);
    newLinkData->queue = parcDeque_Create();
    assertNotNull(newLinkData->queue, "Could not create data queue for new link");

    newLinkData->link.myAddressLength = linkData->link.myAddressLength;
    memcpy(&newLinkData->link.myAddress, &linkData->link.myAddress, linkData->link.myAddressLength);

    newLinkData->link.peerAddressLength = peerAddressLength;
    memcpy(&newLinkData->link.peerAddress, peerAddress, peerAddressLength);

    // Clone a new link from the current listener.
    const char *derivedLinkName = _createNameFromLinkData(newLinkData, false);
    AthenaTransportLink *newTransportLink = athenaTransportLink_Clone(athenaTransportLink,
                                                                      derivedLinkName,
                                                                      _ETHSend,
                                                                      _ETHReceiveProxy,
                                                                      _ETHClose);
    if (newTransportLink == NULL) {
        parcLog_Error(athenaTransportLink_GetLogger(athenaTransportLink),
                      "athenaTransportLink_Clone failed");
        parcMemory_Deallocate(&derivedLinkName);
        _ETHLinkData_Destroy(&newLinkData);
        return NULL;
    }

    _setConnectLinkState(newTransportLink, newLinkData);

    // Send the new link up to be added.
    int result = athenaTransportLink_AddLink(athenaTransportLink, newTransportLink);
    if (result == -1) {
        parcLog_Error(athenaTransportLink_GetLogger(athenaTransportLink),
                      "athenaTransportLink_AddLink failed: %s", strerror(errno));
        _ETHLinkData_Destroy(&newLinkData);
        athenaTransportLink_Release(&newTransportLink);
    } else {
        parcLog_Info(athenaTransportLink_GetLogger(athenaTransportLink),
                     "new link accepted by %s: %s %s",
                     athenaTransportLink_GetName(athenaTransportLink), derivedLinkName,
                     athenaTransportLink_IsNotLocal(athenaTransportLink) ? "" : "(Local)");
    }

    parcMemory_Deallocate(&derivedLinkName);

    // Enable Sends
    athenaTransportLink_SetEvent(newTransportLink, AthenaTransportLinkEvent_Send);

    // Could pass a message back here regarding the new link.
    return newTransportLink;
}

static void
_queueMessage(AthenaTransportLink *athenaTransportLink, CCNxMetaMessage *ccnxMetaMessage)
{
    struct _ETHLinkData *linkData = athenaTransportLink_GetPrivateData(athenaTransportLink);
    parcDeque_Append(linkData->queue, ccnxMetaMessage);
    athenaTransportLink_SetEvent(athenaTransportLink, AthenaTransportLinkEvent_Receive);
}

static unsigned long
_hashAddress(struct ether_addr *address)
{
    unsigned long id = 1; // localhost mac is 00:00:..., result must be non-zero
    int i;

    for (i = 0; i < ETHER_ADDR_LEN; i++) {
        id <<= 8;
        id += address->ether_addr_octet[i];
    }
    return id;
}

static void
_demuxDelivery(AthenaTransportLink *athenaTransportLink, CCNxMetaMessage *ccnxMetaMessage, struct ether_addr *peerAddress, socklen_t peerAddressLength)
{
    struct _ETHLinkData *linkData = athenaTransportLink_GetPrivateData(athenaTransportLink);

    AthenaTransportLink *demuxLink = parcHashCodeTable_Get(linkData->multiplexTable, (void *) _hashAddress(peerAddress));

    // If it's an unknown peer, try to create a new link
    if (demuxLink == NULL) {
        demuxLink = _cloneNewLink(athenaTransportLink, peerAddress, peerAddressLength);
        if (demuxLink) {
            parcHashCodeTable_Add(linkData->multiplexTable, (void *) _hashAddress(peerAddress), demuxLink);
        }
    }

    // If there's no existing link and a new one can't be created, drop the message
    if (demuxLink == NULL) {
        ccnxMetaMessage_Release(&ccnxMetaMessage);
        return;
    }

    _queueMessage(demuxLink, ccnxMetaMessage);
}

//
// Receive a message from the specified link.
//
static CCNxMetaMessage *
_ETHReceiveMessage(AthenaTransportLink *athenaTransportLink, struct ether_addr *peerAddress, socklen_t *peerAddressLength)
{
    struct _ETHLinkData *linkData = athenaTransportLink_GetPrivateData(athenaTransportLink);
    CCNxMetaMessage *ccnxMetaMessage = NULL;
    AthenaTransportLinkEvent events = 0;

    PARCBuffer *message = athenaEthernet_Receive(linkData->athenaEthernet, -1, &events);
    if (message == NULL) {
        return NULL;
    }
    // Mark any pending events
    if (events) {
        athenaTransportLink_SetEvent(athenaTransportLink, events);
    }

    // Map the header
    struct ether_header *header = parcBuffer_Overlay(message, sizeof(struct ether_header));

    // If the destination does not match my address, drop the message
    if (memcmp(&linkData->link.myAddress, header->ether_dhost, ETHER_ADDR_LEN * sizeof(uint8_t)) != 0) {
        linkData->_stats.receive_NoLinkDestination++;
        parcBuffer_Release(&message);
        return NULL;
    }
    assertTrue(header->ether_type == htons(CCNX_ETHERTYPE), "Unexpected ether type %x", header->ether_type);

    // Set peerAddress from header source address
    *peerAddressLength = ETHER_ADDR_LEN * sizeof(uint8_t);
    memcpy(peerAddress, header->ether_shost, *peerAddressLength);

    parcBuffer_SetPosition(message, sizeof(struct ether_header));
    PARCBuffer *wireFormatBuffer = parcBuffer_Slice(message);
    parcBuffer_Release(&message);

    // If it's not a fragment returns our passed in wireFormatBuffer, otherwise it owns the buffer and eventually
    // passes back the aggregated message after receiving all its fragments, returning NULL in the mean time.
    wireFormatBuffer = athenaFragmenter_ReceiveFragment(linkData->fragmenter, wireFormatBuffer);

    if (wireFormatBuffer != NULL) {
        // Construct, and return a ccnxMetaMessage from the wire format buffer.
        parcBuffer_SetPosition(wireFormatBuffer, 0);
        ccnxMetaMessage = ccnxMetaMessage_CreateFromWireFormatBuffer(wireFormatBuffer);
        if (ccnxMetaMessage == NULL) {
            linkData->_stats.receive_DecodeFailed++;
            parcLog_Error(athenaTransportLink_GetLogger(athenaTransportLink), "Failed to decode message from received packet.");
        } else if (ccnxTlvDictionary_GetSchemaVersion(ccnxMetaMessage) == CCNxTlvDictionary_SchemaVersion_V0) {
            parcLog_Debug(athenaTransportLink_GetLogger(athenaTransportLink),
                          "received deprecated version %d message\n", ccnxTlvDictionary_GetSchemaVersion(ccnxMetaMessage));
        } else {
            parcLog_Debug(athenaTransportLink_GetLogger(athenaTransportLink), "received message (size=%d)",
                          parcBuffer_Remaining(wireFormatBuffer));
        }
        parcBuffer_Release(&wireFormatBuffer);
    }

    return ccnxMetaMessage;
}

//
// Receive a message from a point to point link.
//
static CCNxMetaMessage *
_ETHReceive(AthenaTransportLink *athenaTransportLink)
{
    struct _ETHLinkData *linkData = athenaTransportLink_GetPrivateData(athenaTransportLink);
    struct ether_addr peerAddress;
    socklen_t peerAddressLength;
    CCNxMetaMessage *ccnxMetaMessage = _ETHReceiveMessage(athenaTransportLink, &peerAddress, &peerAddressLength);

    // If the souce does not match my configured link peer address, drop the message
    if (memcmp(&linkData->link.peerAddress, &peerAddress, peerAddressLength) != 0) {
        linkData->_stats.receive_PeerNotConfigured++;
        ccnxMetaMessage_Release(&ccnxMetaMessage);
        return NULL;
    }

    return ccnxMetaMessage;
}

//
// Receive and queue a message from an ethernet listener.
//
static CCNxMetaMessage *
_ETHReceiveListener(AthenaTransportLink *athenaTransportLink)
{
    struct ether_addr peerAddress;
    socklen_t peerAddressLength;
    CCNxMetaMessage *ccnxMetaMessage = _ETHReceiveMessage(athenaTransportLink, &peerAddress, &peerAddressLength);
    if (ccnxMetaMessage) {
        _demuxDelivery(athenaTransportLink, ccnxMetaMessage, &peerAddress, peerAddressLength);
    }
    return NULL;
}

//
// Open a point to point connection.
//
static AthenaTransportLink *
_ETHOpenConnection(AthenaTransportLinkModule *athenaTransportLinkModule, const char *linkName, const char *device, struct ether_addr *source, struct ether_addr *destination, size_t mtu)
{
    const char *derivedLinkName;

    _ETHLinkData *linkData = _ETHLinkData_Create();

    linkData->athenaEthernet = athenaEthernet_Create(athenaTransportLinkModule_GetLogger(athenaTransportLinkModule),
                                                     device, CCNX_ETHERTYPE);
    if (linkData->athenaEthernet == NULL) {
        _ETHLinkData_Destroy(&linkData);
        return NULL;
    }

    if (mtu) {
        linkData->link.mtu = mtu;
        if (mtu > athenaEthernet_GetMTU(linkData->athenaEthernet)) {
            parcLog_Warning(athenaTransportLinkModule_GetLogger(athenaTransportLinkModule),
                            "Setting mtu larger than transport allows (%d > %d)", mtu,
                            athenaEthernet_GetMTU(linkData->athenaEthernet));
        }
    } else {
        linkData->link.mtu = athenaEthernet_GetMTU(linkData->athenaEthernet);
    }

    // Use our default MAC address if none specified.
    if (source == NULL) {
        athenaEthernet_GetMAC(linkData->athenaEthernet, &(linkData->link.myAddress));
        linkData->link.myAddressLength = ETHER_ADDR_LEN;
    } else {
        memcpy(&(linkData->link.myAddress), source, sizeof(struct ether_addr));
    }

    // If there's no destination specified, drop the request.
    if (destination == NULL) {
        _ETHLinkData_Destroy(&linkData);
        return NULL;
    }

    // Copy the peer destination address into our link data
    memcpy(&(linkData->link.peerAddress), destination, sizeof(struct ether_addr));
    linkData->link.peerAddressLength = ETHER_ADDR_LEN;

    derivedLinkName = _createNameFromLinkData(linkData, false);

    if (linkName == NULL) {
        linkName = derivedLinkName;
    }

    AthenaTransportLink *athenaTransportLink = athenaTransportLink_Create(linkName,
                                                                          _ETHSend,
                                                                          _ETHReceive,
                                                                          _ETHClose);
    if (athenaTransportLink == NULL) {
        parcLog_Error(athenaTransportLink_GetLogger(athenaTransportLink),
                      "athenaTransportLink_Create failed");
        parcMemory_Deallocate(&derivedLinkName);
        _ETHLinkData_Destroy(&linkData);
        return NULL;
    }

    athenaTransportLink_SetLogLevel(athenaTransportLink, parcLog_GetLevel(athenaTransportLinkModule_GetLogger(athenaTransportLinkModule)));
    _setConnectLinkState(athenaTransportLink, linkData);

    // Enable Sends
    athenaTransportLink_SetEvent(athenaTransportLink, AthenaTransportLinkEvent_Send);

    parcLog_Info(athenaTransportLink_GetLogger(athenaTransportLink),
                 "new link established: Name=\"%s\" (%s)", linkName, derivedLinkName);

    parcMemory_Deallocate(&derivedLinkName);
    return athenaTransportLink;
    return NULL;
}

static bool
_connectionEquals(const void *a, const void *b)
{
    return (unsigned long *) a == (unsigned long *) b;
}

static HashCodeType
_connectionHashCode(const void *a)
{
    return (HashCodeType) a;
}

static void
_closeConnection(void **link)
{
    AthenaTransportLink *athenaTransportLink = (AthenaTransportLink *) *link;
    athenaTransportLink_Close(athenaTransportLink);
}

//
// Open a listener which will create new links when messages arrive and queue them appropriately.
// Listeners are inherently insecure, as an adversary could easily create many connections that are never closed.
//
static AthenaTransportLink *
_ETHOpenListener(AthenaTransportLinkModule *athenaTransportLinkModule, const char *linkName, const char *device, struct ether_addr *source, size_t mtu)
{
    const char *derivedLinkName;

    _ETHLinkData *linkData = _ETHLinkData_Create();
    linkData->multiplexTable = parcHashCodeTable_Create(_connectionEquals, _connectionHashCode, NULL, _closeConnection);
    assertNotNull(linkData->multiplexTable, "Could not create multiplex table for new listener");

    linkData->athenaEthernet = athenaEthernet_Create(athenaTransportLinkModule_GetLogger(athenaTransportLinkModule),
                                                     device, CCNX_ETHERTYPE);
    if (linkData->athenaEthernet == NULL) {
        _ETHLinkData_Destroy(&linkData);
        return NULL;
    }

    if (mtu) {
        linkData->link.mtu = mtu;
        if (mtu > athenaEthernet_GetMTU(linkData->athenaEthernet)) {
            parcLog_Warning(athenaTransportLinkModule_GetLogger(athenaTransportLinkModule),
                            "Setting mtu larger than transport allows (%d > %d)", mtu,
                            athenaEthernet_GetMTU(linkData->athenaEthernet));
        }
    } else {
        linkData->link.mtu = athenaEthernet_GetMTU(linkData->athenaEthernet);
    }

    // Use specified source MAC address, or default to device MAC
    if (source) {
        memcpy(&(linkData->link.myAddress), source, sizeof(struct ether_addr));
    } else {
        athenaEthernet_GetMAC(linkData->athenaEthernet, &(linkData->link.myAddress));
    }
    linkData->link.myAddressLength = ETHER_ADDR_LEN;

    if (linkData->athenaEthernet == NULL) {
        parcLog_Error(athenaTransportLinkModule_GetLogger(athenaTransportLinkModule),
                      "athenaEthernet_Create error");
        _ETHLinkData_Destroy(&linkData);
        return NULL;
    }

    derivedLinkName = _createNameFromLinkData(linkData, true);

    if (linkName == NULL) {
        linkName = derivedLinkName;
    }

    // Listener doesn't require a send method.  The receive method is used to establish new connections.
    AthenaTransportLink *athenaTransportLink = athenaTransportLink_Create(linkName,
                                                                          NULL,
                                                                          _ETHReceiveListener,
                                                                          _ETHClose);
    if (athenaTransportLink == NULL) {
        parcLog_Error(athenaTransportLink_GetLogger(athenaTransportLink),
                      "athenaTransportLink_Create failed");
        parcMemory_Deallocate(&derivedLinkName);
        _ETHLinkData_Destroy(&linkData);
        return athenaTransportLink;
    }
    athenaTransportLink_SetLogLevel(athenaTransportLink, parcLog_GetLevel(athenaTransportLinkModule_GetLogger(athenaTransportLinkModule)));

    athenaTransportLink_SetPrivateData(athenaTransportLink, linkData);
    athenaTransportLink_SetEventFd(athenaTransportLink, athenaEthernet_GetDescriptor(linkData->athenaEthernet));

    // Links established for listening are not used to route messages.
    // They can be kept in a listener list that doesn't consume a linkId.
    athenaTransportLink_SetRoutable(athenaTransportLink, false);

    parcLog_Info(athenaTransportLink_GetLogger(athenaTransportLink),
                 "new listener established: Name=\"%s\" (%s)", linkName, derivedLinkName);

    parcMemory_Deallocate(&derivedLinkName);
    return athenaTransportLink;
}

#define LINK_NAME_SPECIFIER "name%3D"
#define LOCAL_LINK_FLAG "local%3D"
#define LINK_MTU_SIZE "mtu%3D"
#define SRC_LINK_SPECIFIER "src%3D"
#define ETH_LISTENER_FLAG "listener"
#define FRAGMENTER "fragmenter%3D"

static int
_parseFragmenterName(const char *token, char *name)
{
    if (sscanf(token, "%*[^%%]%%3D%s", name) != 1) {
        return -1;
    }
    return 0;
}

static int
_parseLinkName(const char *token, char *name)
{
    if (sscanf(token, "%*[^%%]%%3D%s", name) != 1) {
        return -1;
    }
    return 0;
}

static int
_parseLocalFlag(const char *token)
{
    int forceLocal = 0;
    char localFlag[MAXPATHLEN] = { 0 };
    if (sscanf(token, "%*[^%%]%%3D%s", localFlag) != 1) {
        return 0;
    }
    if (strncasecmp(localFlag, "false", strlen("false")) == 0) {
        forceLocal = AthenaTransportLink_ForcedNonLocal;
    } else if (strncasecmp(localFlag, "true", strlen("true")) == 0) {
        forceLocal = AthenaTransportLink_ForcedLocal;
    }
    return forceLocal;
}

static size_t
_parseMTU(const char *token, size_t *mtu)
{
    if (sscanf(token, "%*[^%%]%%3D%zd", mtu) != 1) {
        return -1;
    }
    return *mtu;
}

static int
_parseSrc(AthenaTransportLinkModule *athenaTransportLinkModule, const char *token, struct ether_addr *srcMAC)
{
    char srcAddressString[NI_MAXHOST];
    if (sscanf(token, "%*[^%%]%%3D%[^%%]", srcAddressString) != 1) {
        errno = EINVAL;
        return -1;
    }
    return _stringToEtherAddr(srcMAC, srcAddressString);
}

static int
_parseAddress(const char *string, struct ether_addr *address)
{
    char addressString[NI_MAXHOST] = { 0 };
    char device[NI_MAXHOST] = { 0 };
    if (sscanf(string, "%[^:]:%s", device, addressString) != 1) {
        sscanf(string, ":%s", addressString);
    }
    if (addressString[0] == '\0') { // If no MAC specified, look up address
        return athenaEthernet_GetInterfaceMAC(device, address);
    } else {
        return _stringToEtherAddr(address, addressString);
    }
    return 0;
}

#include <parc/algol/parc_URIAuthority.h>

static AthenaTransportLink *
_ETHOpen(AthenaTransportLinkModule *athenaTransportLinkModule, PARCURI *connectionURI)
{
    AthenaTransportLink *result = 0;
    char device[NI_MAXHOST];

    const char *authorityString = parcURI_GetAuthority(connectionURI);
    if (authorityString == NULL) {
        parcLog_Error(athenaTransportLinkModule_GetLogger(athenaTransportLinkModule),
                      "Unable to parse connection authority %s", authorityString);
        errno = EINVAL;
        return NULL;
    }
    PARCURIAuthority *authority = parcURIAuthority_Parse(authorityString);
    const char *URIHostname = parcURIAuthority_GetHostName(authority);
    strcpy(device, URIHostname);

    bool srcMACSpecified = false;
    struct ether_addr srcMAC = { { 0 } };
    struct ether_addr destMAC = { { 0 } };
    if (_parseAddress(authorityString, &destMAC) != 0) {
        parcURIAuthority_Release(&authority);
        errno = EINVAL;
        return NULL;
    }
    parcURIAuthority_Release(&authority);

    bool isListener = false;
    char *linkName = NULL;
    char specifiedLinkName[MAXPATHLEN] = { 0 };
    char *fragmenterName = NULL;
    char specifiedFragmenterName[MAXPATHLEN] = { 0 };
    size_t mtu = 0;
    int forceLocal = 0;

    PARCURIPath *remainder = parcURI_GetPath(connectionURI);
    size_t segments = parcURIPath_Count(remainder);
    for (int i = 0; i < segments; i++) {
        PARCURISegment *segment = parcURIPath_Get(remainder, i);
        const char *token = parcURISegment_ToString(segment);

        if (strcasecmp(token, ETH_LISTENER_FLAG) == 0) {
            // Packet source for listener is destination parameter, unless told otherwise
            if (srcMACSpecified == false) {
                memcpy(&srcMAC, &destMAC, sizeof(struct ether_addr));
            }
            isListener = true;
            parcMemory_Deallocate(&token);
            continue;
        }

        if (strncasecmp(token, FRAGMENTER, strlen(FRAGMENTER)) == 0) {
            if (_parseFragmenterName(token, specifiedFragmenterName) != 0) {
                parcLog_Error(athenaTransportLinkModule_GetLogger(athenaTransportLinkModule),
                              "Improper fragmenter name specification (%s)", token);
                parcMemory_Deallocate(&token);
                errno = EINVAL;
                return NULL;
            }
            fragmenterName = specifiedFragmenterName;
            parcMemory_Deallocate(&token);
            continue;
        }

        if (strncasecmp(token, SRC_LINK_SPECIFIER, strlen(SRC_LINK_SPECIFIER)) == 0) {
            if (_parseSrc(athenaTransportLinkModule, token, &srcMAC) != 0) {
                parcLog_Error(athenaTransportLinkModule_GetLogger(athenaTransportLinkModule),
                              "Improper connection source specification (%s)", token);
                parcMemory_Deallocate(&token);
                return NULL;
            }
            srcMACSpecified = true;
            parcMemory_Deallocate(&token);
            continue;
        }

        if (strncasecmp(token, LINK_MTU_SIZE, strlen(LINK_MTU_SIZE)) == 0) {
            if (_parseMTU(token, &mtu) == -1) {
                parcLog_Error(athenaTransportLinkModule_GetLogger(athenaTransportLinkModule),
                              "Improper MTU specification (%s)", token);
                parcMemory_Deallocate(&token);
                errno = EINVAL;
                return NULL;
            }
            parcMemory_Deallocate(&token);
            continue;
        }

        if (strncasecmp(token, LINK_NAME_SPECIFIER, strlen(LINK_NAME_SPECIFIER)) == 0) {
            if (_parseLinkName(token, specifiedLinkName) != 0) {
                parcLog_Error(athenaTransportLinkModule_GetLogger(athenaTransportLinkModule),
                              "Improper connection name specification (%s)", token);
                parcMemory_Deallocate(&token);
                errno = EINVAL;
                return NULL;
            }
            linkName = specifiedLinkName;
            parcMemory_Deallocate(&token);
            continue;
        }

        if (strncasecmp(token, LOCAL_LINK_FLAG, strlen(LOCAL_LINK_FLAG)) == 0) {
            forceLocal = _parseLocalFlag(token);
            if (forceLocal == 0) {
                parcLog_Error(athenaTransportLinkModule_GetLogger(athenaTransportLinkModule),
                              "Improper local specification (%s)", token);
                parcMemory_Deallocate(&token);
                errno = EINVAL;
                return NULL;
            }
            parcMemory_Deallocate(&token);
            continue;
        }

        parcLog_Error(athenaTransportLinkModule_GetLogger(athenaTransportLinkModule),
                      "Unknown connection parameter (%s)", token);
        parcMemory_Deallocate(&token);
        errno = EINVAL;
        return NULL;
    }

    if (isListener) {
        result = _ETHOpenListener(athenaTransportLinkModule, linkName, device, &srcMAC, mtu);
    } else {
        if (srcMACSpecified) {
            result = _ETHOpenConnection(athenaTransportLinkModule, linkName, device, &srcMAC, &destMAC, mtu);
        } else {
            result = _ETHOpenConnection(athenaTransportLinkModule, linkName, device, NULL, &destMAC, mtu);
        }
    }

    if (result && fragmenterName) {
        struct _ETHLinkData *linkData = athenaTransportLink_GetPrivateData(result);
        linkData->fragmenter = athenaFragmenter_Create(result, fragmenterName);
        if (linkData->fragmenter == NULL) {
            parcLog_Error(athenaTransportLinkModule_GetLogger(athenaTransportLinkModule),
                          "Failed to open/initialize %s fragmenter for %s: %s", fragmenterName, linkName, strerror(errno));
            athenaTransportLink_Close(result);
            return NULL;
        }
    }

    // forced IsLocal/IsNotLocal, mainly for testing
    if (result && forceLocal) {
        athenaTransportLink_ForceLocal(result, forceLocal);
    }

    return result;
}

static int
_ETHPoll(AthenaTransportLink *athenaTransportLink, int timeout)
{
    struct _ETHLinkData *linkData = athenaTransportLink_GetPrivateData(athenaTransportLink);

    if (linkData->queue) {
        return (int) parcDeque_Size(linkData->queue);
    }
    return 0;
}

PARCArrayList *
athenaTransportLinkModuleETH_Init()
{
    // Module for providing Ethernet links.
    AthenaTransportLinkModule *athenaTransportLinkModule;
    PARCArrayList *moduleList = parcArrayList_Create(NULL);
    assertNotNull(moduleList, "parcArrayList_Create failed to create module list");

    athenaTransportLinkModule = athenaTransportLinkModule_Create("ETH",
                                                                 _ETHOpen,
                                                                 _ETHPoll);
    assertNotNull(athenaTransportLinkModule, "parcMemory_AllocateAndClear failed allocate ETH athenaTransportLinkModule");
    bool result = parcArrayList_Add(moduleList, athenaTransportLinkModule);
    assertTrue(result == true, "parcArrayList_Add failed");

    return moduleList;
}

void
athenaTransportLinkModuleETH_Fini()
{
}
