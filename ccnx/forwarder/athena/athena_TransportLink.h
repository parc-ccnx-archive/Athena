/*
 * Copyright (c) 2015, Xerox Corporation (Xerox) and Palo Alto Research Center, Inc (PARC)
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
 * @copyright (c) 2015, Xerox Corporation (Xerox) and Palo Alto Research Center, Inc (PARC).  All rights reserved.
 */
#ifndef libathena_TransportLink_h
#define libathena_TransportLink_h

#include <ccnx/transport/common/transport_MetaMessage.h>

#include <parc/algol/parc_BitVector.h>
#include <parc/algol/parc_FileOutputStream.h>
#include <parc/logging/parc_Log.h>
#include <parc/logging/parc_LogReporterFile.h>

/**
 * @typedef AthenaTransportLink
 * @brief Transport Link instance private data
 */
typedef struct AthenaTransportLink AthenaTransportLink;

/**
 * @typedef AthenaTransportLink_SendMethod
 * @brief method provided by link implementation for sending messages
 */
typedef int (AthenaTransportLink_SendMethod)(struct AthenaTransportLink *athenaTransportLink, CCNxMetaMessage *ccnxMetaMessage);

/**
 * @typedef AthenaTransportLink_ReceiveMethod
 * @brief method provided by link implementation for receiving messages
 */
typedef CCNxMetaMessage *(AthenaTransportLink_ReceiveMethod)(struct AthenaTransportLink *athenaTransportLink);

/**
 * @typedef AthenaTransportLink_CloseMethod
 * @brief method provided by link implementation for closing the link
 */
typedef void (AthenaTransportLink_CloseMethod)(struct AthenaTransportLink *athenaTransportLink);

/**
 * @typedef AthenaTransportLink_RemoveLinkCallback
 * @brief method to provide notification of link removals to our TransportLinkModule
 */
typedef void (AthenaTransportLink_RemoveLinkCallback)(void *, struct AthenaTransportLink *athenaTransportLink);

/**
 * @typedef AthenaTransportLink_RemoveLinkCallbackContext
 * @brief private context data for remove link callback
 */
typedef void *AthenaTransportLink_RemoveLinkCallbackContext;

/**
 * @typedef AthenaTransportLink_AddLinkCallback
 * @brief method to provide notification of link additions to our TransportLinkModule
 */
typedef int (AthenaTransportLink_AddLinkCallback)(void *, struct AthenaTransportLink *athenaTransportLink);

/**
 * @typedef AthenaTransportLink_AddLinkCallbackContext
 * @brief private context data for add link callback
 */
typedef void *AthenaTransportLink_AddLinkCallbackContext;

/**
 * @typedef AthenaTransportLinkFlag
 * @brief An enumeration of link instance flags
 */
typedef enum {
    AthenaTransportLinkFlag_None          = 0x00,
    AthenaTransportLinkFlag_IsNotRoutable = 0x01,
    AthenaTransportLinkFlag_IsLocal       = 0x02
} AthenaTransportLinkFlag;

#define AthenaTransportLink_ForcedLocal  1
#define AthenaTransportLink_ForcedNonLocal  -1

/**
 * @typedef AthenaTransportLinkEvent
 * @brief An enumeration of event types
 */
typedef enum {
    AthenaTransportLinkEvent_None    = 0x00,
    AthenaTransportLinkEvent_Receive = 0x01,
    AthenaTransportLinkEvent_Send    = 0x02,
    AthenaTransportLinkEvent_Error   = 0x04,
    AthenaTransportLinkEvent_Closing = 0x08
} AthenaTransportLinkEvent;

/**
 * @abstract obtain a pointer to the logger
 * @discussion
 *
 * @return pointer to logging facility
 *
 * Example:
 * @code
 * void
 * {
 *     PARCLog *log = athenaTransportLink_GetLogger(athenaTransportLink);
 * }
 * @endcode
 */
PARCLog *athenaTransportLink_GetLogger(AthenaTransportLink *athenaTransportLink);

/**
 * @abstract create a new transport link instance
 * @discussion
 *
 * @param [in] name of new link
 * @param [in] send method for sending messages on link
 * @param [in] receive method for receiving messages on link
 * @param [in] close method for calling the close method of the link implementation
 * @param [in] error method for calling the error method of the link implementation
 * @return pointer to new link instance
 *
 * Example:
 * @code
 * void
 * {
 *     PARCLog *log = athenaTransportLink_GetLogger(athenaTransportLink);
 * }
 * @endcode
 */
AthenaTransportLink *athenaTransportLink_Create(const char *name,
                                                AthenaTransportLink_SendMethod *send,
                                                AthenaTransportLink_ReceiveMethod *receive,
                                                AthenaTransportLink_CloseMethod *close);

