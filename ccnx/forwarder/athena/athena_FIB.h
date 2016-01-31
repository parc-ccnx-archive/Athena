/*
 * Copyright (c) 2015, Xerox Corporation (Xerox)and Palo Alto Research Center (PARC)
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
 * @copyright 2015, Xerox Corporation (Xerox)and Palo Alto Research Center (PARC).  All rights reserved.
 */
#ifndef libathena_athena_FIB_h
#define libathena_athena_FIB_h

#include <parc/algol/parc_BitVector.h>

#include <ccnx/transport/common/transport_MetaMessage.h>

/*
 * FIB interfaces
 *
 *    athenaFIB_Create
 *    athenaFIB_Release
 *
 *    athenaFIB_RemoveLink
 *
 *    athenaFIB_Lookup
 *    athenaFIB_DeleteRoute
 *    athenaFIB_AddRoute
 */

/**
 * @typedef AthenaFIB
 * @brief FIB table, KEY == tlvName, VALUE == AthenaFIBEntry
 */
struct athena_FIB;
typedef struct athena_FIB AthenaFIB;

/**
 * @typedef AthenaFIBEntry
 * @brief FIB table entry, vector of links to forward to
 */
struct athena_FIB_list_entry;
typedef struct athena_FIB_list_entry AthenaFIBListEntry;


CCNxName *athenaFIBListEntry_GetName(AthenaFIBListEntry *entry);

int athenaFIBListEntry_GetLinkId(AthenaFIBListEntry *entry);

/**
 * @abstract Create a FIB table
 * @discussion
 *
 * @return pointer to a FIB instance
 *
 * Example:
 * @code
 * {
 *     AthenaFIB *athenaFIB = athenaFIB_Create();
 *     if (athenaFIB == NULL) {
 *         parcLog_Error(logger, "Failed to create a FIB.");
 *     }
 * }
 * @endcode
 */
AthenaFIB *athenaFIB_Create();

/**
 * @abstract Release a FIB
 * @discussion
 *
 * @param [in] athenaFIB
 *
 * Example:
 * @code
 * {
 *     athenaFIB_Release(&athenaFIB);
 *     if (athenaFIB) {
 *         parcLog_Error(logger, "Failed to release FIB.");
 *     }
 * }
 * @endcode
 */
void athenaFIB_Release(AthenaFIB **athenaFIB);

/**
 * @abstract Remove the specified link from any and all FIB routes.
 * @discussion
 *
 * Routes which result in having no egress ports are removed from the FIB
 *
 * @param [in] athenaFIB
 * @param [in] ccnxLinkVector
 * @return true on success.
 *
 * Example:
 * @code
 * {
 *     PARCBitVector *ccnxLinkVector = parcBitVector_Create();
 *     parcBitVector_Set(ccnxLinkVector, 3); // we're removing link id 3
 *     athenaFIB_RemoveLink(athenaFIB, ccnxLinkVector);
 *     parcBitVector_Release(&ccnxLinkVector);
 * }
 * @endcode
 */
bool athenaFIB_RemoveLink(AthenaFIB *athenaFIB, const PARCBitVector *ccnxLinkVector);

// A athenaFIB_AddLink method is not required as it's implied in AddRoute
// bool athenaFIB_AddLink(AthenaFIB *athenaFIB, PARCBitVector *ccnxLinkVector);

/**
 * @abstract lookup destination vector for message in FIB
 * @discussion
 *
 * @param [in] athenaFIB
 * @param [in] ccnxMessage
 * @return vector of links to send message to
 *
 * Example:
 * @code
 * {
 *     PARCBitVector *egressVector = athenaFIB_Lookup(athenaFIB, ccnxMessage);
 *     _transportSendMessage(ccnxMessage, egressVector);
 *     parcBitVector_Release(egressVector);
 * }
 * @endcode
 */
PARCBitVector *athenaFIB_Lookup(AthenaFIB *athenaFIB, const CCNxName *ccnxName);

/**
 * @abstract add route to FIB
 * @discussion
 *
 * @param [in] athenaFIB
 * @param [in] ccnxName
 * @param [in] ccnxLinkVector
 * @return true if successful
 *
 * Example:
 * @code
 * {
 *     CCNxName *ccnxName = ccnxName_CreateFromURI("lci:/ccnx/tutorial");
 *     PARCBitVector *egressLinks = parcBitVector_Create();
 *     // send out on links 3 and 5
 *     parcBitVector_Set(egressLinks, 3);
 *     parcBitVector_Set(egressLinks, 5);
 *     athenaFIB_AddRoute(athenaFIB, ccnxName, egressLinks);
 *     parcBitVector_Release(&egressLinks);
 *     ccnxName_Release(&ccnxName);
 * }
 * @endcode
 */
bool athenaFIB_AddRoute(AthenaFIB *athenaFIB, const CCNxName *ccnxName, const PARCBitVector *ccnxLinkVector);

/**
 * @abstract remove route to link from FIB
 * @discussion
 *
 * @param [in] athenaFIB
 * @param [in] ccnxName
 * @param [in] ccnxLinkVector
 * @return true if successful
 *
 * Example:
 * @code
 * {
 *     CCNxName *ccnxName = ccnxName_CreateFromURI("lci:/ccnx/tutorial");
 *     PARCBitVector *egressLinks = parcBitVector_Create();
 *     parcBitVector_Set(egressLinks, 3); // delete link id 3
 *     athenaFIB_DeleteRoute(athenaFIB, ccnxName, egressLinks);
 *     parcBitVector_Release(&egressLinks);
 *     ccnxName_Release(&ccnxName);
 * }
 * @endcode
 */
bool athenaFIB_DeleteRoute(AthenaFIB *athenaFIB, const CCNxName *ccnxName, const PARCBitVector *ccnxLinkVector);

/**
 * @abstract retrieve the entry list for the FIB.
 * @discussion
 *
 * The index into the list is the linkId and the element value is the CCNxName that goes along
 * with the linkId.
 *
 * @param [in] athenaFIB
 * @return PARCList of links
 *
 * Example:
 * @code
 * {
 * }
 */
PARCList *athenaFIB_CreateEntryList(AthenaFIB *athenaFIB);

/**
 * Process a message (e.g. an Interest) addressed to this module. For example, it might be a
 * message asking for a particular statistic or a control message. The response can be NULL,
 * or a response. A response to a query message might be a ContentObject with the requested data.
 * The function should return NULL if the interest request is unknown and the caller should handle the response.
 *
 * @param athenaFIB instance
 * @param message the message addressed to this instance of the store
 * @return NULL if message request is unknown.
 * @return a `CCNxMetaMessage` instance containing a response.
 */
CCNxMetaMessage *athenaFIB_ProcessMessage(AthenaFIB *athenaFIB, const CCNxMetaMessage *message);
#endif // libccnx_FIB_h
