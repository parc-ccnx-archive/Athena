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
#ifndef libathena_TransportLinkAdapter_h
#define libathena_TransportLinkAdapter_h

typedef struct AthenaTransportLinkAdapter AthenaTransportLinkAdapter;

#include <parc/algol/parc_BitVector.h>
#include <parc/algol/parc_ArrayList.h>
#include <ccnx/transport/common/transport_MetaMessage.h>
#include <parc/algol/parc_FileOutputStream.h>
#include <parc/logging/parc_Log.h>
#include <parc/logging/parc_LogReporterFile.h>
#include <parc/algol/parc_URI.h>

#include <ccnx/forwarder/athena/athena_TransportLinkModule.h>
#include <ccnx/forwarder/athena/athena_TransportLink.h>

//
// Transport Link Adapter interfaces
//
//    athenaTransportLinkAdapter_Create
//    athenaTransportLinkAdapter_Destroy
//
//    athenaTransportLinkAdapter_Open
//    athenaTransportLinkAdapter_Close
//    athenaTransportLinkAdapter_Poll
//
//    athenaTransportLinkAdapter_Send
//    athenaTransportLinkAdapter_Receive
//
//    athenaTransportLinkAdapter_LinkIdToName
//    athenaTransportLinkAdapter_LinkNameToId
//
//    athenaTransportLinkAdapter_AddModule
//    athenaTransportLinkAdapter_LookupModule
//
//    athenaTransportLinkModule_Create
//    athenaTransportLinkModule_Destroy
//

/**
 * @typedef AthenaTransportLinkAdapter_RemoveLinkCallbackContext
 * @brief context for removeLink callback to coordinate removal of a link with the PIT and FIB.
 */
typedef void *AthenaTransportLinkAdapter_RemoveLinkCallbackContext;

/**
 * @typedef AthenaTransportLinkAdapter_RemoveLinkCallback
 * @brief TransportLinkAdapter callback to coordinate removal of a link with the PIT and FIB.
 */
typedef void (AthenaTransportLinkAdapter_RemoveLinkCallback)(AthenaTransportLinkAdapter_RemoveLinkCallbackContext removeLinkContext, PARCBitVector *parcBitVector);

/**
 * @abstract create a new instance of a link adapter
 * @discussion
 *
 * @param [in] removeLink call back for cleaning up link references
 * @param [in] removeLinkContext context for _removeLink callback
 * @return pointer to new instance, or NULL if it failed
 *
 * Example:
 * @code
 * void
 * _removeLink(void *context, PARCBitVector *parcBitVector)
 * {
 *     AthenaPIT *athenaPIT = (AthenaPIT *)context;
 *     athena->athenaPIB_RemoveLink(athenaFIB, parcBitVector)
 * }
 *
 * void
 * function()
 * {
 *     AthenaPIT *athenaPIT = athenaPIT_Create();
 *     AthenaTransportLinkAdapter *athenaTransportLinkAdapter = athenaTransportLinkAdapter_Create(removeLink, athenaPIT);
 *     ...
 *     athenaTransportLinkAdapter_Destroy(&athenaTransportLinkAdapter)
 *     athenaPIT_Release(&athenaPIT);
 * }
 * @endcode
 */
AthenaTransportLinkAdapter *athenaTransportLinkAdapter_Create(AthenaTransportLinkAdapter_RemoveLinkCallback,
                                                              AthenaTransportLinkAdapter_RemoveLinkCallbackContext removeLinkContext);

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
 *     PARCLog *log = athenaTransportLinkAdapter_GetLogger(athenaTransportLinkAdapter);
 * }
 * @endcode
 */
PARCLog *athenaTransportLinkAdapter_GetLogger(AthenaTransportLinkAdapter *athenaTransportLinkAdapter);

/**
 * @abstract Destroy an instance of a link adapter
 * @discussion
 *
 * Quiesce and close all links, disabling all callbacks.
 *
 * @param [in] athenaTransportLinkAdapter link adapter instance
 *
 * Example:
 * @code
 * void
 * {
 *     athenaTransportLinkAdapter_Release(&athenaTransportLinkAdapter);
 *     if (athenaTransportLinkAdapter) {
 *         parcLog_Error(logger, "Failed to release link adapter instance.");
 *     }
 * }
 * @endcode
 */
void athenaTransportLinkAdapter_Destroy(AthenaTransportLinkAdapter **);

