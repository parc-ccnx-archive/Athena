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
#ifndef libathena_athena_pit_h
#define libathena_athena_pit_h

#include <parc/algol/parc_BitVector.h>

#include <ccnx/common/ccnx_ContentObject.h>
#include <ccnx/common/ccnx_Interest.h>

#include <ccnx/transport/common/transport_MetaMessage.h>

/*
 * PIT interfaces
 *
 *    athenaPIT_Create
 *    athenaPIT_Release
 *
 *    athenaPIT_Match
 *
 *    athenaPIT_AddInterest
 *    athenaPIT_RemoveInterest
 *    athenaPIT_RemoveLink
 */

/**
 * @typedef AthenaPIT
 * @brief PIT table, KEY == tlvName..., VALUE == AthenaPITEntry
 */
struct athena_pit;
typedef struct athena_pit AthenaPIT;

/**
 * @typedef AthenaPITResolution
 * @brief PIT decision resolved during insertion
 */
typedef enum AthenaPITResolution {
    AthenaPITResolution_Forward,
    AthenaPITResolution_Aggregated,
    AthenaPITResolution_Error = -1
} AthenaPITResolution;

/**
 * @abstract Create a PIT table with the default entry limit.
 * @discussion
 *
 * @return pointer to a PIT instance
 *
 * Example:
 * @code
 * {
 *     AthenaPIT *athenaPIT = athenaPIT_Create();
 *     if (athenaPIT == NULL) {
 *         parcLog_Error(logger, "Failed to create PIT.");
 *     }
 * }
 * @endcode
 */
AthenaPIT *athenaPIT_Create();

/**
 * @abstract Create a PIT table
 * @discussion
 *
 * @param [in] capacity - PIT entry limit
 *
 * @return pointer to a PIT instance
 *
 * Example:
 * @code
 * {
 *     AthenaPIT *athenaPIT = athenaPIT_CreateCapacity(1000000);
 *     if (athenaPIT == NULL) {
 *         parcLog_Error(logger, "Failed to create PIT.");
 *     }
 * }
 * @endcode
 */
AthenaPIT *athenaPIT_CreateCapacity(size_t capacity);

/**
 * @abstract Release a PIT
 * @discussion
 *
 * @param [in] athenaPIT
 *
 * Example:
 * @code
 * {
 *     athenaPIT_Release(&athenaPIT);
 *     if (athenaPIT) {
 *         parcLog_Error(logger, "Failed to release PIT.");
 *     }
 * }
 * @endcode
 */
void athenaPIT_Release(AthenaPIT **athenaPIT);

/**
 * @abstract Add an interest to the PIT
 * @discussion
 *
 * @param [in] athenaPIT
 * @param [in] ccnxInterestMessage
 * @param [in] ingressVector
 * @param [out] expectedReturnVector
 * @return AthenaPITResolution_Aggregated if aggregated, AthenaPITResolution_Forward if it needs to be forwarded, AthenaPITResolution_Error on error.
 *
 * Example:
 * @code
 * {
 *     athenaPIT_AddInterest(athenaPIT, interestMessage, &expectedReturnVector);
 * }
 * @endcode
 */
AthenaPITResolution athenaPIT_AddInterest(AthenaPIT *athenaPIT,
                                          const CCNxInterest *ccnxInterestMessage,
                                          const PARCBitVector *ingressVector,
                                          PARCBitVector **expectedReturnVector);

/**
 * @abstract Remove an interest from the PIT
 * @discussion
 *
 * @param [in] athenaPIT
 * @param [in] ccnxInterestMessage Interest to remove
 * @param [in] ingressVector The ingressVector of the interest to Remove
 * @return true on success
 *
 * Example:
 * @code
 * {
 *     if (athenaPIT_RemoveInterest(athenaPIT, interestMessage, ingressVector) != true) {
 *         parcLog_Error(logger, "Failed to remove interest from the pending interest table");
 *     }
 * }
 * @endcode
 */
bool athenaPIT_RemoveInterest(AthenaPIT *athenaPIT,
                              const CCNxInterest *ccnxInterestMessage,
                              const PARCBitVector *ingressVector);

/**
 * @abstract get the delivery vector in the PIT for a message
 * @discussion
 *
 * @param [in] athenaPIT
 * @param [in] ccnxContentMessage
 *
 * Example:
 * @code
 * {
 *     PARCBitVector *egressVector = athenaPIT_Match(athenaPIT, ccnxMessage, ingressVector);
 *     PARCBitVector *result = ccnxTransportLinkAdapter_Send(ccnxLinkAdapter, ccnxMessage, egressVector);
 *     if (result) {
 *         // possibly attempt resend on result vector
 *         parcBitVector_Release(&result);
 *     }
 *     parcBitVector_Release(&egressVector);
 *     parcBitVector_Release(&ingressVector);
 * }
 * @endcode
 */
