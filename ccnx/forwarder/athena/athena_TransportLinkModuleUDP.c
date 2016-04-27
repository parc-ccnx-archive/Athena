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
    struct sockaddr_in myAddress;
    socklen_t myAddressLength;
    struct sockaddr_in peerAddress;
    socklen_t peerAddressLength;
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
 * @abstract create link name based on file descriptor
 * @discussion
 *
 * @param [in] type of connection
 * @param [in] fd file descriptor
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
_createNameFromLinkData(const _connectionPair *linkData)
{
    char nameBuffer[MAXPATHLEN];
    const char *protocol = "udp";

    // Get our local hostname and port
    char myHost[NI_MAXHOST], myPort[NI_MAXSERV];
    int myResult = getnameinfo((struct sockaddr *) &linkData->myAddress, linkData->myAddressLength,
                               myHost, NI_MAXHOST, myPort, NI_MAXSERV, NI_NUMERICSERV);

    // Get our peer's hostname and port
    char peerHost[NI_MAXHOST], peerPort[NI_MAXSERV];
    int peerResult = getnameinfo((struct sockaddr *) &linkData->peerAddress, linkData->peerAddressLength,
                                 peerHost, NI_MAXHOST, peerPort, NI_MAXSERV, NI_NUMERICSERV);

    if ((peerResult == 0) && (myResult == 0)) { // point to point connection
        sprintf(nameBuffer, "%s://%s:%s<->%s:%s", protocol, myHost, myPort, peerHost, peerPort);
    } else if (myResult == 0) { // listener only
        sprintf(nameBuffer, "%s://%s:%s", protocol, myHost, myPort);
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
                        (struct sockaddr *) &linkData->link.peerAddress, linkData->link.peerAddressLength);
#else
    writeCount = sendto(linkData->fd, buffer, length, 0,
                        (struct sockaddr *) &linkData->link.peerAddress, linkData->link.peerAddressLength);
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
    PARCBuffer *wireFormatBuffer = athenaTransportLinkModule_GetMessageBuffer(ccnxMetaMessage);
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
    if (linkData->link.peerAddress.sin_addr.s_addr == linkData->link.myAddress.sin_addr.s_addr) { // a local connection
        isLocal = true;
    }
    athenaTransportLink_SetLocal(athenaTransportLink, isLocal);
}

