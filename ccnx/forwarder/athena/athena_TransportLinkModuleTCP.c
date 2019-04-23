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
#include <stdio.h>
#include <unistd.h>
#include <sys/param.h>
#include <arpa/inet.h>

#include <parc/algol/parc_Network.h>
#include <ccnx/forwarder/athena/athena_TransportLinkModule.h>

#include <ccnx/common/codec/ccnxCodec_TlvPacket.h>
#include <ccnx/common/ccnx_WireFormatMessage.h>

static int _listenerBacklog = 16;

#define TCP_SCHEME "tcp"
#define TCP6_SCHEME "tcp6"

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
// Private data for each link instance
//
typedef struct _TCPLinkData {
    int fd;
    struct sockaddr_storage myAddress;
    struct sockaddr_storage peerAddress;
    struct {
        size_t receive_ReadHeaderFailure;
        size_t receive_BadMessageLength;
        size_t receive_ReadError;
        size_t receive_ReadRetry;
        size_t receive_ReadWouldBlock;
        size_t receive_PollRetry;
        size_t receive_ShortRead;
        size_t receive_DecodeFailed;
        size_t send_ShortWrite;
        size_t send_Retry;
        size_t send_Error;
    } _stats;
} _TCPLinkData;

static _TCPLinkData *
_TCPLinkData_Create()
{
    _TCPLinkData *linkData = parcMemory_AllocateAndClear(sizeof(_TCPLinkData));
    assertNotNull(linkData, "Could not create private data for new link");
    return linkData;
}

static void
_TCPLinkData_Destroy(_TCPLinkData **linkData)
{
    parcMemory_Deallocate(linkData);
}