/**
 * @abstract create a new transport link instance from an existing link
 * @discussion
 *
 * @param [in] athenaTransportLink link to clone new instance from
 * @param [in] name of new link
 * @param [in] send method for sending messages on link
 * @param [in] receive method for receiving messages on link
 * @param [in] close method for callin the close method of the link implementation
 * @return pointer to new link instance
 *
 * Example:
 * @code
 * void
 * {
 * }
 * @endcode
 */
AthenaTransportLink *athenaTransportLink_Clone(AthenaTransportLink *athenaTransportLink,
                                               const char *name,
                                               AthenaTransportLink_SendMethod *send,
                                               AthenaTransportLink_ReceiveMethod *receive,
                                               AthenaTransportLink_CloseMethod *close);

/**
 * @abstract release a reference to a transport link
 * @discussion
 *
 * @param [in] athenaTransportLink link to decrement reference count on
 *
 * Example:
 * @code
 * void
 * {
 *     athenaTransportLink_Release(&athenaTransportLink);
 * }
 * @endcode
 */
void athenaTransportLink_Release(AthenaTransportLink **);

/**
 * @abstract acquire a reference to a transport link
 * @discussion
 *
 * @param [in] athenaTransportLink link to increment reference count on
 * @return pointer to link instance
 *
 * Example:
 * @code
 * void
 * {
 *     athenaTransportLink_Acquire(&athenaTransportLink);
 * }
 * @endcode
 */
AthenaTransportLink *athenaTransportLink_Acquire(const AthenaTransportLink *);

/**
 * @abstract called to invoke the link specific close method
 * @discussion
 *
 * @param [in] athenaTransportLink link instance to close
 *
 * Example:
 * @code
 * {
 *     athenaTransportLink_Close(athenaTransportLink);
 * }
 * @endcode
 */
void
athenaTransportLink_Close(AthenaTransportLink *athenaTransportLink);

/**
 * @abstract called to invoke the link specific receive method
 * @discussion
 *
 * @param [in] athenaTransportLink link instance to return message from
 * @return pointer to received message, NULL if no message or errno set to indicate error
 *
 * Example:
 * @code
 * {
 * }
 * @endcode
 */
CCNxMetaMessage *athenaTransportLink_Receive(AthenaTransportLink *athenaTransportLink);

/**
 * @abstract called to invoke the link specific send method
 * @discussion
 *
 * @param [in] athenaTransportLink link instance to send message on
 * @param [in] ccnxMetaMessage message to send
 * @return 0 if successful, -1 on error with errno set to indicate error
 *
 * Example:
 * @code
 * {
 * }
 * @endcode
 */
int athenaTransportLink_Send(AthenaTransportLink *athenaTransportLink, CCNxMetaMessage *ccnxMetaMessage);

/**
 * @abstract called to return the name of the transport link
 * @discussion
 *
 * @param [in] athenaTransportLink link instance
 * @return name, or NULL with errno set to indicate error
 *
 * Example:
 * @code
 * {
 * }
 * @endcode
 */
const char *athenaTransportLink_GetName(AthenaTransportLink *athenaTransportLink);

/**
 * @abstract called to invoke the link specific close method
 * @discussion
 *
 * @param [in] athenaTransportLink link instance to query on set events
 * @return events currently set on link
 *
 * Example:
 * @code
 * {
 * }
 * @endcode
 */
AthenaTransportLinkEvent athenaTransportLink_GetEvent(AthenaTransportLink *athenaTransportLink);

/**
 * @abstract clear an event on an athenaTransportLInk instance
 * @discussion
 *
 * @param [in] athenaTransportLink link instance to clear events on
 * @param [in] event to clear
 *
 * Example:
 * @code
 * {
 * }
 * @endcode
 */
void athenaTransportLink_ClearEvent(AthenaTransportLink *athenaTransportLink, AthenaTransportLinkEvent event);

/**
 * @abstract set an event on an athenaTransportLInk instance
 * @discussion
 *
 * @param [in] athenaTransportLink link instance to set events on
 * @param [in] event to set
 *
 * Example:
 * @code
 * {
 * }
 * @endcode
 */
void athenaTransportLink_SetEvent(AthenaTransportLink *athenaTransportLink, AthenaTransportLinkEvent event);

/**
 * @abstract get private data pointer from transport link
 * @discussion
 *
 * @param [in] athenaTransportLink link instance to return data from
 * @return pointer to previously set transport link private data
 *
 * Example:
 * @code
 * {
 * }
 * @endcode
 */
void *athenaTransportLink_GetPrivateData(AthenaTransportLink *athenaTransportLink);

/**
 * @abstract called to invoke the link specific close method
 * @discussion
 *
 * @param [in] athenaTransportLink link instance to set private data on
 * @param [in] linkData private data pointer
 *
 * Example:
 * @code
 * {
 * }
 * @endcode
 */
void athenaTransportLink_SetPrivateData(AthenaTransportLink *athenaTransportLink, void *linkData);