static AthenaTransportLink *
_newLink(AthenaTransportLink *athenaTransportLink, _UDPLinkData *newLinkData)
{
    // Accept a new tunnel connection.

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
_hashAddress(struct sockaddr_in *address)
{
    return ((unsigned long) address->sin_addr.s_addr << 32) | address->sin_port;
}

static void
_demuxDelivery(AthenaTransportLink *athenaTransportLink, CCNxMetaMessage *ccnxMetaMessage, struct sockaddr_in *peerAddress, socklen_t peerAddressLength)
{
    struct _UDPLinkData *linkData = athenaTransportLink_GetPrivateData(athenaTransportLink);

    AthenaTransportLink *demuxLink = parcHashCodeTable_Get(linkData->multiplexTable, (void *) _hashAddress(peerAddress));

    // If it's an unknown peer, try to create a new link
    if (demuxLink == NULL) {
        _UDPLinkData *newLinkData = _UDPLinkData_Create();

        // We use our parents fd to send, and receive demux'd messages from our parent on our queue
        newLinkData->fd = dup(linkData->fd);
        newLinkData->queue = parcDeque_Create();
        assertNotNull(newLinkData->queue, "Could not create data queue for new link");

        newLinkData->link.myAddressLength = linkData->link.myAddressLength;
        memcpy(&newLinkData->link.myAddress, &linkData->link.myAddress, linkData->link.myAddressLength);

        newLinkData->link.peerAddressLength = peerAddressLength;
        memcpy(&newLinkData->link.peerAddress, peerAddress, peerAddressLength);

        demuxLink = _newLink(athenaTransportLink, newLinkData);
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
// Peek at the header and derive our total message length
//
static size_t
_messageLengthFromHeader(AthenaTransportLink *athenaTransportLink, _UDPLinkData *linkData)
{
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
        parcLog_Error(athenaTransportLink_GetLogger(athenaTransportLink), "Framing error, flushing link.");
        char trash[MAXPATHLEN];
        // Flush link to attempt to resync our framing
        while (read(linkData->fd, trash, sizeof(trash)) == sizeof(trash)) {
            parcLog_Error(athenaTransportLink_GetLogger(athenaTransportLink), "... flushing link.");
        }
        return -1;
    }

    return messageLength;
}

//
// Receive a message from the specified link.
//
static CCNxMetaMessage *
_UDPReceiveMessage(AthenaTransportLink *athenaTransportLink, struct sockaddr_in *peerAddress, socklen_t *peerAddressLength)
{
    struct _UDPLinkData *linkData = athenaTransportLink_GetPrivateData(athenaTransportLink);
    CCNxMetaMessage *ccnxMetaMessage = NULL;
    size_t messageLength;

    // If an MTU has been set, allocate a buffer of that size to avoid having to peek at the message,
    // othersize derive the link from the header and allocate a buffer based on the message size.

    if (linkData->link.mtu != 0) {
        messageLength = linkData->link.mtu;
    } else {
        messageLength = _messageLengthFromHeader(athenaTransportLink, linkData);
        if (messageLength <= 0) {
            return NULL;
        }
    }

    PARCBuffer *wireFormatBuffer = parcBuffer_Allocate(messageLength);

    char *buffer = parcBuffer_Overlay(wireFormatBuffer, 0);
    *peerAddressLength = (socklen_t) sizeof(struct sockaddr_in);
    ssize_t readCount = recvfrom(linkData->fd, buffer, messageLength, 0, (struct sockaddr *) peerAddress, peerAddressLength);
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
    struct sockaddr_in peerAddress; // Unused
    socklen_t peerAddressLength; // Unused
    CCNxMetaMessage *ccnxMetaMessage = _UDPReceiveMessage(athenaTransportLink, &peerAddress, &peerAddressLength);
    return ccnxMetaMessage;
}

//
// Receive and queue a message from a UDP listener.
//
static CCNxMetaMessage *
_UDPReceiveListener(AthenaTransportLink *athenaTransportLink)
{
    struct sockaddr_in peerAddress;
    socklen_t peerAddressLength;
    CCNxMetaMessage *ccnxMetaMessage = _UDPReceiveMessage(athenaTransportLink, &peerAddress, &peerAddressLength);
    if (ccnxMetaMessage) {
        _demuxDelivery(athenaTransportLink, ccnxMetaMessage, &peerAddress, peerAddressLength);
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
_UDPOpenConnection(AthenaTransportLinkModule *athenaTransportLinkModule, const char *linkName, struct sockaddr_in *source, struct sockaddr_in *destination, size_t mtu)
{
    const char *derivedLinkName;

    _UDPLinkData *linkData = _UDPLinkData_Create();

    linkData->link.peerAddress = *((struct sockaddr_in *) destination);
    linkData->link.peerAddressLength = sizeof(struct sockaddr_in);
    if (mtu) {
        linkData->link.mtu = mtu;
    }

    linkData->fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
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
    result = bind(linkData->fd, (struct sockaddr *) source, sizeof(struct sockaddr_in));
    if (result) {
        parcLog_Error(athenaTransportLinkModule_GetLogger(athenaTransportLinkModule),
                      "bind error (%s)", strerror(errno));
        close(linkData->fd);
        _UDPLinkData_Destroy(&linkData);
        return NULL;
    }

    // Retrieve the local endpoint data, used to create the derived name.
    linkData->link.myAddressLength = sizeof(struct sockaddr_in);
    result = getsockname(linkData->fd, (struct sockaddr *) &linkData->link.myAddress, &linkData->link.myAddressLength);
    if (result != 0) {
        parcLog_Error(athenaTransportLinkModule_GetLogger(athenaTransportLinkModule),
                      "Failed to obtain endpoint information from getsockname.");
        _UDPLinkData_Destroy(&linkData);
        return NULL;
    }

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

    _setConnectLinkState(athenaTransportLink, linkData);
    // Enable Send? XXX
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
_UDPOpenListener(AthenaTransportLinkModule *athenaTransportLinkModule, const char *linkName, struct sockaddr_in *destination, size_t mtu)
{
    const char *derivedLinkName;

    _UDPLinkData *linkData = _UDPLinkData_Create();
    linkData->multiplexTable = parcHashCodeTable_Create(_connectionEquals, _connectionHashCode, NULL, _closeConnection);
    assertNotNull(linkData->multiplexTable, "Could not create multiplex table for new listener");

    linkData->link.myAddress = *destination;
    linkData->link.myAddressLength = sizeof(struct sockaddr_in);
    if (mtu) {
        linkData->link.mtu = mtu;
    }

    linkData->fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
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
    result = bind(linkData->fd, (struct sockaddr *) &linkData->link.myAddress, linkData->link.myAddressLength);
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
_parseSrc(const char *token, char *srcAddress, uint16_t *srcPort)
{
    if (sscanf(token, "%*[^%%]%%3D%[^%%]%%3A%hd", srcAddress, srcPort) != 2) {
        return -1;
    }
    // Normalize the provided hostname
    struct sockaddr_in *addr = (struct sockaddr_in *) parcNetwork_SockAddress(srcAddress, *srcPort);
    char *hostname = inet_ntoa(addr->sin_addr);
    parcMemory_Deallocate(&addr);

    memcpy(srcAddress, hostname, strlen(hostname) + 1);
    return 0;
}

#include <parc/algol/parc_URIAuthority.h>

static AthenaTransportLink *
_UDPOpen(AthenaTransportLinkModule *athenaTransportLinkModule, PARCURI *connectionURI)
{
    AthenaTransportLink *result = 0;

    const char *authorityString = parcURI_GetAuthority(connectionURI);
    if (authorityString == NULL) {
        parcLog_Error(athenaTransportLinkModule_GetLogger(athenaTransportLinkModule),
                      "Unable to parse connection authority %s", authorityString);
        errno = EINVAL;
        return NULL;
    }
    PARCURIAuthority *authority = parcURIAuthority_Parse(authorityString);
    const char *URIAddress = parcURIAuthority_GetHostName(authority);
    in_port_t port = parcURIAuthority_GetPort(authority);

    // Normalize the provided hostname
    struct sockaddr_in *addr = (struct sockaddr_in *) parcNetwork_SockAddress(URIAddress, port);
    char *address = inet_ntoa(addr->sin_addr);
    parcMemory_Deallocate(&addr);

    parcURIAuthority_Release(&authority);

    if (address == NULL) {
        parcLog_Error(athenaTransportLinkModule_GetLogger(athenaTransportLinkModule),
                      "Unable to lookup hostname %s", address);
        errno = EINVAL;
        return NULL;
    }
    if (port == 0) {
        parcLog_Error(athenaTransportLinkModule_GetLogger(athenaTransportLinkModule),
                      "Invalid address specification, port == 0");
        errno = EINVAL;
        return NULL;
    }

    bool listener = false;
    char srcAddress[NI_MAXHOST] = "0.0.0.0";
    size_t mtu = 0;
    uint16_t srcPort = 0;
    int forceLocal = 0;
    char *linkName = NULL;
    char specifiedLinkName[MAXPATHLEN] = { 0 };
    char *fragmenterName = NULL;
    char specifiedFragmenterName[MAXPATHLEN] = { 0 };

    PARCURIPath *remainder = parcURI_GetPath(connectionURI);
    size_t segments = parcURIPath_Count(remainder);
    for (int i = 0; i < segments; i++) {
        PARCURISegment *segment = parcURIPath_Get(remainder, i);
        const char *token = parcURISegment_ToString(segment);

        if (strcasecmp(token, UDP_LISTENER_FLAG) == 0) {
            listener = true;
            parcMemory_Deallocate(&token);
            continue;
        }

        if (strncasecmp(token, SRC_LINK_SPECIFIER, strlen(SRC_LINK_SPECIFIER)) == 0) {
            if (_parseSrc(token, srcAddress, &srcPort) != 0) {
                parcLog_Error(athenaTransportLinkModule_GetLogger(athenaTransportLinkModule),
                              "Improper connection source specification (%s)", token);
                parcMemory_Deallocate(&token);
                errno = EINVAL;
                return NULL;
            }
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

    struct sockaddr_in *destination = parcNetwork_SockInet4Address(address, port);
    struct sockaddr_in *source = parcNetwork_SockInet4Address(srcAddress, srcPort);

    if (listener) {
        result = _UDPOpenListener(athenaTransportLinkModule, linkName, destination, mtu);
    } else {
        result = _UDPOpenConnection(athenaTransportLinkModule, linkName, source, destination, mtu);
    }

    parcMemory_Deallocate(&destination);
    parcMemory_Deallocate(&source);

    if (result && fragmenterName) {
        struct _UDPLinkData *linkData = athenaTransportLink_GetPrivateData(result);
        linkData->fragmenter = athenaFragmenter_Create(result, fragmenterName);
        if (linkData->fragmenter == NULL) {
            parcLog_Error(athenaTransportLinkModule_GetLogger(athenaTransportLinkModule),
                          "Failed to open/initialize %s fragmenter for %s", fragmenterName, linkName);
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

    return moduleList;
}

void
athenaTransportLinkModuleUDP_Fini()
{
}