#ifdef SIN6_LEN
#define SOCKADDR_IN_LEN(s) (socklen_t)(((struct sockaddr *)s)->sa_len)
#else
#define SOCKADDR_IN_LEN(s) (socklen_t)((((struct sockaddr_in *)s)->sin_family == AF_INET) ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6))
#endif

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
static const char *
_createNameFromLinkData(const _TCPLinkData *linkData)
{
    char nameBuffer[MAXPATHLEN];
    const char *protocol;

    // Get our local hostname and port
    char myHost[NI_MAXHOST], myPort[NI_MAXSERV];
    int myResult = getnameinfo((struct sockaddr *) &linkData->myAddress, SOCKADDR_IN_LEN(&linkData->myAddress),
                               myHost, NI_MAXHOST, myPort, NI_MAXSERV, NI_NUMERICSERV);

    // Get our peer's hostname and port
    char peerHost[NI_MAXHOST], peerPort[NI_MAXSERV];
    int peerResult = getnameinfo((struct sockaddr *) &linkData->peerAddress, SOCKADDR_IN_LEN(&linkData->peerAddress),
                                 peerHost, NI_MAXHOST, peerPort, NI_MAXSERV, NI_NUMERICSERV);

    protocol = (linkData->myAddress.ss_family == AF_INET6) ? TCP6_SCHEME : TCP_SCHEME;
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

static ssize_t
_writeLink(AthenaTransportLink *athenaTransportLink, void *buffer, size_t length)
{
    struct _TCPLinkData *linkData = athenaTransportLink_GetPrivateData(athenaTransportLink);

#ifdef LINUX_IGNORESIGPIPE
    int count = send(linkData->fd, buffer, length, MSG_NOSIGNAL);
#else
    ssize_t count = write(linkData->fd, buffer, length);
#endif

    if (count <= 0) { // on error close the link, else return to retry a zero write
        if (count == -1) {
            if ((errno == EAGAIN) || (errno == EINTR)) {
                parcLog_Debug(athenaTransportLink_GetLogger(athenaTransportLink), "send retry (%s)", strerror(errno));
                linkData->_stats.send_Retry++;
            } else {
                athenaTransportLink_SetEvent(athenaTransportLink, AthenaTransportLinkEvent_Error);
                linkData->_stats.send_Error++;
                parcLog_Error(athenaTransportLink_GetLogger(athenaTransportLink),
                              "send error, closing link (%s)", strerror(errno));
            }
        } else {
            linkData->_stats.send_ShortWrite++;
            parcLog_Debug(athenaTransportLink_GetLogger(athenaTransportLink), "short write");
        }
        return -1;
    }
    return count;
}

static int
_TCPSend(AthenaTransportLink *athenaTransportLink, CCNxMetaMessage *ccnxMetaMessage)
{
    if (ccnxTlvDictionary_GetSchemaVersion(ccnxMetaMessage) == CCNxTlvDictionary_SchemaVersion_V0) {
        parcLog_Warning(athenaTransportLink_GetLogger(athenaTransportLink),
                        "sending deprecated version %d message\n", ccnxTlvDictionary_GetSchemaVersion(ccnxMetaMessage));
    }

    // Get wire format and write it out.
    PARCBuffer *wireFormatBuffer = athenaTransportLinkModule_CreateMessageBuffer(ccnxMetaMessage);

    parcBuffer_SetPosition(wireFormatBuffer, 0);
    size_t length = parcBuffer_Limit(wireFormatBuffer);
    char *buffer = parcBuffer_Overlay(wireFormatBuffer, length);

    parcLog_Debug(athenaTransportLink_GetLogger(athenaTransportLink),
                  "sending message (size=%d)", length);

    int writeCount = 0;
    // If a short write, attempt to write the remainder of the message
    while (writeCount < length) {
        ssize_t count = _writeLink(athenaTransportLink, &buffer[writeCount], length - writeCount);
        if (count <= 0) {
            parcBuffer_Release(&wireFormatBuffer);
            return -1;
        }
        writeCount += count;
    }

    parcBuffer_Release(&wireFormatBuffer);
    return 0;
}

static bool
_linkIsEOF(AthenaTransportLink *athenaTransportLink)
{
    struct _TCPLinkData *linkData = athenaTransportLink_GetPrivateData(athenaTransportLink);

    // If poll indicates there's a read event and a subsequent read returns zero our peer has hungup.
    struct pollfd pollfd = { .fd = linkData->fd, .events = POLLIN };
    int events = poll(&pollfd, 1, 0);
    if (events == -1) {
        if ((errno == EAGAIN) || (errno == EINTR)) {
            linkData->_stats.receive_PollRetry++;
            parcLog_Info(athenaTransportLink_GetLogger(athenaTransportLink), "poll retry (%s)", strerror(errno));
            return false;
        }
        parcLog_Error(athenaTransportLink_GetLogger(athenaTransportLink), "poll error (%s)", strerror(errno));
        return true; // poll error, close the link
    } else if (events == 0) {
        // there are no pending events, was truly a zero read
        return false;
    }
    if (pollfd.revents & POLLIN) {
        char peekBuffer;
        ssize_t readCount = recv(linkData->fd, (void *) &peekBuffer, 1, MSG_PEEK);
        if (readCount == -1) {
            if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) { // read blocked
                linkData->_stats.receive_ReadWouldBlock++;
                return false;
            }
            return true; // read error
        }
        if (readCount == 0) { // EOF
            return true;
        }
    }
    return false;
}

static ssize_t
_readLink(AthenaTransportLink *athenaTransportLink, void *buffer, size_t length)
{
    struct _TCPLinkData *linkData = athenaTransportLink_GetPrivateData(athenaTransportLink);
    ssize_t readCount = read(linkData->fd, buffer, length);

    if (readCount == -1) {
        if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) { // read blocked
            linkData->_stats.receive_ReadWouldBlock++;
        } else {
            linkData->_stats.receive_ReadError++;
            athenaTransportLink_SetEvent(athenaTransportLink, AthenaTransportLinkEvent_Error);
        }
        parcLog_Debug(athenaTransportLink_GetLogger(athenaTransportLink), "read error (%s)", strerror(errno));
    }
    return readCount;
}

static void
_flushLink(AthenaTransportLink *athenaTransportLink)
{
    char trash[MAXPATHLEN];

    // Flush link to attempt to resync our framing
    while (_readLink(athenaTransportLink, trash, sizeof(trash)) == sizeof(trash)) {
        parcLog_Error(athenaTransportLink_GetLogger(athenaTransportLink), "... flushing link.");
    }
}