/**
 * @abstract called from the link specific adapter to coordinate the addition of a link instance
 * @discussion
 *
 * @param [in] athenaTransportLink link adapter instance
 * @param [in] addLink method to call with new link instance
 * @param [in] addLinkContext contextual state for addLink call
 *
 * Example:
 * @code
 * {
 *
 * }
 * @endcode
 */
void
athenaTransportLink_SetAddLinkCallback(AthenaTransportLink *athenaTransportLink,
                                       AthenaTransportLink_AddLinkCallback *addLink,
                                       AthenaTransportLink_AddLinkCallbackContext addLinkContext);

/**
 * @abstract called from the link specific adapter to coordinate the addition of a link instance
 * @discussion
 *
 * @param [in] athenaTransportLink link adapter instance
 * @param [in] removeLink method to call with link instance to remove
 * @param [in] removeLinkContext contextual state for removeLink call
 *
 * Example:
 * @code
 * {
 *
 * }
 * @endcode
 */
void
athenaTransportLink_SetRemoveLinkCallback(AthenaTransportLink *athenaTransportLink,
                                          AthenaTransportLink_RemoveLinkCallback *removeLink,
                                          AthenaTransportLink_RemoveLinkCallbackContext removeLinkContext);

/**
 * @abstract get the file descriptor to be used by the transport link adapter for polling
 * @discussion
 *
 * @param [in] athenaTransportLink link adapter instance
 * @return the current pollfd setting for the transport link
 *
 * Example:
 * @code
 * {
 *
 * }
 * @endcode
 */
int athenaTransportLink_GetEventFd(AthenaTransportLink *athenaTransportLink);

/**
 * @abstract allow the transport link adapter to poll events for the link
 * @discussion
 *
 * @param [in] athenaTransportLink link adapter instance
 * @param [in] fd file descriptor to poll, -1 if polling is to be performed locally
 *
 * Example:
 * @code
 * {
 *
 * }
 * @endcode
 */
void athenaTransportLink_SetEventFd(AthenaTransportLink *athenaTransportLink, int eventFd);

/**
 * @abstract set a flag on the link
 * @discussion
 *
 * @param [in] athenaTransportLink link adapter instance
 * @param [in] flags link adapter instance
 *
 * Example:
 * @code
 * {
 *
 * }
 * @endcode
 */
void athenaTransportLink_SetRoutable(AthenaTransportLink *athenaTransportLink, bool routable);

/**
 * @abstract get the flags set on a link
 * @discussion
 *
 * @param [in] athenaTransportLink link adapter instance
 * @return the current link flags
 *
 * Example:
 * @code
 * {
 *
 * }
 * @endcode
 */
bool athenaTransportLink_IsNotRoutable(AthenaTransportLink *athenaTransportLink);

/**
 * @abstract add a new link
 * @discussion
 *
 * @param [in] athenaTransportLink link adapter instance
 * @param [in] athenaTransportLink new link adapter instance
 * @return 0 on success, -1 with errno set on error
 *
 * Example:
 * @code
 * {
 *
 * }
 * @endcode
 */
int athenaTransportLink_AddLink(AthenaTransportLink *athenaTransportLink, AthenaTransportLink *newTransportLink);

/**
 * @abstract tell us if a link is considered local
 * @discussion
 *
 * @param [in] athenaTransportLink link instance
 * @return false if local, true if not
 *
 * Example:
 * @code
 * {
 *
 * }
 * @endcode
 */
bool athenaTransportLink_IsNotLocal(AthenaTransportLink *athenaTransportLink);

/**
 * @abstract set the transport isLocal flag.
 * @discussion
 *
 * @param [in] athenaTransportLink link instance
 * @param [in] flag to set local value to, true if local ...
 *
 * Example:
 * @code
 * {
 *
 * }
 * @endcode
 */
void athenaTransportLink_SetLocal(AthenaTransportLink *athenaTransportLink, bool flag);

/**
 * @abstract force the connection to be local or non-local
 * @discussion
 *
 * @param [in] athenaTransportLink link instance
 * @param [in] forceLocal 0 if disabled, 1 if local, -1 if non-local
 *
 * Example:
 * @code
 * {
 *
 * }
 * @endcode
 */
void athenaTransportLink_ForceLocal(AthenaTransportLink *athenaTransportLink, int forceLocal);

/**
 * @abstract tell us whether the link is forced local/non-local
 * @discussion
 *
 * @param [in] athenaTransportLink link instance
 * @return true of link is forced, false if not
 *
 * Example:
 * @code
 * {
 *
 * }
 * @endcode
 */
bool athenaTransportLink_IsForceLocal(AthenaTransportLink *athenaTransportLink);

/**
 * Set the logging level for a link
 *
 * @param athenaTransportLink instance
 * @param level to set logging to (see PARCLog)
 */
void athenaTransportLink_SetLogLevel(AthenaTransportLink *athenaTransportLink, const PARCLogLevel level);
#endif // libathena_TransportLink_h