PARCBitVector *athenaPIT_Match(AthenaPIT *athenaPIT,
                               const CCNxName *name,
                               const PARCBuffer *keyId,
                               const PARCBuffer *contentId,
                               const PARCBitVector *ingressVector);

/**
 * @abstract Remove the specified link from any and all PIT entries.
 * @discussion
 *
 * PIT entries which result in having no ingress ports are removed
 *
 * @param [in] athenaPIT
 * @param [in] ccnxLinkVector
 * @return true on success.
 *
 * Example:
 * @code
 * {
 *     PARCBitVector *ccnxLinkVector = parcBitVector_Create();
 *     parcBitVector_Set(ccnxLinkVector, 3); // we're removing link id 3
 *     athenaPIT_RemoveLink(athenaPIT, ccnxLinkVector);
 *     parcBitVector_Release(&ccnxLinkVector);
 * }
 * @endcode
 */
bool athenaPIT_RemoveLink(AthenaPIT *athenaPIT, const PARCBitVector *ccnxLinkVector);

/**
 * @abstract Get the current number of PIT table entries.
 * @discussion
 *
 * @param [in] athenaPIT
 *
 * @return the number of entries
 *
 * Example:
 * @code
 * {
 *     AthenaPIT *pit = athenaPIT_Create();
 *     ...
 *     // Add some entries ...
 *     ...
 *     size_t entryCount = athenaPIT_GetNumberOfTableEntries(pit);
 * }
 * @endcode
 */
size_t athenaPIT_GetNumberOfTableEntries(const AthenaPIT *athenaPIT);

/**
 * @abstract Get the current number interests pending
 * @discussion
 *
 * This is different than then number of table entries in that it included
 * aggrigated interests in the count.
 *
 * @param [in] athenaPIT
 *
 * @return the number of entries
 *
 * Example:
 * @code
 * {
 *     AthenaPIT *pit = athenaPIT_Create();
 *     ...
 *     // Add some entries ...
 *     ...
 *     size_t entryCount = athenaPIT_GetNumberOfPendingInterests(pit);
 * }
 * @endcode
 */
size_t athenaPIT_GetNumberOfPendingInterests(const AthenaPIT *athenaPIT);

/**
 * @abstract Get the current walking average lifetimes for the last N matched pit entries.
 * @discussion
 *
 * @param [in] athenaPIT
 *
 * @return the mean lifetime
 *
 * Example:
 * @code
 * {
 *     AthenaPIT *pit = athenaPIT_Create();
 *     ...
 *     // Run for a while ...
 *     ...
 *     time_t meanLifetime = athenaPIT_GetMeanEntryLifetime(pit);
 * }
 * @endcode
 */
time_t athenaPIT_GetMeanEntryLifetime(const AthenaPIT *athenaPIT);

/**
 * Process a message (e.g. an Interest) addressed to this module. For example, it might be a
 * message asking for a particular statistic or a control message. The response can be NULL,
 * or a response. A response to a query message might be a ContentObject with the requested data.
 *
 * @param [in] athenaPIT
 * @param [in] message the message addressed to this instance of the store
 * @return NULL if no response is required or available.
 * @return a `CCNxMetaMessage` instance containing a response, if applicable.
 *
 *
 * Example:
 * @code
 * {
 *     AthenaPIT *pit = athenaPIT_Create();
 *     CCNxMetaMessage *response = athenaPIT_ProcessMessage(pit, interest);
 * }
 * @endcode
 */
CCNxMetaMessage *athenaPIT_ProcessMessage(const AthenaPIT *athenaPIT, const CCNxMetaMessage *message);

/**
 * @abstract Return a list of PIT entry information
 * @discussion
 *
 * The function creates a PARCList of PARCBuffers containing PIT entry information in csv format.
 * The first element of the list is column header information. The information currently returned
 * for each entry is:
 *    Name, ingress vector, egress vector, KeyId Restricted, Content Hash Restricted, Nameless
 *
 * @param [in] athenaPIT PIT instance
 * @return PARCList List of PIT entry information
 */
PARCList *athenaPIT_CreateEntryList(const AthenaPIT *athenaPIT);

#endif // libathena_pit_h