static CCNxMetaMessage *
_TCPReceive(AthenaTransportLink *athenaTransportLink)
{
    struct _TCPLinkData *linkData = athenaTransportLink_GetPrivateData(athenaTransportLink);
    CCNxMetaMessage *ccnxMetaMessage = NULL;

    // Read our message header to determine the total buffer length we need to allocate.
    size_t fixedHeaderLength = ccnxCodecTlvPacket_MinimalHeaderLength();
    PARCBuffer *wireFormatBuffer = parcBuffer_Allocate(fixedHeaderLength);
    const uint8_t *messageHeader = parcBuffer_Overlay(wireFormatBuffer, 0);

    ssize_t readCount = _readLink(athenaTransportLink, (void *) messageHeader, fixedHeaderLength);

    if (readCount == -1) {
        parcBuffer_Release(&wireFormatBuffer);
        return NULL;
    }

    // A zero read means either no more data is available or our peer has hungup.
    if (readCount == 0) {
        if (_linkIsEOF(athenaTransportLink)) {
            athenaTransportLink_SetEvent(athenaTransportLink, AthenaTransportLinkEvent_Error);
        } else {
            // A zero read, try again later
        }
        parcBuffer_Release(&wireFormatBuffer);
        return NULL;
    }

    // If it was a short read, attempt to read the remainder of the header
    while (readCount < fixedHeaderLength) {
        ssize_t count = _readLink(athenaTransportLink, (void *) &messageHeader[readCount], fixedHeaderLength - readCount);
        if (count == -1) {
            parcBuffer_Release(&wireFormatBuffer);
            return NULL;
        }
        if (count == 0) { // on error or zero read, return to check at the top of TCPReceive for EOF
            linkData->_stats.receive_ReadHeaderFailure++;
            parcLog_Debug(athenaTransportLink_GetLogger(athenaTransportLink), "short read error (%s)", strerror(errno));
            parcBuffer_Release(&wireFormatBuffer);
            return NULL;
        }
        readCount += count;
    }

    // Obtain the total size of the message from the header
    size_t messageLength = ccnxCodecTlvPacket_GetPacketLength(wireFormatBuffer);

    // Today, if the length is bad we flush the link and return.
    // Could do more to check the integrity of the message and framing.
    if (messageLength < fixedHeaderLength) {
        linkData->_stats.receive_BadMessageLength++;
        parcLog_Error(athenaTransportLink_GetLogger(athenaTransportLink),
                      "Framing error, length less than required header (%zu < %zu), flushing.",
                      messageLength, fixedHeaderLength);
        _flushLink(athenaTransportLink);
        parcBuffer_Release(&wireFormatBuffer);
        return NULL;
    }

    // Allocate the remainder of message buffer and read a message into it.
    wireFormatBuffer = parcBuffer_Resize(wireFormatBuffer, messageLength);
    char *buffer = parcBuffer_Overlay(wireFormatBuffer, 0);
    buffer += fixedHeaderLength; // skip past the header we've already read
    messageLength -= fixedHeaderLength;

    readCount = _readLink(athenaTransportLink, buffer, messageLength);

    // On error, just return and retry.
    if (readCount == -1) {
        parcBuffer_Release(&wireFormatBuffer);
        return NULL;
    }

    // A zero read means either no more data is currently available or our peer hungup.
    // Just return to retry as we'll detect EOF when we come back at the top of TCPReceive
    if (readCount == 0) {
        parcBuffer_Release(&wireFormatBuffer);
        return NULL;
    }

    // If it was a short read, attempt to read the remainder of the message
    while (readCount < messageLength) {
        ssize_t count = _readLink(athenaTransportLink, &buffer[readCount], messageLength - readCount);
        if (readCount == -1) {
            parcBuffer_Release(&wireFormatBuffer);
            return NULL;
        }
        if (count == 0) { // on error or zero read, return to check at the top of TCPReceive for EOF
            linkData->_stats.receive_ShortRead++;
            parcLog_Debug(athenaTransportLink_GetLogger(athenaTransportLink), "short read error (%s)", strerror(errno));
            parcBuffer_Release(&wireFormatBuffer);
            return NULL;
        }
        readCount += count;
    }

    parcLog_Debug(athenaTransportLink_GetLogger(athenaTransportLink), "received message (size=%d)", readCount);
    parcBuffer_SetPosition(wireFormatBuffer, fixedHeaderLength + messageLength);
    parcBuffer_Flip(wireFormatBuffer);

    // Construct, and return a ccnxMetaMessage from the wire format buffer.
    ccnxMetaMessage = ccnxMetaMessage_CreateFromWireFormatBuffer(wireFormatBuffer);
    if (ccnxMetaMessage == NULL) {
        linkData->_stats.receive_DecodeFailed++;
        parcLog_Error(athenaTransportLink_GetLogger(athenaTransportLink), "Failed to decode message from received packet.");
    } else if (ccnxTlvDictionary_GetSchemaVersion(ccnxMetaMessage) == CCNxTlvDictionary_SchemaVersion_V0) {
        parcLog_Warning(athenaTransportLink_GetLogger(athenaTransportLink),
                        "received deprecated version %d message\n", ccnxTlvDictionary_GetSchemaVersion(ccnxMetaMessage));
    }

    parcBuffer_Release(&wireFormatBuffer);

    return ccnxMetaMessage;
}

