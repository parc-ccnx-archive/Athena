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

#include <config.h>

#include <ccnx/forwarder/athena/athena.h>
#include <ccnx/common/ccnx_WireFormatMessage.h>
#include <ccnx/common/ccnx_Name.h>

#include <ccnx/forwarder/athena/athena_ContentStore.h>
#include <ccnx/forwarder/athena/athena_LRUContentStore.h>

//#include <ccnx/common/codec/schema_v1/ccnxCodecSchemaV1_FixedHeader.h>

#include <parc/algol/parc_HashCodeTable.h>

/**
 * @typedef AthenaContentStore
 * @brief Content Store instance private data
 */
struct athena_contentstore {
    AthenaContentStoreInterface *interface;   // The functions to use to access the implementation.
    AthenaContentStoreImplementation *impl;   // The implementation itself, local data, etc.
};

static void
_athenaContentStore_Finalize(AthenaContentStore **store)
{
    AthenaContentStoreInterface *impl = (*store)->impl;
    if (impl != NULL) {
        parcObject_Release((PARCObject **) &impl);
    }
    //parcMemory_Deallocate(store);
}

parcObject_ImplementAcquire(athenaContentStore, AthenaContentStore);

parcObject_ImplementRelease(athenaContentStore, AthenaContentStore);

parcObject_ExtendPARCObject(AthenaContentStore, _athenaContentStore_Finalize, NULL, NULL,
                            NULL, NULL, NULL, NULL);


AthenaContentStore *
athenaContentStore_Create(AthenaContentStoreInterface *interface, AthenaContentStoreConfig *config)
{
    AthenaContentStore *result = parcObject_CreateInstance(AthenaContentStore);
    if (result != NULL) {
        result->interface = interface;
        result->impl = interface->create(config);
    }

    return result;
}

bool
athenaContentStore_PutContentObject(AthenaContentStore *store, const CCNxContentObject *contentItem)
{
    if (store->interface->putContentObject == NULL) {
        return false;
    }

    return store->interface->putContentObject(store->impl, contentItem);
}

CCNxContentObject *
athenaContentStore_GetMatch(AthenaContentStore *store, const CCNxInterest *interest)
{
    if (store->interface->getMatch == NULL) {
        return NULL;
    }

    return store->interface->getMatch(store->impl, interest);
}

bool
athenaContentStore_RemoveMatch(AthenaContentStore *store, const CCNxName *name, const PARCBuffer *keyId, const PARCBuffer *contentObjectHash)
{
    if (store->interface->removeMatch == NULL) {
        return false;
    }

    return store->interface->removeMatch(store->impl, name, keyId, contentObjectHash);
}

bool
athenaContentStore_SetCapacity(AthenaContentStore *store, size_t maxSizeInMB)
{
    if (store->interface->setCapacity == NULL) {
        return false;
    }

    return store->interface->setCapacity(store->impl, maxSizeInMB);
}

size_t
athenaContentStore_GetCapacity(AthenaContentStore *store)
{
    if (store->interface->getCapacity == NULL) {
        trapNotImplemented("GetCapacity() not implemented for specified content store implementation.");
    }

    return store->interface->getCapacity(store->impl);
}

CCNxMetaMessage *
athenaContentStore_ProcessMessage(AthenaContentStore *store, const CCNxMetaMessage *message)
{
    if (store->interface->processMessage == NULL) {
        return NULL;
    }

    return store->interface->processMessage(store->impl, message);
}
