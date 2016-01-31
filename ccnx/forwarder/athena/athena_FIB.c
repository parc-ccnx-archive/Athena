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
#include <config.h>

#include <ccnx/forwarder/athena/athena.h>
#include <parc/algol/parc_BitVector.h>
#include <parc/algol/parc_HashMap.h>
#include <parc/algol/parc_TreeRedBlack.h>

#include <ccnx/forwarder/athena/athena_FIB.h>

/**
 * @typedef AthenaFIB
 * @brief FIB tables, tableByName (KEY == CCNxName, VALUE == PARCBitVector
 *                    listOfLinks (List ( index = linkId ) of lists (CCNxNames))
 */
struct athena_FIB {
    PARCHashMap *tableByName;
    PARCList *listOfLinks;
    PARCBitVector *defaultRoute;
};

/**
 * @typedef AthenaFIBListEntry
 * @brief Element for FIB table entry list
 */
struct athena_FIB_list_entry {
    CCNxName *name;
    int linkId;
};

CCNxName *
athenaFIBListEntry_GetName(AthenaFIBListEntry *entry)
{
    return entry->name;
}

int
athenaFIBListEntry_GetLinkId(AthenaFIBListEntry *entry)
{
    return entry->linkId;
}


static void
_athenaFIB_Destroy(AthenaFIB **fib)
{
    AthenaFIB *pFib = *fib;
    parcHashMap_Release(&pFib->tableByName);
    parcList_Release(&pFib->listOfLinks);
    if (pFib->defaultRoute != NULL) {
        parcBitVector_Release(&pFib->defaultRoute);
    }
}

parcObject_ExtendPARCObject(AthenaFIB, _athenaFIB_Destroy, NULL, NULL, NULL, NULL, NULL, NULL);

parcObject_ImplementAcquire(athenaFIB, AthenaFIB);

parcObject_ImplementRelease(athenaFIB, AthenaFIB);

AthenaFIB *
athenaFIB_Create()
{
    AthenaFIB *newFIB = parcObject_CreateInstance(AthenaFIB);
    if (newFIB != NULL) {
        newFIB->listOfLinks = parcList(parcArrayList_Create((void (*)(void**))parcList_Release), PARCArrayListAsPARCList);
        newFIB->tableByName = parcHashMap_Create();
        newFIB->defaultRoute = NULL;
    }

    return newFIB;
}

PARCBitVector *
athenaFIB_Lookup(AthenaFIB *athenaFIB, const CCNxName *ccnxName)
{
    CCNxName *name = ccnxName_Copy(ccnxName);
    PARCBitVector *result = (PARCBitVector *) parcHashMap_Get(athenaFIB->tableByName, (PARCObject *) name);

    while ((ccnxName_GetSegmentCount(name) > 0) && (result == NULL)) {
        name = ccnxName_Trim(name, 1);
        result = (PARCBitVector *) parcHashMap_Get(athenaFIB->tableByName, (PARCObject *) name);
    }
    ccnxName_Release(&name);

    if (result == NULL) {
        result = athenaFIB->defaultRoute;
    }

    return result;
}

bool
athenaFIB_AddRoute(AthenaFIB *athenaFIB, const CCNxName *ccnxName, const PARCBitVector *ccnxLinkVector)
{
    PARCBitVector *linkV = NULL;

    // Check if the is a mapping for the default route
    if (ccnxName_GetSegmentCount(ccnxName) == 1) {
        CCNxNameSegment *segment = ccnxName_GetSegment(ccnxName, 0);
        if ((ccnxNameSegment_GetType(segment) == CCNxNameLabelType_NAME) &&
            (ccnxNameSegment_Length(segment) == 0)) {
            if (athenaFIB->defaultRoute == NULL) {
                athenaFIB->defaultRoute = parcBitVector_Create();
            }
            linkV = athenaFIB->defaultRoute;
        }
    }

    if (linkV == NULL) { // It's not the default link
        // for each bit in the link vector, add an entry for the name in the list of links for future
        // cleanup
        for (int i = 0, bit = 0; i < parcBitVector_NumberOfBitsSet(ccnxLinkVector); ++i, ++bit) {
            bit = parcBitVector_NextBitSet(ccnxLinkVector, bit);
            if (bit >= parcList_Size(athenaFIB->listOfLinks)) {
                //Expand the list if needed
                for (size_t j = parcList_Size(athenaFIB->listOfLinks); j <= bit; ++j) {
                    parcList_Add(athenaFIB->listOfLinks, NULL);
                }
            }
            PARCList *nameList = parcList_GetAtIndex((PARCList *) athenaFIB->listOfLinks, bit);
            if (nameList == NULL) {
                nameList = parcList(parcArrayList_Create((void (*)(void **))ccnxName_Release), PARCArrayListAsPARCList);
                parcList_SetAtIndex(athenaFIB->listOfLinks, bit, (PARCObject *) nameList);
            }
            parcList_Add(nameList, (PARCObject *) ccnxName_Acquire(ccnxName));
        }

        // Now add the actual fib mapping
        linkV = (PARCBitVector *) parcHashMap_Get(athenaFIB->tableByName, (PARCObject *) ccnxName);
        if (linkV == NULL) {
            PARCBitVector *newLinkV = parcBitVector_Create();
            linkV = newLinkV;
            parcHashMap_Put(athenaFIB->tableByName, (PARCObject *) ccnxName, (PARCObject *) newLinkV);
            parcBitVector_Release(&newLinkV);
        }
    }

    parcBitVector_SetVector(linkV, ccnxLinkVector);

    return true;
}

