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
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>

#include <parc/algol/parc_Network.h>
#include <parc/algol/parc_Deque.h>
#include <parc/algol/parc_HashCodeTable.h>
#include <parc/algol/parc_Hash.h>
#include <ccnx/forwarder/athena/athena_TransportLinkModule.h>
#include <ccnx/forwarder/athena/athena_Fragmenter.h>

#include <ccnx/common/codec/ccnxCodec_TlvPacket.h>

#define UDP_SCHEME "udp"
#define UDP6_SCHEME "udp6"

//
// Platform support required for managing SIGPIPE.
//
// MacOS (BSD) provides SO_NOSIGPIPE socket options for returning EPIPE on writes,
// Linux provides MSB_NOSIGNAL send(2) options to allow the same.
// Alternate implementations could interpose a SIGPIPE signal handler.
//
#if defined(SO_NOSIGPIPE) // MacOS, BSD
    #define BSD_IGNORESIGPIPE 1
#elif defined(MSG_NOSIGNAL) // Primarily Linux
    #define LINUX_IGNORESIGPIPE 1
#else
"Platform not supported";
#endif

//
// Private data for each link connection
//
typedef struct _connectionPair {
    struct sockaddr_storage myAddress;
    struct sockaddr_storage peerAddress;
    size_t mtu;
} _connectionPair;

//
// Private data for each link instance
//
typedef struct _UDPLinkData {
    int fd;
    _connectionPair link;
    PARCDeque *queue;
    PARCHashCodeTable *multiplexTable;
    struct {
        size_t receive_ReadHeaderFailure;
        size_t receive_BadMessageLength;
        size_t receive_ReadError;
        size_t receive_ReadRetry;
        size_t receive_ReadWouldBlock;
        size_t receive_ShortRead;
        size_t receive_DecodeFailed;
        size_t send_ShortWrite;
        size_t send_SendRetry;
    } _stats;
    AthenaFragmenter *fragmenter;
} _UDPLinkData;

#define _UDP_DEFAULT_MTU_SIZE (1024 * 64)

static _UDPLinkData *
_UDPLinkData_Create()
{
    _UDPLinkData *linkData = parcMemory_AllocateAndClear(sizeof(_UDPLinkData));
    assertNotNull(linkData, "Could not create private data for new link");
    linkData->link.mtu = _UDP_DEFAULT_MTU_SIZE;
    return linkData;
}