static void
_TCPClose(AthenaTransportLink *athenaTransportLink)
{
    parcLog_Info(athenaTransportLink_GetLogger(athenaTransportLink),
                 "link %s closed", athenaTransportLink_GetName(athenaTransportLink));
    _TCPLinkData *linkData = athenaTransportLink_GetPrivateData(athenaTransportLink);
    close(linkData->fd);
    _TCPLinkData_Destroy(&linkData);
}

static void
_setConnectLinkState(AthenaTransportLink *athenaTransportLink, _TCPLinkData *linkData)
{
    athenaTransportLink_SetPrivateData(athenaTransportLink, linkData);

    // Register file descriptor to be polled.  This must be set before adding the link (case ???).
    athenaTransportLink_SetEventFd(athenaTransportLink, linkData->fd);

    // Determine and flag the link cost for forwarding messages.
    // Messages without sufficient hop count collateral will be dropped.
    // Local links will always be allowed to be taken (i.e. localhost).
    bool isLocal = false;
    if (linkData->peerAddress.ss_family == AF_INET) {
        if (((struct sockaddr_in *)&linkData->peerAddress)->sin_addr.s_addr ==
            ((struct sockaddr_in *)&linkData->myAddress)->sin_addr.s_addr)
            isLocal = true;
    } else if (linkData->peerAddress.ss_family == AF_INET6) {
        if (memcmp(((struct sockaddr_in6 *)&linkData->peerAddress)->sin6_addr.s6_addr,
                   ((struct sockaddr_in6 *)&linkData->myAddress)->sin6_addr.s6_addr, 16) == 0) {
            isLocal = true;
        }
    }
    athenaTransportLink_SetLocal(athenaTransportLink, isLocal);

    // Allow messages to initially be sent
    athenaTransportLink_SetEvent(athenaTransportLink, AthenaTransportLinkEvent_Send);
}

static int
_setSocketOptions(AthenaTransportLinkModule *athenaTransportLinkModule, int fd)
{
    int on = 1;
    int result = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (void *) &on, (socklen_t) sizeof(on));
    if (result) {
        parcLog_Error(athenaTransportLinkModule_GetLogger(athenaTransportLinkModule),
                      "setsockopt failed to set SO_REUSEADDR (%s)", strerror(errno));
        return -1;
    }
#ifdef BSD_IGNORESIGPIPE
    result = setsockopt(fd, SOL_SOCKET, SO_NOSIGPIPE, (void *) &on, sizeof(on));
    if (result) {
        parcLog_Error(athenaTransportLinkModule_GetLogger(athenaTransportLinkModule),
                      "setsockopt failed to set SO_NOSIGPIPE (%s)", strerror(errno));
        return -1;
    }