/**
 * @abstract Receive the next available message
 * @discussion
 *
 * @param [in] athenaTransportLinkAdapter link adapter instance
 * @param [inout] ingressVector pointer to hold retrieved ingress vector
 * @param [in] timeout miliseconds to wait for a message, -1 blocks until a message is available
 * @return if successful, message received, otherwise NULL
 *
 * Example:
 * @code
 * {
 *     PARCBitVector *ingressVector;
 *     CCNxMetaMessage *ccnxMessage;
 *     ccnxMessage = AthenaTransportLinkAdapter_Receive(tla, &ingressVector, 0);
 *     parcLog_Info(logger, "Message a received from %s link.",
 *                  athenaTransportLinkAdapter_LinkIdToName(parcBitVector_NextBitSet(ingressVector, 0)));
 *     parcBitVector_Release(&ingressVector);
 *     ccnxMetaMessage_Release(&ingressVector);
 * }
 * @endcode
 */
CCNxMetaMessage *athenaTransportLinkAdapter_Receive(AthenaTransportLinkAdapter *athenaTransportLinkAdapter,
                                                    PARCBitVector **ingressVector,
                                                    int timeout);

/**
 * @abstract Send a message out the specified links
 * @discussion
 *
 * Send the message out the specified links and return the links it failed on, if any.
 * Will return a NULL pointer if all succeeded, but any vector returned must be released after processing.
 * This function is not ultimately responsible for guaranteeing delivery, it can attempt and fail or it can
 * retry, but in the end it is the responsibility of the client to resubmit an interest if it is not satisfied,
 * whether it's because the interest was never delivered, or the expected content object was never received.
 *
 * Decrements the hopcount on the message before delivery, if the incoming hopcount is 0 the message is sent along to the
 * transport stack links, but dropped from all others.
 *
 * @param [in] athenaTransportLinkAdapter link adapter instance
 * @param [in] ccnxMessage message to send
 * @param [in] egressLinkVector links to send message out on
 * @return NULL if successfull, otherwise vector list of links which failed.
 *
 * Example:
 * @code
 * {
 *     PARCBitVector *egressLinkVector = athenaFIB_Lookup(athenaFIB, ccnxMessage);
 *     if (egressLinkVector) {
 *         PARCBitVector *result = athenaTransportLinkAdapter_Send(athenaTransportLinkAdapter,
 *                                                               ccnxMessage,
 *                                                               egressLinkVector);
 *         if (result) {
 *             parcLog_Error(logger, "Failed to send message on %d requested links.",
 *                    parcBitVector_NumberOfSetBits(result));
 *             parcBitVector_Release(&result);
 *         }
 *         parcBitVector_Release(&egressLinkVector);
 *     }
 * }
 * @endcode
 */
PARCBitVector *athenaTransportLinkAdapter_Send(AthenaTransportLinkAdapter *athenaTransportLinkAdapter,
                                               CCNxMetaMessage *ccnxMessage,
                                               PARCBitVector *egressLinkVector);

/**
 * @abstract Create and add an additional link to the AthenaTransportLinkAdapter instance list
 * @discussion
 *
 * Create a new link using the specified link module.  The link module subsequently calls athenaTransportLinkAdapter_AddLink
 * with the newly instantiated link data structure or returns a failure, setting errno appropriately.
 *
 * @param [in] athenaTransportLinkAdapter link adapter instance
 * @param [in] connectionURI link connection URI specification
 * @return name of the new link on success, NULL on error with errno set to indicate the error
 *
 * Example:
 * @code
 * {
 *     PARCURI *connectionURI = parcURI_Parse("tcp://localhost:9695/name=TCP_0");
 *     const char *result = athenaTransportLinkAdapter_Open(athenaTransportLinkAdapter, connectionURI);
 *     if (result == NULL) {
 *         parcLog_Error(logger, "Failed to create new transport link TCP_0: %s", strerror(errno));
 *     }
 * }
 * @endcode
 */
const char *athenaTransportLinkAdapter_Open(AthenaTransportLinkAdapter *athenaTransportLinkAdapter,
                                            PARCURI *connectionURI);

/**
 * @abstract Poll all currently active link
 * @discussion
 *
 * @param [in] athenaTransportLinkAdapter link adapter instance
 * @return number of events
 *
 * Example:
 * @code
 * {
 *     AthenaTransportLinkAdapter *athenaTransportLinkAdapter = athenaTransportLinkAdapter_Create();
 *     ... establish links ...
 *     if (athenaTransportLinkAdapter) {
 *         int events = athenaTransportLinkAdapter_Poll(athenaTransportLinkAdapter, -1);
 *     }
 * }
 * @endcode
 */
int athenaTransportLinkAdapter_Poll(AthenaTransportLinkAdapter *athenaTransportLinkAdapter, int timeout);

