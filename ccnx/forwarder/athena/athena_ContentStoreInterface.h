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
 * @author Alan Walendowski, Palo Alto Research Center (Xerox PARC)
 * @copyright 2015, Xerox Corporation (Xerox)and Palo Alto Research Center (PARC).  All rights reserved.
 */

#ifndef libathena_ContentStoreInterface_h
#define libathena_ContentStoreInterface_h

#include <sys/types.h>
#include <stdbool.h>

#include <parc/algol/parc_Buffer.h>
#include <parc/algol/parc_Iterator.h>

typedef void AthenaContentStoreConfig;

typedef void AthenaContentStoreImplementation;

typedef struct athena_contentstore_interface {

    char *description;

    AthenaContentStoreImplementation *(*create)(AthenaContentStoreConfig *config);

    void (*release)(AthenaContentStoreImplementation **storePtr);

    /** @see athenaContentStore_PutContentObject */
    bool (*putContentObject)(AthenaContentStoreImplementation *store, const CCNxContentObject *content); // Note: the content object must contain its wireformat buffer

    /** @see athenaContentStore_GetMatch */
    CCNxContentObject *(*getMatch)(AthenaContentStoreImplementation *store, const CCNxInterest *interest);

    /** @see athenaContentStore_RemoveMatch */
    bool (*removeMatch)(AthenaContentStoreImplementation *store, const CCNxName *name, const PARCBuffer *keyIdRestriction, const PARCBuffer *contentObjectHash);

    /** @see athenaContentStore_SetCapacity */
    bool (*setCapacity)(AthenaContentStoreImplementation *store, size_t maxSizeInMB);

    /** @see athenaContentStore_GetCapacity */
    size_t (*getCapacity)(AthenaContentStoreImplementation *store);

    /** @see athenaContentStore_ProcessMessage */
    CCNxMetaMessage *(*processMessage)(AthenaContentStoreImplementation *store, const CCNxMetaMessage *message);

} AthenaContentStoreInterface;

#endif
