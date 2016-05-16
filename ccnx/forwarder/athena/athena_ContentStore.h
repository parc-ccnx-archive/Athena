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
#ifndef libathena_ContentStore_h
#define libathena_ContentStore_h

#include <sys/types.h>
#include <stdbool.h>

#include <parc/algol/parc_Buffer.h>
#include <parc/algol/parc_Iterator.h>

#include <ccnx/common/ccnx_Interest.h>
#include <ccnx/common/ccnx_ContentObject.h>

#include <ccnx/transport/common/transport_MetaMessage.h>

#include <ccnx/forwarder/athena/athena_ContentStoreInterface.h>

/**
 * @typedef AthenaContentStore
 * @brief Content Store instance private data
 */
typedef struct athena_contentstore AthenaContentStore;

/**
 * Create a new ContentStore, limited to the specified capacity.
 * The capacity is a cap on the amount of memory or persistent storage used by the store.
 *
 * @param store
 * @param [in] a pointer to implementation-specific configuration information
 * @return pointer to the new content store instance.
 */
AthenaContentStore *athenaContentStore_Create(AthenaContentStoreInterface *interface, AthenaContentStoreConfig *config);

/**
 * Release a ContentStore
 *
 * @param store
 */
void athenaContentStore_Release(AthenaContentStore **store);

/**
 * Put a ContentObject into the store. Requires the content object to have the wire format buffer attached.
 *
 * @param store
 * @param [in] contentItem - the item to store.
 * @return true if successful.
 */
bool athenaContentStore_PutContentObject(AthenaContentStore *store, const CCNxContentObject *contentItem);

/**
 * Put a ContentObject, in wire format, into the store.
 *
 * @param store
 * @param [in] wireFormat - the item to store.
 * @param [in] name - the name of the ContentObject. Can be NULL.
 * @param [in] keyIdRestriction - the Key ID associated with the matching ContentObject. Can be NULL.
 * @param [in] contentObjectHash - the SHA25 hash of the ContentObject. Can be NULL.
 * @return true if successful.
 */
//bool athenaContentStore_PutWireFormat(AthenaContentStore *store, const PARCBuffer *wireFormat, const CCNxName *name, const PARCKeyId *keyId, const PARCBuffer *contentObjectHash);

/**
 * Given a {@link CCNxInteres}, attempt to find a {@link CCNxContentObject} that matches either:
 *    - its name and content object hash restriction,
 *    - its name and key Id restriction,
 *    - or just its name, if no other restriction is specified.
 * Results will be from the most restricive search. So, if a name and a contentObjectHash is specified then only the ContentObjects matching both will be returned.
 * @param store
 * @param [in] interest - the {@link CCNxInterest} to attempt to find a match for.
 * @return a pointer to the matched ContentObject.
 */
CCNxContentObject *athenaContentStore_GetMatch(AthenaContentStore *store, const CCNxInterest *interest);

/**
 * Remove an item that matches the specified search parameters.
 *
 * @param store
 * @param [in] name - the name of the ContentObject. Can be NULL.
 * @param [in] keyIdRestriction - the Key ID associated with the matching ContentObject. Can be NULL.
 * @param [in] contentObjectHash - the SHA25 hash of the ContentObject. Can be NULL.
 * @param [in] interest - the interest for which to find a match
 * @return true if an item was removed from the store, false otherwise.
 */
bool athenaContentStore_RemoveMatch(AthenaContentStore *store, const CCNxName *name, const PARCBuffer *keyIdRestriction, const PARCBuffer *contentObjectHash);

/**
 * Set the max size of the specified `AthenaContentStore` to the size, in MB, specified by `maxSizeInMB`. If the Content Store
 * implementation needs to discard items to apply the new size limit, the order in which items are discarded is undefined.
 * This should not be considered a precise limit, but Content Store implementations should do their best to roughly comply.
 *
 * @param store
 * @param [in] maxSizeInMB - the requested upper limit to the size of the Content Store, in MB.
 * @return true if the new size limit could be applied, false otherwise.
 */
bool athenaContentStore_SetCapacity(AthenaContentStore *store, size_t maxSizeInMB);

/**
 * Return the maximum capacity, in MB, of the specified `AthenaContentStore` instance.
 *
 * @param store
 * @return true if the new size limit could be applied, false otherwise.
 */
size_t athenaContentStore_GetCapacity(AthenaContentStore *store);

/**
 * Process a message (e.g. an Interest) addressed to this module. For example, it might be a
 * message asking for a particular statistic or a control message. The response can be NULL,
 * or a response. A response to a query message might be a ContentObject with the requested data.
 * The function should return NULL if the interest request is unknown and the caller should handle the response.
 *
 * @param store
 * @param message the message addressed to this instance of the store
 * @return NULL if no response is required or available.
 * @return a `CCNxMetaMessage` instance containing a response.
 */
CCNxMetaMessage *athenaContentStore_ProcessMessage(AthenaContentStore *store, const CCNxMetaMessage *message);
#endif // libathena_ContentStore_h