/**
 * @abstract Close down and remove a link from the AthenaTransportLinkAdapter instance list
 * @discussion
 *
 * This is an accessor method for the control module to remove an active link.
 * If implemented as a synchronous operation it returns only when the link has
 * been scrubbed from all other component references.  Once a link has been removed
 * the link ID can be reused to identify a new link.
 *
 * @param [in] athenaTransportLinkAdapter link adapter instance
 * @param [in] linkVector linkID to close
 * @return link vector of unsuccessful closures, NULL on error with errno set
 *
 * Example:
 * @code
 * {
 *     PARCBitVector *linkVector = parcBitVector_Create();
 *     parcBitVector_Set(linkVector, 5); // close link id 5
 *     PARCBitVector *result = athenaTransportLinkAdapter_Close(tla, linkVector);
 *     if (result == NULL) {
 *         parcLog_Error(logger, "Failed to remove transport link 5: %s", strerror(errno));
 *         return -1;
 *     }
 *     if (parcBitVector_NumberOfSetBits(result) != 0) {
 *         parcLog_Error(logger, "Unable to remove transport link 5");
 *     }
 *     parcBitVector_Release(&result);
 *     parcBitVector_Release(&linkVector);
 * }
 * @endcode
 */
PARCBitVector *athenaTransportLinkAdapter_Close(AthenaTransportLinkAdapter *athenaTransportLinkAdapter,
                                                PARCBitVector *linkVector);

/**
 * @abstract find the link name associated with an internal link identifier
 * @discussion
 *
 * @param [in] athenaTransportLinkAdapter link adapter instance
 * @param [in] linkId internal link identifier
 * @return the link name, NULL if linkId was not found
 *
 * Example:
 * @code
 * {
 *     for (i = 0; i < parcBitVector_NumberOfSetBits(linkVector); i++) {
 *         parcLog_Info(logger, "Link id %d is named %s\n", athenaTransportLinkAdapter_LinkIdToName(tla, i));
 *     }
 * }
 * @endcode
 */
const char *athenaTransportLinkAdapter_LinkIdToName(AthenaTransportLinkAdapter *athenaTransportLinkAdapter,
                                                    int linkId);

/**
 * @abstract find the internal link identifier associated with a specific link name
 * @discussion
 *
 * @param [in] athenaTransportLinkAdapter link adapter instance
 * @param [in] linkName external name of the link
 * @return the link id on success, -1 with errno set to indicate the error
 *
 * Example:
 * @code
 * {
 *     char *linkName = "UDP_0";
 *
 *     int linkId = athenaTransportLinkAdapter_LinkNameToId((tla, linkName);
 *     if (linkVector) {
 *         parcLog_Info(logger, "Link id of %s is %d\n", linkName, linkId);
 *     } else {
 *         parcLog_Info(logger, "Link id of %s not found\n", linkName);
 *     }
 * }
 * @endcode
 */
int athenaTransportLinkAdapter_LinkNameToId(AthenaTransportLinkAdapter *athenaTransportLinkAdapter,
                                            const char *linkName);

/**
 * @abstract remove link by name
 * @discussion
 *
 * This method is used to close a link that does not have a link ID.  Typcally, these are listening instances
 * that are not routable but create link instances that are.
 *
 * @param [in] athenaTransportLinkAdapter link adapter instance
 * @param [in] linkname link instance to remove
 * @return 0 on success, -1 with errno set on error
 *
 * Example:
 * @code
 * {
 *
 * }
 * @endcode
 */
int athenaTransportLinkAdapter_CloseByName(AthenaTransportLinkAdapter *athenaTransportLinkAdapter,
                                           const char *linkName);

/**
 * @abstract tell us if a link is considered local
 * @discussion
 *
 * @param [in] athenaTransportLinkAdapter link adapter instance
 * @param [in] linkId link instance
 * @return false if local, true if not
 *
 * Example:
 * @code
 * {
 *
 * }
 * @endcode
 */
bool athenaTransportLinkAdapter_IsNotLocal(AthenaTransportLinkAdapter *athenaTransportLinkAdapter, int linkId);

/**
 * Process a message (e.g. an Interest) addressed to this module. For example, it might be a
 * message asking for a particular statistic or a control message. The response can be NULL,
 * or a response. A response to a query message might be a ContentObject with the requested data.
 * The function should return NULL if the interest request is unknown and the caller should handle the response.
 *
 * @param athenaTransportLinkAdapter instance
 * @param message the message addressed to this instance of the store
 * @return NULL if message request is unknown.
 * @return a `CCNxMetaMessage` instance containing a response.
 */
CCNxMetaMessage *athenaTransportLinkAdapter_ProcessMessage(AthenaTransportLinkAdapter *athenaTransportLinkAdapter, const CCNxMetaMessage *message);

/**
 * Set the logging level
 *
 * @param athenaTransportLinkAdapter instance
 * @param level to set logging to (see PARCLog)
 */
void athenaTransportLinkAdapter_SetLogLevel(AthenaTransportLinkAdapter *athenaTransportLinkAdapter, const PARCLogLevel level);
#endif // libathena_TransportLinkAdapter_h