#endif
    return 0;
}

#define TCP_LISTENER_FLAG "listener"
#define LINK_NAME_SPECIFIER "name%3D"
#define LOCAL_LINK_FLAG "local%3D"

static struct sockaddr_storage *
_getSockaddr(const char *moduleName, const char *hostname, in_port_t port)
{
    struct sockaddr_storage *sockaddr = NULL;

    if (strcmp(moduleName, TCP_SCHEME) == 0) {
        char address[INET_ADDRSTRLEN];
        struct addrinfo hints = { .ai_family = AF_INET };
        struct addrinfo *ai;

        if (getaddrinfo(hostname, NULL, &hints, &ai) == 0) {
            // Convert given hostname to a canonical presentation
            inet_ntop(AF_INET, (void *)&((struct sockaddr_in *)ai->ai_addr)->sin_addr, address, INET_ADDRSTRLEN);
            freeaddrinfo(ai);
            sockaddr = (struct sockaddr_storage *)parcNetwork_SockInet4Address(address, port);
        }
    } else if (strcmp(moduleName, TCP6_SCHEME) == 0) {
        char address[INET6_ADDRSTRLEN];
        struct addrinfo hints = { .ai_family = AF_INET6 };
        struct addrinfo *ai;

        if (hostname[0] == '[') {
            if (hostname[strlen(hostname) - 1] != ']') {
                errno = EINVAL;
                return sockaddr;
            }
            hostname = parcMemory_StringDuplicate(hostname + 1, strlen(hostname) - 2);
        } else {
            hostname = parcMemory_StringDuplicate(hostname, strlen(hostname));
        }
        if (getaddrinfo(hostname, NULL, &hints, &ai) == 0) {
            // Convert given hostname to a canonical presentation
            inet_ntop(AF_INET6, (void *)&((struct sockaddr_in6 *)ai->ai_addr)->sin6_addr, address, INET6_ADDRSTRLEN);
            freeaddrinfo(ai);
            sockaddr = (struct sockaddr_storage *)parcNetwork_SockInet6Address(address, port,
                                                                               ((struct sockaddr_in6 *)ai->ai_addr)->sin6_flowinfo,
                                                                               ((struct sockaddr_in6 *)ai->ai_addr)->sin6_scope_id);
        }
        parcMemory_Deallocate(&hostname);
    }

    return sockaddr;
}

typedef struct _URISpecificationParameters {
    char *linkName;
    const char *derivedLinkName;
    struct sockaddr_storage *destination;
    bool listener;
    int forceLocal;
} _URISpecificationParameters;