bool
athenaFIB_DeleteRoute(AthenaFIB *athenaFIB, const CCNxName *ccnxName, const PARCBitVector *ccnxLinkVector)
{
    bool result = false;

    PARCBitVector *linkV = athenaFIB_Lookup(athenaFIB, ccnxName);
    if (linkV != NULL) {
        parcBitVector_ClearVector(linkV, ccnxLinkVector);
        if (parcBitVector_NumberOfBitsSet(linkV) == 0) {
            parcHashMap_Remove(athenaFIB->tableByName, (PARCObject *) ccnxName);
        }
        result = true;
    }

    return result;
}

bool
athenaFIB_RemoveLink(AthenaFIB *athenaFIB, const PARCBitVector *ccnxLinkVector)
{
    bool result = true;

    for (int i = 0, bit = 0; i < parcBitVector_NumberOfBitsSet(ccnxLinkVector); ++i) {
        bit = parcBitVector_NextBitSet(ccnxLinkVector, bit);
        if (parcList_Size(athenaFIB->listOfLinks) <= bit) {
            break;
        }
        PARCList *nameList = parcList_GetAtIndex(athenaFIB->listOfLinks, bit);
        if (nameList != NULL) {
            for (int j = 0; j < parcList_Size(nameList); ++j) {
                CCNxName *key = (CCNxName *) parcList_GetAtIndex(nameList, j);
                athenaFIB_DeleteRoute(athenaFIB, key, ccnxLinkVector);
            }
            parcList_Clear(nameList);
            result = true;
        }
    }

    return result;
}

static void
_athenaFIBListEntry_Destroy(AthenaFIBListEntry **entryHandle)
{
    AthenaFIBListEntry *entry = *entryHandle;
    if (entry->name != NULL) {
        ccnxName_Release(&entry->name);
    }
}

parcObject_ExtendPARCObject(AthenaFIBListEntry, _athenaFIBListEntry_Destroy, NULL, NULL, NULL, NULL, NULL, NULL);

static
parcObject_ImplementRelease(_athenaFIBListEntry, AthenaFIBListEntry);

static AthenaFIBListEntry *
_athenaFIBListEntry_Create(const CCNxName *name, int linkId)
{
    AthenaFIBListEntry *entry = parcObject_CreateInstance(AthenaFIBListEntry);

    if (entry != NULL) {
        entry->name = ccnxName_Acquire(name);
        entry->linkId = linkId;
    }

    return entry;
}


PARCList *
athenaFIB_CreateEntryList(AthenaFIB *athenaFIB)
{
    PARCList *result =
        parcList(parcArrayList_Create((void (*)(void **))_athenaFIBListEntry_Release), PARCArrayListAsPARCList);

    for (size_t i = 0; i < parcList_Size(athenaFIB->listOfLinks); ++i) {
        PARCList *linksForId = parcList_GetAtIndex(athenaFIB->listOfLinks, i);
        if (linksForId != NULL) {
            for (size_t j = 0; j < parcList_Size(linksForId); ++j) {
                CCNxName *name = parcList_GetAtIndex(linksForId, j);
                AthenaFIBListEntry *entry = _athenaFIBListEntry_Create(name, (int) i);
                parcList_Add(result, entry);
            }
        }
    }
    return result;
}

CCNxMetaMessage *
athenaFIB_ProcessMessage(AthenaFIB *athenaFIB, const CCNxMetaMessage *message)
{
    CCNxMetaMessage *responseMessage = NULL;
    return responseMessage;
}