static void
_UDPLinkData_Destroy(_UDPLinkData **linkData)
{
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
 * @abstract create link name based on connection information
 * @discussion
 *
 * @param [in] linkData connection information
 * @return allocated name, must be released with parcMemory_Deallocate()
 *
 * Example:
 * @code
 * {
 *
 * }
 * @endcode
 */
#define SOCKADDR_IN_LEN(s) (socklen_t)((((struct sockaddr_in *)s)->sin_family == AF_INET) ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6))
#define SOCKADDR_LEN(s) (socklen_t)sizeof(struct sockaddr)

static const char *
_createNameFromLinkData(const _connectionPair *linkData)
{
    char nameBuffer[MAXPATHLEN];
    const char *protocol;

    // Get our local hostname and port
    char myHost[NI_MAXHOST], myPort[NI_MAXSERV];
    int myResult = getnameinfo((struct sockaddr *) &linkData->myAddress, SOCKADDR_IN_LEN(&linkData->myAddress),
                               myHost, NI_MAXHOST, myPort, NI_MAXSERV, NI_NUMERICSERV);

    // Get our peer's hostname and port
    char peerHost[NI_MAXHOST], peerPort[NI_MAXSERV];
    int peerResult = 0;
    peerResult = getnameinfo((struct sockaddr *) &linkData->peerAddress, SOCKADDR_IN_LEN(&linkData->peerAddress),
                             peerHost, NI_MAXHOST, peerPort, NI_MAXSERV, NI_NUMERICSERV);

    protocol = (linkData->myAddress.ss_family == AF_INET6) ? UDP6_SCHEME : UDP_SCHEME;
    if ((peerResult == 0) && (myResult == 0)) { // point to point connection
        if (strchr(myHost, ':')) {
            if (strchr(peerHost, ':')) {
                sprintf(nameBuffer, "%s://[%s]:%s<->[%s]:%s", protocol, myHost, myPort, peerHost, peerPort);
            } else {
                sprintf(nameBuffer, "%s://[%s]:%s<->%s:%s", protocol, myHost, myPort, peerHost, peerPort);
            }
        } else if (strchr(peerHost, ':')) {
            sprintf(nameBuffer, "%s://%s:%s<->[%s]:%s", protocol, myHost, myPort, peerHost, peerPort);
        } else {
            sprintf(nameBuffer, "%s://%s:%s<->%s:%s", protocol, myHost, myPort, peerHost, peerPort);
        }
    } else if (myResult == 0) { // listener only
        if (strchr(myHost, ':')) {
            sprintf(nameBuffer, "%s://[%s]:%s", protocol, myHost, myPort);
        } else {
            sprintf(nameBuffer, "%s://%s:%s", protocol, myHost, myPort);
        }
    } else { // some unknown possibility
        sprintf(nameBuffer, "%s://Unknown", protocol);
    }

    return parcMemory_StringDuplicate(nameBuffer, strlen(nameBuffer));
}

PARCBuffer *
_encodingBufferIOVecToPARCBuffer(CCNxCodecEncodingBufferIOVec *encodingBufferIOVec)
{
    size_t totalBytes = 0;
    for (int i = 0; i < encodingBufferIOVec->iovcnt; i++) {
        totalBytes += encodingBufferIOVec->iov[i].iov_len;
    }
    PARCBuffer *buffer = parcBuffer_Allocate(totalBytes);
    assertNotNull(buffer, "parcBuffer_Allocate failed to allocate %zu array", totalBytes);

    for (int i = 0; i < encodingBufferIOVec->iovcnt; i++) {
        parcBuffer_PutArray(buffer, encodingBufferIOVec->iov[i].iov_len, encodingBufferIOVec->iov[i].iov_base);
    }
    parcBuffer_SetPosition(buffer, 0);
    return buffer;
}

static int
_sendBuffer(AthenaTransportLink *athenaTransportLink, PARCBuffer *wireFormatBuffer)
{
    struct _UDPLinkData *linkData = athenaTransportLink_GetPrivateData(athenaTransportLink);
    size_t length = parcBuffer_Limit(wireFormatBuffer);
    const char *buffer = parcBuffer_Overlay(wireFormatBuffer, length);

    parcLog_Debug(athenaTransportLink_GetLogger(athenaTransportLink),
                  "sending message (size=%d)", length);

    ssize_t writeCount = 0;
#ifdef LINUX_IGNORESIGPIPE
    writeCount = sendto(linkData->fd, buffer, length, MSG_NOSIGNAL,
                        (struct sockaddr *)&linkData->link.peerAddress, SOCKADDR_IN_LEN(&linkData->link.peerAddress));
#else
    writeCount = sendto(linkData->fd, buffer, length, 0,
                        (struct sockaddr *)&linkData->link.peerAddress, SOCKADDR_IN_LEN(&linkData->link.peerAddress));
#endif

    // on error close the link, else return to retry a zero write
    if (writeCount == -1) {
        if ((errno == EAGAIN) || (errno == EINTR)) {
            linkData->_stats.send_SendRetry++;
            parcLog_Debug(athenaTransportLink_GetLogger(athenaTransportLink), "send retry (%s)", strerror(errno));
        } else {
            athenaTransportLink_SetEvent(athenaTransportLink, AthenaTransportLinkEvent_Error);
            parcLog_Error(athenaTransportLink_GetLogger(athenaTransportLink),
                          "send error (%s)", strerror(errno));
        }
        return -1;
    }

    // Short write
    if (writeCount != length) {
        linkData->_stats.send_ShortWrite++;
        parcLog_Debug(athenaTransportLink_GetLogger(athenaTransportLink), "short write");
        return -1;
    }

    return 0;
}

static int
_UDPSend(AthenaTransportLink *athenaTransportLink, CCNxMetaMessage *ccnxMetaMessage)
{
    struct _UDPLinkData *linkData = athenaTransportLink_GetPrivateData(athenaTransportLink);

    if (ccnxTlvDictionary_GetSchemaVersion(ccnxMetaMessage) == CCNxTlvDictionary_SchemaVersion_V0) {
        parcLog_Debug(athenaTransportLink_GetLogger(athenaTransportLink),
                      "sending deprecated version %d message\n", ccnxTlvDictionary_GetSchemaVersion(ccnxMetaMessage));
    }

    // Get a wire format buffer and write it out.
    PARCBuffer *wireFormatBuffer = athenaTransportLinkModule_CreateMessageBuffer(ccnxMetaMessage);
    parcBuffer_SetPosition(wireFormatBuffer, 0);
    size_t messageLength = parcBuffer_Limit(wireFormatBuffer);
    PARCBuffer *buffer = NULL;

    int fragmentNumber = 0;
    CCNxCodecEncodingBufferIOVec *ioFragment = NULL;

    // Get initial IO vector message or message fragment if we need, and have, fragmentation support.
    if (messageLength <= linkData->link.mtu) {
        buffer = parcBuffer_Acquire(wireFormatBuffer);
    } else {
        if (linkData->fragmenter != NULL) {
            ioFragment = athenaFragmenter_CreateFragment(linkData->fragmenter, wireFormatBuffer,
                                                         linkData->link.mtu, fragmentNumber);
            if (ioFragment) {
                buffer = _encodingBufferIOVecToPARCBuffer(ioFragment);
                fragmentNumber++;
            }
        } else { // message too big and no fragmenter provided
            parcBuffer_Release(&wireFormatBuffer);
            parcLog_Debug(athenaTransportLink_GetLogger(athenaTransportLink),
                          "message larger than mtu and no fragmention support (size=%d)", messageLength);
            errno = EMSGSIZE;
            return -1;
        }
    }

    int sendResult = 0;
    while (buffer) {
        sendResult = _sendBuffer(athenaTransportLink, buffer);
        parcBuffer_Release(&buffer);
        if (ioFragment) {
            ccnxCodecEncodingBufferIOVec_Release(&ioFragment);
        }
        if (sendResult == -1) {
            break;
        }
        if (messageLength > linkData->link.mtu) {
            ioFragment = athenaFragmenter_CreateFragment(linkData->fragmenter, wireFormatBuffer,
                                                         linkData->link.mtu, fragmentNumber);
            if (ioFragment) {
                buffer = _encodingBufferIOVecToPARCBuffer(ioFragment);
                fragmentNumber++;
            }
        }
    }

    parcBuffer_Release(&wireFormatBuffer);
    return 0;
}

static void
_UDPClose(AthenaTransportLink *athenaTransportLink)
{
    parcLog_Info(athenaTransportLink_GetLogger(athenaTransportLink),
                 "link %s closed", athenaTransportLink_GetName(athenaTransportLink));
    _UDPLinkData *linkData = athenaTransportLink_GetPrivateData(athenaTransportLink);
    close(linkData->fd);
    _UDPLinkData_Destroy(&linkData);
}

//
// Return a message which has been queued on this link from a UDP listener
//
static CCNxMetaMessage *
_UDPReceiveProxy(AthenaTransportLink *athenaTransportLink)
{
    CCNxMetaMessage *ccnxMetaMessage = NULL;
    struct _UDPLinkData *linkData = athenaTransportLink_GetPrivateData(athenaTransportLink);
    if (parcDeque_Size(linkData->queue) > 0) {
        ccnxMetaMessage = parcDeque_RemoveFirst(linkData->queue);
        if (parcDeque_Size(linkData->queue) > 0) { // if there's another message, post an event.
            athenaTransportLink_SetEvent(athenaTransportLink, AthenaTransportLinkEvent_Receive);
        }
    }
    return ccnxMetaMessage;
}

static void
_setConnectLinkState(AthenaTransportLink *athenaTransportLink, _UDPLinkData *linkData)
{
    athenaTransportLink_SetPrivateData(athenaTransportLink, linkData);

    // Register file descriptor to be polled.  This must be set before adding the link (case ???).
    athenaTransportLink_SetEventFd(athenaTransportLink, linkData->fd);

    // Determine and flag the link cost for forwarding messages.
    // Messages without sufficient hop count collateral will be dropped.
    // Local links will always be allowed to be taken (i.e. localhost).
    bool isLocal = false;
    if (linkData->link.peerAddress.ss_family == AF_INET) {
        if (((struct sockaddr_in *)&linkData->link.peerAddress)->sin_addr.s_addr ==
            ((struct sockaddr_in *)&linkData->link.myAddress)->sin_addr.s_addr)
            isLocal = true;
    } else if (linkData->link.peerAddress.ss_family == AF_INET6) {
        if (memcmp(((struct sockaddr_in6 *)&linkData->link.peerAddress)->sin6_addr.s6_addr,
                   ((struct sockaddr_in6 *)&linkData->link.myAddress)->sin6_addr.s6_addr, 16) == 0) {
            isLocal = true;
        }
    }
    athenaTransportLink_SetLocal(athenaTransportLink, isLocal);
}

static AthenaTransportLink *
_cloneNewLink(AthenaTransportLink *athenaTransportLink, struct sockaddr_storage *peerAddress)
{
    struct _UDPLinkData *linkData = athenaTransportLink_GetPrivateData(athenaTransportLink);
    _UDPLinkData *newLinkData = _UDPLinkData_Create();

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
    newLinkData->fd = dup(linkData->fd);
    newLinkData->queue = parcDeque_Create();
    assertNotNull(newLinkData->queue, "Could not create data queue for new link");

    memcpy(&newLinkData->link.myAddress, &linkData->link.myAddress, SOCKADDR_IN_LEN(&linkData->link.myAddress));

    memcpy(&newLinkData->link.peerAddress, peerAddress, SOCKADDR_IN_LEN(peerAddress));

    // Clone a new link from the current listener.
    const char *derivedLinkName = _createNameFromLinkData(&newLinkData->link);
    AthenaTransportLink *newTransportLink = athenaTransportLink_Clone(athenaTransportLink,
                                                                      derivedLinkName,
                                                                      _UDPSend,
                                                                      _UDPReceiveProxy,
                                                                      _UDPClose);
    if (newTransportLink == NULL) {
        parcLog_Error(athenaTransportLink_GetLogger(athenaTransportLink),
                      "athenaTransportLink_Clone failed");
        parcMemory_Deallocate(&derivedLinkName);
        _UDPLinkData_Destroy(&newLinkData);
        return NULL;
    }

    _setConnectLinkState(newTransportLink, newLinkData);

    // Send the new link up to be added.
    int result = athenaTransportLink_AddLink(athenaTransportLink, newTransportLink);
    if (result == -1) {
        parcLog_Error(athenaTransportLink_GetLogger(athenaTransportLink),
                      "athenaTransportLink_AddLink failed: %s", strerror(errno));
        _UDPLinkData_Destroy(&newLinkData);
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
    struct _UDPLinkData *linkData = athenaTransportLink_GetPrivateData(athenaTransportLink);
    parcDeque_Append(linkData->queue, ccnxMetaMessage);
    athenaTransportLink_SetEvent(athenaTransportLink, AthenaTransportLinkEvent_Receive);
}

static unsigned long
_hashAddress(struct sockaddr_storage *address)
{
    if (address->ss_family == AF_INET) {
        return ((unsigned long) ((struct sockaddr_in *)address)->sin_addr.s_addr << 32) |
                                ((struct sockaddr_in *)address)->sin_port;
    } else if (address->ss_family == AF_INET6) {
        uint64_t sin6addrHash = parcHash64_Data(&((struct sockaddr_in6 *)address)->sin6_addr.s6_addr, 16);
        return parcHash64_Data_Cumulative(&((struct sockaddr_in6 *)address)->sin6_port, sizeof(in_port_t), sin6addrHash);
    } else {
        assertTrue(0, "Unsupported address family %d\n", address->ss_family);
    }
    return 0;
}

static void
_demuxDelivery(AthenaTransportLink *athenaTransportLink, CCNxMetaMessage *ccnxMetaMessage, struct sockaddr_storage *peerAddress)
{
    struct _UDPLinkData *linkData = athenaTransportLink_GetPrivateData(athenaTransportLink);

    AthenaTransportLink *demuxLink = parcHashCodeTable_Get(linkData->multiplexTable, (void *) _hashAddress(peerAddress));

    // If it's an unknown peer, try to create a new link
    if (demuxLink == NULL) {
        demuxLink = _cloneNewLink(athenaTransportLink, peerAddress);
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

static void
_flushLink(AthenaTransportLink *athenaTransportLink)
{
    struct _UDPLinkData *linkData = athenaTransportLink_GetPrivateData(athenaTransportLink);
    char trash[MAXPATHLEN];

    // Flush link to attempt to resync our framing
    while (read(linkData->fd, trash, sizeof(trash)) == sizeof(trash)) {
        parcLog_Error(athenaTransportLink_GetLogger(athenaTransportLink), "... flushing link.");
    }
}

//
// Peek at the header and derive our total message length
//
static size_t
_messageLengthFromHeader(AthenaTransportLink *athenaTransportLink)
{
    struct _UDPLinkData *linkData = athenaTransportLink_GetPrivateData(athenaTransportLink);

    // Peek at our message header to determine the total length of buffer we need to allocate.
    size_t fixedHeaderLength = ccnxCodecTlvPacket_MinimalHeaderLength();
    PARCBuffer *wireFormatBuffer = parcBuffer_Allocate(fixedHeaderLength);
    const uint8_t *peekBuffer = parcBuffer_Overlay(wireFormatBuffer, 0);

    ssize_t readCount = recv(linkData->fd, (void *) peekBuffer, fixedHeaderLength, MSG_PEEK);

    if (readCount == -1) {
        parcBuffer_Release(&wireFormatBuffer);
        if ((errno == EAGAIN) || (errno == EINTR)) {
            parcLog_Debug(athenaTransportLink_GetLogger(athenaTransportLink), "recv retry (%s)", strerror(errno));
            linkData->_stats.receive_ReadRetry++;
        } else {
            linkData->_stats.receive_ReadError++;
            parcLog_Debug(athenaTransportLink_GetLogger(athenaTransportLink), "recv error (%s)", strerror(errno));
            athenaTransportLink_SetEvent(athenaTransportLink, AthenaTransportLinkEvent_Error);
        }
        return -1;
    }

    // A zero read means no data
    if (readCount == 0) {
        parcBuffer_Release(&wireFormatBuffer);
        return -1;
    }

    // Check for a short header read, since we're only peeking here we just return and retry later
    if (readCount != fixedHeaderLength) {
        linkData->_stats.receive_ReadHeaderFailure++;
        parcBuffer_Release(&wireFormatBuffer);
        return -1;
    }

    // Obtain the total size of the message from the header
    size_t messageLength = ccnxCodecTlvPacket_GetPacketLength(wireFormatBuffer);
    parcBuffer_Release(&wireFormatBuffer);

    // Could do more to check the integrity of the message and framing.
    // If length is greater than our MTU we will find out in the read.
    if (messageLength < fixedHeaderLength) {
        linkData->_stats.receive_BadMessageLength++;
        parcLog_Error(athenaTransportLink_GetLogger(athenaTransportLink),
                      "Framing error, length less than required header (%zu < %zu), flushing.",
                      messageLength, fixedHeaderLength);
        _flushLink(athenaTransportLink);
        return -1;
    }

    return messageLength;
}

//
// Receive a message from the specified link.
//
static CCNxMetaMessage *
_UDPReceiveMessage(AthenaTransportLink *athenaTransportLink, struct sockaddr_storage *peerAddress)
{
    struct _UDPLinkData *linkData = athenaTransportLink_GetPrivateData(athenaTransportLink);
    CCNxMetaMessage *ccnxMetaMessage = NULL;
    size_t messageLength;

    // If an MTU has been set, allocate a buffer of that size to avoid having to peek at the message,
    // othersize derive the link from the header and allocate a buffer based on the message size.

    if (linkData->link.mtu != 0) {
        messageLength = linkData->link.mtu;
    } else {
        messageLength = _messageLengthFromHeader(athenaTransportLink);
        if (messageLength <= 0) {
            return NULL;
        }
    }

    PARCBuffer *wireFormatBuffer = parcBuffer_Allocate(messageLength);

    char *buffer = parcBuffer_Overlay(wireFormatBuffer, 0);
    socklen_t peerAddressLength = sizeof(struct sockaddr_storage);
    ssize_t readCount = recvfrom(linkData->fd, buffer, messageLength, 0, (struct sockaddr *) peerAddress, &peerAddressLength);
    // Reset to the real expected message length from the header if we didn't already obtain it
    if (linkData->link.mtu != 0) {
        messageLength = ccnxCodecTlvPacket_GetPacketLength(wireFormatBuffer);
    }

    // On error mark the link to close or retry.
    if (readCount == -1) {
        parcBuffer_Release(&wireFormatBuffer);
        if ((errno == EAGAIN) || (errno == EINTR)) {
            linkData->_stats.receive_ReadRetry++;
            parcLog_Debug(athenaTransportLink_GetLogger(athenaTransportLink), "recv retry (%s)", strerror(errno));
        } else {
            linkData->_stats.receive_ReadError++;
            athenaTransportLink_SetEvent(athenaTransportLink, AthenaTransportLinkEvent_Error);
            parcLog_Debug(athenaTransportLink_GetLogger(athenaTransportLink), "read error (%s)", strerror(errno));
        }
        return NULL;
    }

    // A zero read means either no more data is currently available or our peer hungup.
    // Just return to retry as we'll detect EOF when we come back at the top of UDPReceive
    if (readCount == 0) {
        parcBuffer_Release(&wireFormatBuffer);
        return NULL;
    }

    // If it was it a short read just return to retry later.
    while (readCount < messageLength) {
        linkData->_stats.receive_ShortRead++;
        parcLog_Debug(athenaTransportLink_GetLogger(athenaTransportLink), "short read error (%s)", strerror(errno));
        parcBuffer_Release(&wireFormatBuffer);
        return NULL;
    }

    parcLog_Debug(athenaTransportLink_GetLogger(athenaTransportLink), "received message (size=%d)", readCount);
    parcBuffer_SetPosition(wireFormatBuffer, parcBuffer_Position(wireFormatBuffer) + readCount);
    parcBuffer_Flip(wireFormatBuffer);

    // If it's not a fragment returns our passed in wireFormatBuffer, otherwise it owns the buffer and eventually
    // passes back the aggregated message after receiving all its fragments, returning NULL in the mean time.
    wireFormatBuffer = athenaFragmenter_ReceiveFragment(linkData->fragmenter, wireFormatBuffer);

    if (wireFormatBuffer != NULL) {
        // Construct, and return a ccnxMetaMessage from the wire format buffer.
        ccnxMetaMessage = ccnxMetaMessage_CreateFromWireFormatBuffer(wireFormatBuffer);
        if (ccnxMetaMessage == NULL) {
            linkData->_stats.receive_DecodeFailed++;
            parcLog_Error(athenaTransportLink_GetLogger(athenaTransportLink), "Failed to decode message from received packet.");
        } else if (ccnxTlvDictionary_GetSchemaVersion(ccnxMetaMessage) == CCNxTlvDictionary_SchemaVersion_V0) {
            parcLog_Debug(athenaTransportLink_GetLogger(athenaTransportLink),
                          "received deprecated version %d message\n", ccnxTlvDictionary_GetSchemaVersion(ccnxMetaMessage));
        }
        parcBuffer_Release(&wireFormatBuffer);
    }

    return ccnxMetaMessage;
}

//
// Receive a message from a point to point UDP tunnel.
//
static CCNxMetaMessage *
_UDPReceive(AthenaTransportLink *athenaTransportLink)
{
    struct sockaddr_storage peerAddress; // Unused
    CCNxMetaMessage *ccnxMetaMessage = _UDPReceiveMessage(athenaTransportLink, &peerAddress);
    return ccnxMetaMessage;
}

//
// Receive and queue a message from a UDP listener.
//
static CCNxMetaMessage *
_UDPReceiveListener(AthenaTransportLink *athenaTransportLink)
{
    struct sockaddr_storage peerAddress;
    CCNxMetaMessage *ccnxMetaMessage = _UDPReceiveMessage(athenaTransportLink, &peerAddress);
    if (ccnxMetaMessage) {
        _demuxDelivery(athenaTransportLink, ccnxMetaMessage, &peerAddress);
    }
    return NULL;
}

static int
_setSocketOptions(AthenaTransportLinkModule *athenaTransportLinkModule, int fd)
{
#ifdef BSD_IGNORESIGPIPE
    int on = 1;
    int result = setsockopt(fd, SOL_SOCKET, SO_NOSIGPIPE, (void *) &on, sizeof(on));
    if (result) {
        parcLog_Error(athenaTransportLinkModule_GetLogger(athenaTransportLinkModule),
                      "setsockopt failed to set SO_NOSIGPIPE (%s)", strerror(errno));
        return -1;
    }
#endif
    return 0;
}

//
// Open a UDP point to point connection.
//
static AthenaTransportLink *
_UDPOpenConnection(AthenaTransportLinkModule *athenaTransportLinkModule, const char *linkName, struct sockaddr *source, struct sockaddr *destination, size_t mtu)
{
    const char *derivedLinkName;

    _UDPLinkData *linkData = _UDPLinkData_Create();

    memcpy(&linkData->link.peerAddress, destination, SOCKADDR_IN_LEN(destination));
    if (mtu) {
        linkData->link.mtu = mtu;
    }

    linkData->fd = socket(linkData->link.peerAddress.ss_family, SOCK_DGRAM, IPPROTO_UDP);
    if (linkData->fd < 0) {
        parcLog_Error(athenaTransportLinkModule_GetLogger(athenaTransportLinkModule), "socket error (%s)", strerror(errno));
        _UDPLinkData_Destroy(&linkData);
        return NULL;
    }

    int result = _setSocketOptions(athenaTransportLinkModule, linkData->fd);
    if (result) {
        close(linkData->fd);
        _UDPLinkData_Destroy(&linkData);
        return NULL;
    }

    // bind the local endpoint so we can know our allocated port if it was wildcarded
    if (source) {
        result = bind(linkData->fd, (struct sockaddr *) source, SOCKADDR_IN_LEN(source));
        if (result) {
            parcLog_Error(athenaTransportLinkModule_GetLogger(athenaTransportLinkModule),
                          "bind error (%s)", strerror(errno));
            close(linkData->fd);
            _UDPLinkData_Destroy(&linkData);
            return NULL;
        }
        memcpy(&linkData->link.myAddress, source, SOCKADDR_IN_LEN(source));
    }

    // Retrieve the local endpoint data, used to create the derived name.
    char myAddress[1024];
    socklen_t myAddressLength = 1024;
    result = getsockname(linkData->fd, (struct sockaddr *) &myAddress, &myAddressLength);
    if (result != 0) {
        parcLog_Error(athenaTransportLinkModule_GetLogger(athenaTransportLinkModule),
                      "Failed to obtain endpoint information from getsockname.");
        _UDPLinkData_Destroy(&linkData);
        return NULL;
    }
    memcpy(&linkData->link.myAddress, myAddress, myAddressLength);

    derivedLinkName = _createNameFromLinkData(&linkData->link);

    if (linkName == NULL) {
        linkName = derivedLinkName;
    }

    AthenaTransportLink *athenaTransportLink = athenaTransportLink_Create(linkName,
                                                                          _UDPSend,
                                                                          _UDPReceive,
                                                                          _UDPClose);
    if (athenaTransportLink == NULL) {
        parcLog_Error(athenaTransportLink_GetLogger(athenaTransportLink),
                      "athenaTransportLink_Create failed");
        parcMemory_Deallocate(&derivedLinkName);
        _UDPLinkData_Destroy(&linkData);
        return NULL;
    }

    athenaTransportLink_SetLogLevel(athenaTransportLink, parcLog_GetLevel(athenaTransportLinkModule_GetLogger(athenaTransportLinkModule)));
    _setConnectLinkState(athenaTransportLink, linkData);
    // Enable Send? XXX
    athenaTransportLink_SetEvent(athenaTransportLink, AthenaTransportLinkEvent_Send);

    parcLog_Info(athenaTransportLink_GetLogger(athenaTransportLink),
                 "new link established: Name=\"%s\" (%s)", linkName, derivedLinkName);

    parcMemory_Deallocate(&derivedLinkName);
    return athenaTransportLink;
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
_UDPOpenListener(AthenaTransportLinkModule *athenaTransportLinkModule, const char *linkName, struct sockaddr *destination, size_t mtu)
{
    const char *derivedLinkName;

    _UDPLinkData *linkData = _UDPLinkData_Create();
    linkData->multiplexTable = parcHashCodeTable_Create(_connectionEquals, _connectionHashCode, NULL, _closeConnection);
    assertNotNull(linkData->multiplexTable, "Could not create multiplex table for new listener");

    memcpy(&linkData->link.myAddress, destination, SOCKADDR_IN_LEN(destination));
    if (mtu) {
        linkData->link.mtu = mtu;
    }

    linkData->fd = socket(linkData->link.myAddress.ss_family, SOCK_DGRAM, IPPROTO_UDP);
    if (linkData->fd < 0) {
        parcLog_Error(athenaTransportLinkModule_GetLogger(athenaTransportLinkModule),
                      "socket error (%s)", strerror(errno));
        _UDPLinkData_Destroy(&linkData);
        return NULL;
    }

    int result = _setSocketOptions(athenaTransportLinkModule, linkData->fd);
    if (result) {
        close(linkData->fd);
        _UDPLinkData_Destroy(&linkData);
        return NULL;
    }

    // Set non-blocking flag
    int flags = fcntl(linkData->fd, F_GETFL, NULL);
    if (flags < 0) {
        parcLog_Error(athenaTransportLinkModule_GetLogger(athenaTransportLinkModule),
                      "fcntl failed to get non-blocking flag (%s)", strerror(errno));
        close(linkData->fd);
        _UDPLinkData_Destroy(&linkData);
        return NULL;
    }
    result = fcntl(linkData->fd, F_SETFL, flags | O_NONBLOCK);
    if (result) {
        parcLog_Error(athenaTransportLinkModule_GetLogger(athenaTransportLinkModule),
                      "fcntl failed to set non-blocking flag (%s)", strerror(errno));
        close(linkData->fd);
        _UDPLinkData_Destroy(&linkData);
        return NULL;
    }

    // bind to listen on requested address
    result = bind(linkData->fd, (struct sockaddr *)&linkData->link.myAddress, SOCKADDR_IN_LEN(&linkData->link.myAddress));
    if (result) {
        parcLog_Error(athenaTransportLinkModule_GetLogger(athenaTransportLinkModule),
                      "bind error (%s)", strerror(errno));
        close(linkData->fd);
        _UDPLinkData_Destroy(&linkData);
        return NULL;
    }

    derivedLinkName = _createNameFromLinkData(&linkData->link);

    if (linkName == NULL) {
        linkName = derivedLinkName;
    }

    // Listener doesn't require a send method.  The receive method is used to establish new connections.
    AthenaTransportLink *athenaTransportLink = athenaTransportLink_Create(linkName,
                                                                          NULL,
                                                                          _UDPReceiveListener,
                                                                          _UDPClose);
    if (athenaTransportLink == NULL) {
        parcLog_Error(athenaTransportLink_GetLogger(athenaTransportLink),
                      "athenaTransportLink_Create failed");
        parcMemory_Deallocate(&derivedLinkName);
        close(linkData->fd);
        _UDPLinkData_Destroy(&linkData);
        return athenaTransportLink;
    }

    athenaTransportLink_SetLogLevel(athenaTransportLink, parcLog_GetLevel(athenaTransportLinkModule_GetLogger(athenaTransportLinkModule)));
    athenaTransportLink_SetPrivateData(athenaTransportLink, linkData);
    athenaTransportLink_SetEventFd(athenaTransportLink, linkData->fd);

    // Links established for listening are not used to route messages.
    // They can be kept in a listener list that doesn't consume a linkId.
    athenaTransportLink_SetRoutable(athenaTransportLink, false);

    parcLog_Info(athenaTransportLink_GetLogger(athenaTransportLink),
                 "new listener established: Name=\"%s\" (%s)", linkName, derivedLinkName);

    parcMemory_Deallocate(&derivedLinkName);
    return athenaTransportLink;
}

#define UDP_LISTENER_FLAG "listener"
#define SRC_LINK_SPECIFIER "src%3D"
#define LINK_MTU_SIZE "mtu%3D"
#define LINK_NAME_SPECIFIER "name%3D"
#define LOCAL_LINK_FLAG "local%3D"
#define FRAGMENTER "fragmenter%3D"

typedef struct _URISpecificationParameters {
    char *linkName;
    struct sockaddr_storage *source;
    struct sockaddr_storage *destination;
    bool listener;
    size_t mtu;
    int forceLocal;
    char *fragmenterName;
} _URISpecificationParameters;

static struct sockaddr_storage *
_getSockaddr(const char *moduleName, const char *hostname, in_port_t port)
{
    struct sockaddr_storage *sockaddr = NULL;

    if (strcmp(moduleName, UDP_SCHEME) == 0) {
        char address[INET_ADDRSTRLEN];
        struct addrinfo hints = { .ai_family = AF_INET };
        struct addrinfo *ai;

        if (getaddrinfo(hostname, NULL, &hints, &ai) == 0) {
            // Convert given hostname to a canonical presentation
            inet_ntop(AF_INET, (void *)&((struct sockaddr_in *)ai->ai_addr)->sin_addr, address, INET_ADDRSTRLEN);
            freeaddrinfo(ai);
            sockaddr = (struct sockaddr_storage *)parcNetwork_SockInet4Address(address, port);
            return sockaddr;
        }
    } else if (strcmp(moduleName, UDP6_SCHEME) == 0) {
        char address[INET6_ADDRSTRLEN];
        struct addrinfo hints = { .ai_family = AF_INET6 };
        struct addrinfo *ai;

        if (hostname[0] == '[') {
            assertTrue(hostname[strlen(hostname) - 1] == ']', "Malformed IPv6 hostname");
            hostname = parcMemory_StringDuplicate(hostname + 1, strlen(hostname) - 2);
        } else {
            hostname = parcMemory_StringDuplicate(hostname, strlen(hostname));
        }
        if (getaddrinfo(hostname, NULL, &hints, &ai) == 0) {
            // Convert given hostname to a canonical presentation
            inet_ntop(AF_INET6, (void *)&((struct sockaddr_in6 *)ai->ai_addr)->sin6_addr, address, INET6_ADDRSTRLEN);
            freeaddrinfo(ai);
            parcMemory_Deallocate(&hostname);
            sockaddr = (struct sockaddr_storage *)parcNetwork_SockInet6Address(address, port,
                                                                               ((struct sockaddr_in6 *)ai->ai_addr)->sin6_flowinfo,
                                                                               ((struct sockaddr_in6 *)ai->ai_addr)->sin6_scope_id);
            return sockaddr;
        }
        parcMemory_Deallocate(&hostname);
    }

    return NULL;
}

static void
_URISpecificationParameters_Destroy(_URISpecificationParameters **parameters)
{
    if ((*parameters)->fragmenterName) {
        parcMemory_Deallocate(&((*parameters)->fragmenterName));
    }
    if ((*parameters)->linkName) {
        parcMemory_Deallocate(&((*parameters)->linkName));
    }
    if ((*parameters)->source) {
        parcMemory_Deallocate(&((*parameters)->source));
    }
    if ((*parameters)->destination) {
        parcMemory_Deallocate(&((*parameters)->destination));
    }
    parcMemory_Deallocate(parameters);
}

#include <parc/algol/parc_URIAuthority.h>

static _URISpecificationParameters *
_URISpecificationParameters_Create(AthenaTransportLinkModule *athenaTransportLinkModule, PARCURI *connectionURI)
{
    const char *moduleName = parcURI_GetScheme(connectionURI);
    _URISpecificationParameters *parameters = parcMemory_AllocateAndClear(sizeof(_URISpecificationParameters));

    const char *authorityString = parcURI_GetAuthority(connectionURI);
    if (authorityString == NULL) {
        parcLog_Error(athenaTransportLinkModule_GetLogger(athenaTransportLinkModule),
                      "Unable to parse connection authority %s", authorityString);
        errno = EINVAL;
        _URISpecificationParameters_Destroy(&parameters);
        return NULL;
    }
    PARCURIAuthority *authority = parcURIAuthority_Parse(authorityString);
    const char *hostname = parcURIAuthority_GetHostName(authority);
    in_port_t port = parcURIAuthority_GetPort(authority);
    if (port == 0) {
        parcLog_Error(athenaTransportLinkModule_GetLogger(athenaTransportLinkModule),
                      "Invalid address specification, port == 0");
        errno = EINVAL;
        _URISpecificationParameters_Destroy(&parameters);
        parcURIAuthority_Release(&authority);
        return NULL;
    }

    parameters->destination = _getSockaddr(moduleName, hostname, port);
    if (parameters->destination == NULL) {
        parcLog_Error(athenaTransportLinkModule_GetLogger(athenaTransportLinkModule),
                      "Unable to create sockaddr for %s", hostname);
        errno = EINVAL;
        _URISpecificationParameters_Destroy(&parameters);
        parcURIAuthority_Release(&authority);
        return NULL;
    }
    parcURIAuthority_Release(&authority);

    PARCURIPath *remainder = parcURI_GetPath(connectionURI);
    size_t segments = parcURIPath_Count(remainder);
    for (int i = 0; i < segments; i++) {
        PARCURISegment *segment = parcURIPath_Get(remainder, i);
        const char *token = parcURISegment_ToString(segment);

        if (strcasecmp(token, UDP_LISTENER_FLAG) == 0) {
            parameters->listener = true;
            parcMemory_Deallocate(&token);
            continue;
        }

        if (strncasecmp(token, SRC_LINK_SPECIFIER, strlen(SRC_LINK_SPECIFIER)) == 0) {
            char srcAddress[MAXPATHLEN];
            in_port_t srcPort;
            if (sscanf(token, "%*[^%%]%%3D%[^%%]%%3A%hd", (char *)srcAddress, &srcPort) != 2) {
                parcLog_Error(athenaTransportLinkModule_GetLogger(athenaTransportLinkModule),
                              "Improper connection source specification (%s)", token);
                parcMemory_Deallocate(&token);
                _URISpecificationParameters_Destroy(&parameters);
                errno = EINVAL;
                return NULL;
            }
            parameters->source = _getSockaddr(moduleName, srcAddress, srcPort);
            if (parameters->source == NULL) {
                parcLog_Error(athenaTransportLinkModule_GetLogger(athenaTransportLinkModule),
                              "Unable to create sockaddr for %s", hostname);
                errno = EINVAL;
                parcMemory_Deallocate(&token);
                _URISpecificationParameters_Destroy(&parameters);
                return NULL;
            }
            parcMemory_Deallocate(&token);
            continue;
        }

        if (strncasecmp(token, FRAGMENTER, strlen(FRAGMENTER)) == 0) {
            char specifiedFragmenterName[MAXPATHLEN];
            if (sscanf(token, "%*[^%%]%%3D%s", specifiedFragmenterName) != 1) {
                parcLog_Error(athenaTransportLinkModule_GetLogger(athenaTransportLinkModule),
                              "Improper fragmenter name specification (%s)", token);
                parcMemory_Deallocate(&token);
                _URISpecificationParameters_Destroy(&parameters);
                errno = EINVAL;
                return NULL;
            }
            parameters->fragmenterName = parcMemory_StringDuplicate(specifiedFragmenterName, strlen(specifiedFragmenterName));
            parcMemory_Deallocate(&token);
            continue;
        }

        if (strncasecmp(token, LINK_MTU_SIZE, strlen(LINK_MTU_SIZE)) == 0) {
            if (sscanf(token, "%*[^%%]%%3D%zd", &parameters->mtu) != 1) {
                parcLog_Error(athenaTransportLinkModule_GetLogger(athenaTransportLinkModule),
                              "Improper MTU specification (%s)", token);
                parcMemory_Deallocate(&token);
                _URISpecificationParameters_Destroy(&parameters);
                errno = EINVAL;
                return NULL;
            }
            parcMemory_Deallocate(&token);
            continue;
        }

        if (strncasecmp(token, LINK_NAME_SPECIFIER, strlen(LINK_NAME_SPECIFIER)) == 0) {
            char specifiedLinkName[MAXPATHLEN];
            if (sscanf(token, "%*[^%%]%%3D%s", specifiedLinkName) != 1) {
                parcLog_Error(athenaTransportLinkModule_GetLogger(athenaTransportLinkModule),
                              "Improper connection name specification (%s)", token);
                parcMemory_Deallocate(&token);
                _URISpecificationParameters_Destroy(&parameters);
                errno = EINVAL;
                return NULL;
            }
            parameters->linkName = parcMemory_StringDuplicate(specifiedLinkName, strlen(specifiedLinkName));
            parcMemory_Deallocate(&token);
            continue;
        }

        if (strncasecmp(token, LOCAL_LINK_FLAG, strlen(LOCAL_LINK_FLAG)) == 0) {
            char localFlag[MAXPATHLEN] = { 0 };
            if (sscanf(token, "%*[^%%]%%3D%s", localFlag) != 1) {
                parameters->forceLocal = 0;
                parcLog_Error(athenaTransportLinkModule_GetLogger(athenaTransportLinkModule),
                              "Improper local specification (%s)", token);
                parcMemory_Deallocate(&token);
                _URISpecificationParameters_Destroy(&parameters);
                errno = EINVAL;
                return NULL;
            } else if (strncasecmp(localFlag, "false", strlen("false")) == 0) {
                parameters->forceLocal = AthenaTransportLink_ForcedNonLocal;
            } else if (strncasecmp(localFlag, "true", strlen("true")) == 0) {
                parameters->forceLocal = AthenaTransportLink_ForcedLocal;
            } else {
                parcMemory_Deallocate(&token);
                _URISpecificationParameters_Destroy(&parameters);
                errno = EINVAL;
                return NULL;
            }
            parcMemory_Deallocate(&token);
            continue;
        }

        parcLog_Error(athenaTransportLinkModule_GetLogger(athenaTransportLinkModule),
                      "Unknown connection parameter (%s)", token);
        parcMemory_Deallocate(&token);
        _URISpecificationParameters_Destroy(&parameters);
        errno = EINVAL;
        return NULL;
    }

    return parameters;
}

static AthenaTransportLink *
_UDPOpen(AthenaTransportLinkModule *athenaTransportLinkModule, PARCURI *connectionURI)
{
    AthenaTransportLink *result = 0;

    _URISpecificationParameters *parameters = _URISpecificationParameters_Create(athenaTransportLinkModule, connectionURI);
    if (parameters == NULL) {
        parcLog_Error(athenaTransportLinkModule_GetLogger(athenaTransportLinkModule),
                      "Unable to parse connection URI specification (%s)", connectionURI);
        errno = EINVAL;
        return NULL;
    }

    if (parameters->listener) {
        result = _UDPOpenListener(athenaTransportLinkModule, parameters->linkName,
                                  (struct sockaddr *)parameters->destination, parameters->mtu);
    } else {
        result = _UDPOpenConnection(athenaTransportLinkModule, parameters->linkName,
                                  (struct sockaddr *)parameters->source,
                                  (struct sockaddr *)parameters->destination, parameters->mtu);
    }

    if (result && parameters->fragmenterName) {
        struct _UDPLinkData *linkData = athenaTransportLink_GetPrivateData(result);
        linkData->fragmenter = athenaFragmenter_Create(result, parameters->fragmenterName);
        if (linkData->fragmenter == NULL) {
            parcLog_Error(athenaTransportLinkModule_GetLogger(athenaTransportLinkModule),
                          "Failed to open/initialize %s fragmenter for %s: %s", parameters->fragmenterName, parameters->linkName, strerror(errno));
            athenaTransportLink_Close(result);
            _URISpecificationParameters_Destroy(&parameters);
            return NULL;
        }
    }

    // forced IsLocal/IsNotLocal, mainly for testing
    if (result && parameters->forceLocal) {
        athenaTransportLink_ForceLocal(result, parameters->forceLocal);
    }

    _URISpecificationParameters_Destroy(&parameters);

    return result;
}

static int
_UDPPoll(AthenaTransportLink *athenaTransportLink, int timeout)
{
    struct _UDPLinkData *linkData = athenaTransportLink_GetPrivateData(athenaTransportLink);

    if (linkData->queue) {
        return (int) parcDeque_Size(linkData->queue);
    }
    return 0;
}

PARCArrayList *
athenaTransportLinkModuleUDP_Init()
{
    // Module for providing UDP tunnel connections.
    AthenaTransportLinkModule *athenaTransportLinkModule;
    PARCArrayList *moduleList = parcArrayList_Create(NULL);
    assertNotNull(moduleList, "parcArrayList_Create failed to create module list");

    athenaTransportLinkModule = athenaTransportLinkModule_Create("UDP",
                                                                 _UDPOpen,
                                                                 _UDPPoll);
    assertNotNull(athenaTransportLinkModule, "parcMemory_AllocateAndClear failed allocate UDP athenaTransportLinkModule");
    bool result = parcArrayList_Add(moduleList, athenaTransportLinkModule);
    assertTrue(result == true, "parcArrayList_Add failed");

    athenaTransportLinkModule = athenaTransportLinkModule_Create("UDP6",
                                                                 _UDPOpen,
                                                                 _UDPPoll);
    assertNotNull(athenaTransportLinkModule, "parcMemory_AllocateAndClear failed allocate UDP athenaTransportLinkModule");
    result = parcArrayList_Add(moduleList, athenaTransportLinkModule);
    assertTrue(result == true, "parcArrayList_Add failed");

    return moduleList;
}

PARCArrayList *
athenaTransportLinkModuleUDP6_Init()
{
    return athenaTransportLinkModuleUDP_Init();
}

void
athenaTransportLinkModuleUDP_Fini()
{
}

void
athenaTransportLinkModuleUDP6_Fini()
{
}