static void
_URISpecificationParameters_Destroy(_URISpecificationParameters **parameters)
{
    if ((*parameters)->linkName) {
        parcMemory_Deallocate(&((*parameters)->linkName));
    }
    if ((*parameters)->derivedLinkName) {
        parcMemory_Deallocate(&((*parameters)->derivedLinkName));
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

        if (strcasecmp(token, TCP_LISTENER_FLAG) == 0) {
            parameters->listener = true;
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
                _URISpecificationParameters_Destroy(&parameters);
                return 0;
            }
            if (strncasecmp(localFlag, "false", strlen("false")) == 0) {
                parameters->forceLocal = AthenaTransportLink_ForcedNonLocal;
            } else if (strncasecmp(localFlag, "true", strlen("true")) == 0) {
                parameters->forceLocal = AthenaTransportLink_ForcedLocal;
            } else {
                parcLog_Error(athenaTransportLinkModule_GetLogger(athenaTransportLinkModule),
                              "Improper local specification (%s)", token);
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
        _URISpecificationParameters_Destroy(&parameters);
        parcMemory_Deallocate(&token);
        errno = EINVAL;
        return NULL;
    }
    return parameters;
}

static AthenaTransportLink *
_TCPOpenConnection(AthenaTransportLinkModule *athenaTransportLinkModule, _URISpecificationParameters *parameters)
{
    _TCPLinkData *linkData = _TCPLinkData_Create();

    linkData->fd = socket(parameters->destination->ss_family, SOCK_STREAM, 0);
    if (linkData->fd < 0) {
        parcLog_Error(athenaTransportLinkModule_GetLogger(athenaTransportLinkModule), "socket error (%s)", strerror(errno));
        _TCPLinkData_Destroy(&linkData);
        return NULL;
    }

    // Connect to the specified peer
    linkData->peerAddress = *parameters->destination;

    int result = connect(linkData->fd, (struct sockaddr *) &linkData->peerAddress, SOCKADDR_IN_LEN(&linkData->peerAddress));
    if (result < 0) {
        parcLog_Error(athenaTransportLinkModule_GetLogger(athenaTransportLinkModule), "connect error (%s)", strerror(errno));
        _TCPLinkData_Destroy(&linkData);
        return NULL;
    }

    result = _setSocketOptions(athenaTransportLinkModule, linkData->fd);
    if (result) {
        close(linkData->fd);
        _TCPLinkData_Destroy(&linkData);
        return NULL;
    }

    // Retrieve the local endpoint data, used to create the derived name.
    socklen_t addressLength = sizeof(linkData->myAddress);
    result = getsockname(linkData->fd, (struct sockaddr *) &linkData->myAddress, &addressLength);
    if (result != 0) {
        parcLog_Error(athenaTransportLinkModule_GetLogger(athenaTransportLinkModule),
                      "Failed to obtain endpoint information from getsockname.");
        _TCPLinkData_Destroy(&linkData);
        return NULL;
    }

    parameters->derivedLinkName = _createNameFromLinkData(linkData);

    const char *linkName;
    if (parameters->linkName == NULL) {
        linkName = parameters->derivedLinkName;
    } else {
        linkName = parameters->linkName;
    }

    AthenaTransportLink *athenaTransportLink = athenaTransportLink_Create(linkName,
                                                                          _TCPSend,
                                                                          _TCPReceive,
                                                                          _TCPClose);
    if (athenaTransportLink == NULL) {
        parcLog_Error(athenaTransportLinkModule_GetLogger(athenaTransportLinkModule),
                      "athenaTransportLink_Create failed");
        _TCPLinkData_Destroy(&linkData);
        return athenaTransportLink;
    }

    athenaTransportLink_SetLogLevel(athenaTransportLink, parcLog_GetLevel(athenaTransportLinkModule_GetLogger(athenaTransportLinkModule)));
    _setConnectLinkState(athenaTransportLink, linkData);

    parcLog_Info(athenaTransportLinkModule_GetLogger(athenaTransportLinkModule),
                 "new link established: Name=\"%s\" (%s)", parameters->linkName, parameters->derivedLinkName);

    return athenaTransportLink;
}

static CCNxMetaMessage *
_TCPReceiveListener(AthenaTransportLink *athenaTransportLink)
{
    struct _TCPLinkData *listenerData = athenaTransportLink_GetPrivateData(athenaTransportLink);

    _TCPLinkData *newLinkData = _TCPLinkData_Create();

    // Accept a new tunnel connection.
    socklen_t addressLength = sizeof(newLinkData->peerAddress);
    newLinkData->fd = accept(listenerData->fd, (struct sockaddr *) &newLinkData->peerAddress, &addressLength);
    if (newLinkData->fd == -1) {
        parcLog_Error(athenaTransportLink_GetLogger(athenaTransportLink), "_TCPReceiveListener accept: %s", strerror(errno));
        _TCPLinkData_Destroy(&newLinkData);
        return NULL;
    }

    // Get the bound local hostname and port.  The listening address may have been wildcarded.
    getsockname(newLinkData->fd, (struct sockaddr *) &newLinkData->myAddress, &addressLength);

    // Clone a new link from the current listener.
    const char *derivedLinkName = _createNameFromLinkData(newLinkData);
    AthenaTransportLink *newTransportLink = athenaTransportLink_Clone(athenaTransportLink,
                                                                      derivedLinkName,
                                                                      _TCPSend,
                                                                      _TCPReceive,
                                                                      _TCPClose);
    if (newTransportLink == NULL) {
        parcLog_Error(athenaTransportLink_GetLogger(athenaTransportLink),
                      "athenaTransportLink_Clone failed");
        parcMemory_Deallocate(&derivedLinkName);
        _TCPLinkData_Destroy(&newLinkData);
        return NULL;
    }

    _setConnectLinkState(newTransportLink, newLinkData);

    // Send the new link up to be added.
    int result = athenaTransportLink_AddLink(athenaTransportLink, newTransportLink);
    if (result == -1) {
        parcLog_Error(athenaTransportLink_GetLogger(athenaTransportLink),
                      "athenaTransportLinkModule_AddLink failed: %s", strerror(errno));
        close(newLinkData->fd);
        _TCPLinkData_Destroy(&newLinkData);
        athenaTransportLink_Release(&newTransportLink);
    } else {
        parcLog_Info(athenaTransportLink_GetLogger(athenaTransportLink),
                     "new link accepted by %s: %s %s",
                     athenaTransportLink_GetName(athenaTransportLink), derivedLinkName,
                     athenaTransportLink_IsNotLocal(athenaTransportLink) ? "" : "(Local)");
    }

    parcMemory_Deallocate(&derivedLinkName);

    // Could pass a message back here regarding the new link.
    return NULL;
}

static AthenaTransportLink *
_TCPOpenListener(AthenaTransportLinkModule *athenaTransportLinkModule, _URISpecificationParameters *parameters)
{
    _TCPLinkData *linkData = _TCPLinkData_Create();

    linkData->myAddress = *(struct sockaddr_storage *)parameters->destination;

    linkData->fd = socket(parameters->destination->ss_family, SOCK_STREAM, 0);
    if (linkData->fd < 0) {
        parcLog_Error(athenaTransportLinkModule_GetLogger(athenaTransportLinkModule),
                      "socket error (%s)", strerror(errno));
        _TCPLinkData_Destroy(&linkData);
        return NULL;
    }

    int result = _setSocketOptions(athenaTransportLinkModule, linkData->fd);
    if (result) {
        close(linkData->fd);
        _TCPLinkData_Destroy(&linkData);
        return NULL;
    }

    // Set non-blocking flag
    int flags = fcntl(linkData->fd, F_GETFL, NULL);
    if (flags < 0) {
        parcLog_Error(athenaTransportLinkModule_GetLogger(athenaTransportLinkModule),
                      "fcntl failed to get non-blocking flag (%s)", strerror(errno));
        close(linkData->fd);
        _TCPLinkData_Destroy(&linkData);
        return NULL;
    }
    result = fcntl(linkData->fd, F_SETFL, flags | O_NONBLOCK);
    if (result) {
        parcLog_Error(athenaTransportLinkModule_GetLogger(athenaTransportLinkModule),
                      "fcntl failed to set non-blocking flag (%s)", strerror(errno));
        close(linkData->fd);
        _TCPLinkData_Destroy(&linkData);
        return NULL;
    }

    // bind and listen on requested address
    result = bind(linkData->fd, (struct sockaddr *) &linkData->myAddress, SOCKADDR_IN_LEN(&linkData->myAddress));
    if (result) {
        parcLog_Error(athenaTransportLinkModule_GetLogger(athenaTransportLinkModule),
                      "bind error (%s)", strerror(errno));
        close(linkData->fd);
        _TCPLinkData_Destroy(&linkData);
        return NULL;
    }
    int backlog = _listenerBacklog;
    result = listen(linkData->fd, backlog);
    if (result) {
        parcLog_Error(athenaTransportLinkModule_GetLogger(athenaTransportLinkModule),
                      "listen error (%s)", strerror(errno));
        close(linkData->fd);
        _TCPLinkData_Destroy(&linkData);
        return NULL;
    }

    parameters->derivedLinkName = _createNameFromLinkData(linkData);

    const char *linkName;
    if (parameters->linkName == NULL) {
        linkName = parameters->derivedLinkName;
    } else {
        linkName = parameters->linkName;
    }

    // Listener doesn't require a send method.  The receive method is used to establish new connections.
    AthenaTransportLink *athenaTransportLink = athenaTransportLink_Create(linkName,
                                                                          NULL,
                                                                          _TCPReceiveListener,
                                                                          _TCPClose);
    if (athenaTransportLink == NULL) {
        parcLog_Error(athenaTransportLinkModule_GetLogger(athenaTransportLinkModule),
                      "athenaTransportLink_Create failed");
        close(linkData->fd);
        _TCPLinkData_Destroy(&linkData);
        return athenaTransportLink;
    }

    athenaTransportLink_SetLogLevel(athenaTransportLink, parcLog_GetLevel(athenaTransportLinkModule_GetLogger(athenaTransportLinkModule)));
    athenaTransportLink_SetPrivateData(athenaTransportLink, linkData);
    athenaTransportLink_SetEventFd(athenaTransportLink, linkData->fd);

    // Links established for listening are not used to route messages.
    // They can be kept in a listener list that doesn't consume a linkId.
    athenaTransportLink_SetRoutable(athenaTransportLink, false);

    parcLog_Info(athenaTransportLinkModule_GetLogger(athenaTransportLinkModule),
                 "new listener established: Name=\"%s\" (%s)", parameters->linkName, parameters->derivedLinkName);

    return athenaTransportLink;
}

static AthenaTransportLink *
_TCPOpen(AthenaTransportLinkModule *athenaTransportLinkModule, PARCURI *connectionURI)
{
    AthenaTransportLink *result = 0;

    _URISpecificationParameters *parameters = _URISpecificationParameters_Create(athenaTransportLinkModule, connectionURI);
    if (parameters == NULL) {
        char *connectionURIstring = parcURI_ToString(connectionURI);
        parcLog_Error(athenaTransportLinkModule_GetLogger(athenaTransportLinkModule),
                      "Unable to parse connection URI specification (%s)", connectionURIstring);
        parcMemory_Deallocate(&connectionURIstring);
        errno = EINVAL;
        return NULL;
    }

    if (parameters->listener) {
        result = _TCPOpenListener(athenaTransportLinkModule, parameters);
    } else {
        result = _TCPOpenConnection(athenaTransportLinkModule, parameters);
    }

    // forced IsLocal/IsNotLocal, mainly for testing
    if (result && parameters->forceLocal) {
        athenaTransportLink_ForceLocal(result, parameters->forceLocal);
    }

    _URISpecificationParameters_Destroy(&parameters);

    return result;
}

static int
_TCPPoll(AthenaTransportLink *athenaTransportLink, int timeout)
{
    return 0;
}

PARCArrayList *
athenaTransportLinkModuleTCP_Init()
{
    // TCP module for establishing point to point tunnel connections.
    AthenaTransportLinkModule *athenaTransportLinkModule;
    PARCArrayList *moduleList = parcArrayList_Create(NULL);
    assertNotNull(moduleList, "parcArrayList_Create failed to create module list");

    athenaTransportLinkModule = athenaTransportLinkModule_Create("TCP",
                                                                 _TCPOpen,
                                                                 _TCPPoll);
    assertNotNull(athenaTransportLinkModule, "parcMemory_AllocateAndClear failed allocate TCP athenaTransportLinkModule");
    bool result = parcArrayList_Add(moduleList, athenaTransportLinkModule);
    assertTrue(result == true, "parcArrayList_Add failed");

    athenaTransportLinkModule = athenaTransportLinkModule_Create("TCP6",
                                                                 _TCPOpen,
                                                                 _TCPPoll);
    assertNotNull(athenaTransportLinkModule, "parcMemory_AllocateAndClear failed allocate TCP athenaTransportLinkModule");
    result = parcArrayList_Add(moduleList, athenaTransportLinkModule);
    assertTrue(result == true, "parcArrayList_Add failed");

    return moduleList;
}

PARCArrayList *
athenaTransportLinkModuleTCP6_Init()
{
    return athenaTransportLinkModuleTCP_Init();
}

void
athenaTransportLinkModuleTCP_Fini()
{
}

void
athenaTransportLinkModuleTCP6_Fini()
{
}
