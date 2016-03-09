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

#include <ccnx/forwarder/athena/athena.h>
#include <parc/algol/parc_DisplayIndented.h>

#include <parc/algol/parc_HashMap.h>
#include <parc/algol/parc_SortedList.h>
#include <parc/algol/parc_Clock.h>

#include <ccnx/common/ccnx_NameSegmentNumber.h>

#include <ccnx/forwarder/athena/athena_LRUContentStore.h>


typedef struct athena_lrucontentstore_entry _AthenaLRUContentStoreEntry;

struct AthenaLRUContentStore {
    PARCClock *wallClock;

    size_t maxSizeInBytes;
    size_t currentSizeInBytes;
    uint64_t numEntries;

    _AthenaLRUContentStoreEntry *lruHead;  // entry that was most recently used
    _AthenaLRUContentStoreEntry *lruTail;  // entry that was least recently used

    PARCHashMap *tableByName;
    PARCHashMap *tableByNameAndKeyId;
    PARCHashMap *tableByNameAndObjectHash;

    PARCSortedList *listByRecommendedCacheTime;
    PARCSortedList *listByExpiryTime;

    struct {
        uint64_t numAdds;
        uint64_t numRemoves;
        uint64_t numMatchHits;
        uint64_t numMatchMisses;
        uint64_t numRemovedByLRU;
        uint64_t numRemovedByExpiration;
        uint64_t numRemovedByRCT;
    } stats;
};

static size_t
_calculateSizeOfContentObject(const CCNxContentObject *contentObject)
{
    size_t result = 0;
    char *nameAsString = ccnxName_ToString(ccnxContentObject_GetName(contentObject));
    result += strlen(nameAsString);
    parcMemory_DeallocateImpl((void **) &nameAsString);

    PARCBuffer *payload = ccnxContentObject_GetPayload(contentObject);
    if (payload != NULL) {
        result += parcBuffer_Limit(payload);
    }
    return result;
}

static PARCObject *
_createHashableKey(const CCNxName *name, const PARCBuffer *keyId, const PARCBuffer *contentObjectHash)
{
    PARCBufferComposer *keyComposer = parcBufferComposer_Create();

    keyComposer = ccnxName_BuildString(name, keyComposer);

    if (keyId != NULL) {
        parcBufferComposer_PutBuffer(keyComposer, keyId);
    }

    if (contentObjectHash != NULL) {
        parcBufferComposer_PutBuffer(keyComposer, contentObjectHash);
    }

    PARCBuffer *result = parcBufferComposer_ProduceBuffer(keyComposer);
    parcBufferComposer_Release(&keyComposer);

    return result;
}


static void _athenaLRUContentStore_PurgeContentStoreEntry(AthenaLRUContentStore *store,
                                                          _AthenaLRUContentStoreEntry *storeEntry);

/***************************************************************************************************
*   Begin AthenaLRUContentStoreEntry definition.
***************************************************************************************************/

//
// This is the primary reference to all of the created subclassed items, and is stored in the LRU.
struct athena_lrucontentstore_entry {
    AthenaContentStoreInterface *storeImpl;

    CCNxContentObject *contentObject;

    int indexCount; // How many 'tableBy<X>' indexes does this entry appear in.

    size_t sizeInBytes;

    bool hasExpiryTime;
    uint64_t expiryTime;

    bool hasRecommendedCacheTime;
    uint64_t recommendedCacheTime;

    bool hasKeyId;
    PARCBuffer *keyId;

    bool hasContentObjectHash;
    PARCBuffer *contentObjectHash;

    _AthenaLRUContentStoreEntry *next;
    _AthenaLRUContentStoreEntry *prev;
};

static void
_athenaLRUContentStoreEntry_Finalize(_AthenaLRUContentStoreEntry **entryPtr)
{
    _AthenaLRUContentStoreEntry *entry = (_AthenaLRUContentStoreEntry *) *entryPtr;
    //printf("LRUContentStoreEntry being finalized.  %p\n", entry);
    ccnxContentObject_Release(&entry->contentObject);

    if (entry->keyId) {
        parcBuffer_Release(&entry->keyId);
    }

    if (entry->contentObjectHash) {
        parcBuffer_Release(&entry->contentObjectHash);
    }
}

static void
_athenaLRUContentStoreEntry_Display(const _AthenaLRUContentStoreEntry *entry, int indentation)
{
    CCNxName *name = ccnxContentObject_GetName(entry->contentObject);
    char *nameString = ccnxName_ToString(name);
    int childIndentation = indentation + 2; //strlen("AthenaLRUContentStoreEntry");
    parcDisplayIndented_PrintLine(indentation,
                                  "AthenaLRUContentStoreEntry {%p, prev = %p, next = %p, co = %p, size = %zu",
                                  entry, entry->prev, entry->next,
                                  entry->contentObject, entry->sizeInBytes);
    parcDisplayIndented_PrintLine(childIndentation, "Name: %p [%s]", name, nameString);
    if (entry->hasExpiryTime) {
        parcDisplayIndented_PrintLine(childIndentation, "ExpiryTime: [%"
                                      PRIu64
                                      "]", entry->expiryTime);
    }

    if (entry->hasRecommendedCacheTime) {
        parcDisplayIndented_PrintLine(childIndentation, "RecommendedCacheTime: [%"
                                      PRIu64
                                      "]", entry->recommendedCacheTime);
    }

    parcDisplayIndented_PrintLine(childIndentation, "}");
    parcMemory_Deallocate(&nameString);
}

static
parcObject_ImplementAcquire(_athenaLRUContentStoreEntry, _AthenaLRUContentStoreEntry
                            );

static
parcObject_ImplementRelease(_athenaLRUContentStoreEntry, _AthenaLRUContentStoreEntry
                            );

// _destroy, _copy, _toString, _equals, _compare, _hashCode, _toJSON
parcObject_ExtendPARCObject(_AthenaLRUContentStoreEntry,
                            _athenaLRUContentStoreEntry_Finalize,
                            NULL, // copy
                            NULL, // toString
                            NULL, // equals,
                            NULL, // compare
                            NULL, // hashCode
                            NULL // toJSON
                            );


static _AthenaLRUContentStoreEntry *
_athenaLRUContentStoreEntry_Create(const CCNxContentObject *contentObject)
{
    _AthenaLRUContentStoreEntry *result = parcObject_CreateAndClearInstance(_AthenaLRUContentStoreEntry);

    if (result != NULL) {
        result->contentObject = ccnxContentObject_Acquire(contentObject);
        result->next = NULL;
        result->prev = NULL;
        result->sizeInBytes = _calculateSizeOfContentObject(contentObject);
        result->hasExpiryTime = false;
        result->hasRecommendedCacheTime = false;

        if (ccnxContentObject_HasExpiryTime(contentObject)) {
            result->hasExpiryTime = true;
            result->expiryTime = ccnxContentObject_GetExpiryTime(contentObject);
        }

        // TODO:
        // Check if the CO has an RCT and set the flags and init the values appropriately.
        result->hasRecommendedCacheTime = false;

        // TODO:
        // Check if the CO has a KeyId and set the fields appropriately.
        result->hasKeyId = false;

        // TODO:
        // Calculate the CO's contentObjectHash and set the fields appropriately
        result->hasContentObjectHash = false;
    }
    return result;
}


static int
_compareByExpiryTime(_AthenaLRUContentStoreEntry *entry1, _AthenaLRUContentStoreEntry *entry2)
{
    int result = 0;
    if (entry1->hasExpiryTime && !entry2->hasExpiryTime) {
        result = -1;
    } else if (!entry1->hasExpiryTime && entry2->hasExpiryTime) {
        result = 1;
    } else if (!entry1->hasExpiryTime && !entry2->hasExpiryTime) {
        result = 0;
    } else {
        // Both have expiryTimes
        if (entry1->expiryTime == entry2->expiryTime) {
            result = 0;
        } else {
            result = entry1->expiryTime < entry2->expiryTime ? -1 : 1;
        }
    }
    return result;
}

static int
_compareByRecommendedCacheTime(const _AthenaLRUContentStoreEntry *entry1, const _AthenaLRUContentStoreEntry *entry2)
{
    int result = 0;
    if (entry1->hasRecommendedCacheTime && !entry2->hasRecommendedCacheTime) {
        result = -1;
    } else if (!entry1->hasRecommendedCacheTime && entry2->hasRecommendedCacheTime) {
        result = 1;
    } else if (!entry1->hasRecommendedCacheTime && !entry2->hasRecommendedCacheTime) {
        result = 0;
    } else {
        // Both have recommended cache times
        if (entry1->recommendedCacheTime == entry2->recommendedCacheTime) {
            result = 0;
        } else {
            result = entry1->recommendedCacheTime < entry2->recommendedCacheTime ? -1 : 1;
        }
    }
    return result;
}


/***************************************************************************************************
*   End AthenaLRUContentStoreEntry definition.
***************************************************************************************************/

static void
_athenaLRUContentStore_RemoveContentStoreEntryFromLRU(AthenaLRUContentStore *impl,
                                                      _AthenaLRUContentStoreEntry *storeEntry)
{
    if (storeEntry->next != NULL) {
        storeEntry->next->prev = storeEntry->prev;
    }

    if (storeEntry->prev != NULL) {
        storeEntry->prev->next = storeEntry->next;
    }

    if (impl->lruHead == storeEntry) {
        impl->lruHead = storeEntry->next;   // Could be NULL
    }

    if (impl->lruTail == storeEntry) {
        impl->lruTail = storeEntry->prev;   // Could be NULL;
    }

    _athenaLRUContentStoreEntry_Release(&storeEntry);
}

static void
_athenaLRUContentStore_PurgeContentStoreEntry(AthenaLRUContentStore *impl, _AthenaLRUContentStoreEntry *storeEntry)
{
    PARCObject *nameKey = _createHashableKey(ccnxContentObject_GetName(storeEntry->contentObject), NULL, NULL);
    parcHashMap_Remove(impl->tableByName, nameKey);
    parcObject_Release((PARCObject **) &nameKey);

    if (storeEntry->hasKeyId) {
        PARCObject *nameAndKeyIdKey = _createHashableKey(ccnxContentObject_GetName(storeEntry->contentObject),
                                                         storeEntry->keyId, NULL);
        parcHashMap_Remove(impl->tableByNameAndKeyId, nameAndKeyIdKey);
        parcObject_Release((PARCObject **) &nameAndKeyIdKey);
    }

    if (storeEntry->hasContentObjectHash) {
        PARCObject *nameAndContentObjectHashKey = _createHashableKey(
            ccnxContentObject_GetName(storeEntry->contentObject), NULL, storeEntry->contentObjectHash);
        parcHashMap_Remove(impl->tableByNameAndObjectHash, nameAndContentObjectHashKey);
        parcObject_Release((PARCObject **) &nameAndContentObjectHashKey);
    }

    parcSortedList_Remove(impl->listByExpiryTime, storeEntry);

    parcSortedList_Remove(impl->listByRecommendedCacheTime, storeEntry);

    impl->currentSizeInBytes -= storeEntry->sizeInBytes;

    _athenaLRUContentStore_RemoveContentStoreEntryFromLRU(impl, storeEntry);

    impl->numEntries--;
}

/**
 * Release all of the AthenaLRUContentStoreEntry items in the store's LRU.
 */
static void
_athenaLRUContentStoreEntry_ReleaseAllInLRU(AthenaLRUContentStore *impl)
{
    _AthenaLRUContentStoreEntry *entry = impl->lruHead;
    while (entry != NULL) {
        _AthenaLRUContentStoreEntry *next = entry->next;
        _athenaLRUContentStoreEntry_Release(&entry);
        entry = next;
    }
}

static void
_athenaLRUContentStore_ReleaseAllData(AthenaLRUContentStore *impl)
{
    if (impl->tableByName) {
        parcHashMap_Release(&impl->tableByName);
    }
    if (impl->tableByNameAndKeyId) {
        parcHashMap_Release(&impl->tableByNameAndKeyId);
    }
    if (impl->tableByNameAndObjectHash) {
        parcHashMap_Release(&impl->tableByNameAndObjectHash);
    }

    if (impl->listByExpiryTime) {
        parcSortedList_Release(&impl->listByExpiryTime);
    }
    if (impl->listByRecommendedCacheTime) {
        parcSortedList_Release(&impl->listByRecommendedCacheTime);
    }

    _athenaLRUContentStoreEntry_ReleaseAllInLRU(impl);

    impl->lruHead = NULL;
    impl->lruTail = NULL;
    impl->currentSizeInBytes = 0;
    impl->numEntries = 0;
}

static void
_athenaLRUContentStore_Finalize(AthenaLRUContentStore **instancePtr)
{
    assertNotNull(instancePtr, "Parameter must be a non-null pointer to a AthenaLRUContentStore pointer.");

    AthenaLRUContentStore *impl = *instancePtr;

    _athenaLRUContentStore_ReleaseAllData(impl);
}


parcObject_ImplementAcquire(athenaLRUContentStore, AthenaLRUContentStore
                            );

parcObject_ExtendPARCObject(AthenaLRUContentStore,
                            _athenaLRUContentStore_Finalize,
                            NULL, // Copy
                            NULL, // ToString
                            NULL, // Equals
                            NULL, // compare
                            NULL, // hashCode
                            NULL // toJSON
                            );

void
athenaLRUContentStore_AssertValid(const AthenaLRUContentStore *instance)
{
    assertTrue(athenaLRUContentStore_IsValid(instance),
               "AthenaLRUContentStore is not valid.");
}

static unsigned int
_calculateNumberOfInitialBucketsBasedOnCapacityInBytes(size_t capacityInBytes)
{
    // **********************************************************************
    // Note!! This is a temporary workaround until PARCHashMap implements
    // load factor resizing.
    // **********************************************************************

    // We're using the very rough heuristic of allowing up to 100
    // ContentObjects in each bucket of the HashMap, assuming each
    // ContentObject is 1KB in size and the hash function works well...

    unsigned int parcHashMapDefaultBuckets = 43; // THIS IS TEMPORARY UNTIL PARCHashMap implements load factor
    unsigned int numBuckets = capacityInBytes / (100 * 1024); // 100 1K objects per bucket
    return numBuckets < parcHashMapDefaultBuckets ? parcHashMapDefaultBuckets : numBuckets;
}

static void
_athenaLRUContentStore_initializeIndexes(AthenaLRUContentStore *impl, size_t capacityInBytes)
{
    //
    // NOTE: Calculating the number of buckets for the hashmaps is a temporary workaround for
    //       PARCHashMap not yet implementing internal resizing. See BugzId: 3950
    //
    unsigned int numBuckets = _calculateNumberOfInitialBucketsBasedOnCapacityInBytes(capacityInBytes);
    impl->tableByName = parcHashMap_CreateCapacity(numBuckets);
    impl->tableByNameAndKeyId = parcHashMap_CreateCapacity(numBuckets);
    impl->tableByNameAndObjectHash = parcHashMap_CreateCapacity(numBuckets);

    impl->listByRecommendedCacheTime = parcSortedList_CreateCompare(
        (PARCSortedListEntryCompareFunction) _compareByRecommendedCacheTime);
    impl->listByExpiryTime = parcSortedList_CreateCompare((PARCSortedListEntryCompareFunction) _compareByExpiryTime);

    impl->lruHead = NULL;
    impl->lruTail = NULL;

    impl->currentSizeInBytes = 0;
}

static AthenaContentStoreImplementation *
_athenaLRUContentStore_Create(AthenaContentStoreConfig *storeConfig)
{
    AthenaLRUContentStoreConfig *config = (AthenaLRUContentStoreConfig *) storeConfig;
    AthenaLRUContentStore *result = parcObject_CreateAndClearInstance(AthenaLRUContentStore);
    if (result != NULL) {
        result->wallClock = parcClock_Wallclock();

        if (config != NULL) {
            result->maxSizeInBytes = config->capacityInMB * (1024 * 1024); // MB to bytes
        } else {
            result->maxSizeInBytes = 10 * (1024 * 1024); // 10 MB default
        }

        _athenaLRUContentStore_initializeIndexes(result, result->maxSizeInBytes);
    }

    return (AthenaContentStoreImplementation *) result;
}

static void
_athenaLRUContentStore_Release(AthenaContentStoreImplementation **instance)
{
    parcObject_Release((PARCObject **) instance);
}

void
athenaLRUContentStore_Display(const AthenaContentStoreImplementation *store, int indentation)
{
    AthenaLRUContentStore *impl = (AthenaLRUContentStore *) store;

    parcDisplayIndented_PrintLine(indentation, "AthenaLRUContentStore @ %p {", impl);
    parcDisplayIndented_PrintLine(indentation + 4, "maxSizeInBytes = %zu", impl->maxSizeInBytes);
    parcDisplayIndented_PrintLine(indentation + 4, "sizeInBytes = %zu", impl->currentSizeInBytes);
    parcDisplayIndented_PrintLine(indentation + 4, "numEntriesInStore = %zu", impl->numEntries);
    parcDisplayIndented_PrintLine(indentation + 4, "numEntriesInNameIndex = %zu", parcHashMap_Size(impl->tableByName));
    parcDisplayIndented_PrintLine(indentation + 4, "numEntriesInName+KeyIndex = %zu",
                                  parcHashMap_Size(impl->tableByNameAndKeyId));
    parcDisplayIndented_PrintLine(indentation + 4, "numEntriesInName+HashIndex = %zu",
                                  parcHashMap_Size(impl->tableByNameAndObjectHash));

    parcDisplayIndented_PrintLine(indentation + 4, "LRU = {");
    _AthenaLRUContentStoreEntry *entry = impl->lruHead;  // Dump entries, head to tail
    while (entry) {
        _athenaLRUContentStoreEntry_Display(entry, indentation + 8);
        entry = entry->next;
    }
    parcDisplayIndented_PrintLine(indentation + 4, "}");
    parcDisplayIndented_PrintLine(indentation, "}");
}

bool
athenaLRUContentStore_IsValid(const AthenaLRUContentStore *instance)
{
    bool result = false;

    if (instance != NULL) {
        result = true;
    }

    return result;
}

char *
athenaLRUContentStore_ToString(const AthenaLRUContentStore *instance)
{
    char *result = parcMemory_Format("AthenaLRUContentStore@%p\n", instance);

    return result;
}

static void
_addContentStoreEntryToLRUHead(AthenaLRUContentStore *impl, _AthenaLRUContentStoreEntry *entry)
{
    trapUnexpectedStateIf(impl->lruHead != NULL && impl->lruHead->prev != NULL,
                          "Unexpected LRU pointer configuration. Next should be NULL.");

    if (impl->lruTail == NULL) {
        impl->lruTail = entry;
    }

    if (impl->lruHead != NULL) {
        impl->lruHead->prev = entry;
    }

    entry->next = impl->lruHead;
    entry->prev = NULL;

    impl->lruHead = entry;
}

/**
 * Called to move an LRU entry to the top of the list. Say, when it's been succesfully searched for and retrieved.
 * Items at the bottom of the list are expired from the list first, so moving to the top keeps it alive in the LRU
 * longer.
 */
static void
_moveContentStoreEntryToLRUHead(AthenaLRUContentStore *impl, _AthenaLRUContentStoreEntry *entry)
{
    if (impl->lruHead == entry) {
        return; // we're done.
    }

    if (entry->prev != NULL) {
        entry->prev->next = entry->next;
    }

    if (entry->next != NULL) {
        entry->next->prev = entry->prev;
    }

    if (impl->lruTail == entry) {
        impl->lruTail = entry->prev;
        impl->lruTail->next = NULL;
    }

    if (impl->lruHead == NULL) {
        entry->next = NULL;
        entry->prev = NULL;
    } else {
        impl->lruHead->prev = entry;
        entry->next = impl->lruHead;
    }
    impl->lruHead = entry;
    entry->prev = NULL;
}

static _AthenaLRUContentStoreEntry *
_getLeastUsedFromLRU(AthenaLRUContentStore *impl)
{
    // The TAIL of the LRU is the oldest, least used object.
    return impl->lruTail;
}

static _AthenaLRUContentStoreEntry *
_getEarliestExpiryTime(AthenaLRUContentStore *impl)
{
    _AthenaLRUContentStoreEntry *result = NULL;

    if (parcSortedList_Size(impl->listByExpiryTime) > 0) {
        result = parcSortedList_GetAtIndex(impl->listByExpiryTime, 0);
    }
    return result;
}

static _AthenaLRUContentStoreEntry *
_getEarliestRecommendedCacheTime(AthenaLRUContentStore *impl)
{
    _AthenaLRUContentStoreEntry *result = NULL;

    if (parcSortedList_Size(impl->listByRecommendedCacheTime) > 0) {
        result = parcSortedList_GetAtIndex(impl->listByRecommendedCacheTime, 0);
    }
    return result;
}

static bool
_makeRoomInStore(AthenaLRUContentStore *impl, size_t sizeNeeded)
{
    bool result = false;

    if (sizeNeeded > impl->maxSizeInBytes) {
        return false; // not possible.
    }

    uint64_t nowInMillis = parcClock_GetTime(impl->wallClock);

    // Evict expired items until we have enough room, or don't have any expired items.
    while (sizeNeeded > (impl->maxSizeInBytes - impl->currentSizeInBytes)) {
        _AthenaLRUContentStoreEntry *entry = _getEarliestExpiryTime(impl);
        if (entry == NULL) {
            break;
        }
        if (nowInMillis > entry->expiryTime) {
            _athenaLRUContentStore_PurgeContentStoreEntry(impl, entry);
        } else {
            break;
        }
    }

    // Evict items past their recommended cache time until we have enough room, or don't have any items.
    while (sizeNeeded > (impl->maxSizeInBytes - impl->currentSizeInBytes)) {
        _AthenaLRUContentStoreEntry *entry = _getEarliestRecommendedCacheTime(impl);
        if (entry == NULL) {
            break;
        }
        if (nowInMillis > entry->recommendedCacheTime) {
            _athenaLRUContentStore_PurgeContentStoreEntry(impl, entry);
        } else {
            break;
        }
    }

    while (sizeNeeded > (impl->maxSizeInBytes - impl->currentSizeInBytes)) {
        _AthenaLRUContentStoreEntry *entry = _getLeastUsedFromLRU(impl);
        if (entry == NULL) {
            break;
        }
        _athenaLRUContentStore_PurgeContentStoreEntry(impl, entry);
    }

    if (impl->maxSizeInBytes - impl->currentSizeInBytes > sizeNeeded) {
        return true;
    }

    return result;
}

/**
 * Add an entry to an index table, returning the entry that would be replaced (if any).
 */
static _AthenaLRUContentStoreEntry *
_addEntryToIndexTableIfNotAlreadyInIt(PARCHashMap *indexTable, PARCObject *key, _AthenaLRUContentStoreEntry *entry)
{
    // Check to see if it's already in the table.
    _AthenaLRUContentStoreEntry *existingEntry = (_AthenaLRUContentStoreEntry *) parcHashMap_Get(indexTable, key);
    if (existingEntry != NULL) {
        // There is an existing entry in this table for this key. Note that it will no longer be in the tableByName
        // after we add the new one. (The hashMap silently replaces it, after calling _Release on it).
        existingEntry->indexCount--;
    }

    // Place the new entry in the index table.
    parcHashMap_Put(indexTable, key, entry);
    entry->indexCount += 1;

    return existingEntry;
}


static bool
_athenaLRUContentStore_PutLRUContentStoreEntry(AthenaContentStoreImplementation *store,
                                               const _AthenaLRUContentStoreEntry *entry)
{
    bool result = false;

    AthenaLRUContentStore *impl = (AthenaLRUContentStore *) store;

    // Enforce capacity limit. If adding the next store item would put us over the limit, we have to remove
    // entrie(s) until there is room.

    bool isEnoughRoomInStore = true;
    if ((entry->sizeInBytes + impl->currentSizeInBytes) > impl->maxSizeInBytes) {
        isEnoughRoomInStore = _makeRoomInStore(impl, entry->sizeInBytes);
    }

    if (isEnoughRoomInStore) {
        _AthenaLRUContentStoreEntry *newEntry = _athenaLRUContentStoreEntry_Acquire(entry);

        // New entries go to the HEAD of the LRU.
        _addContentStoreEntryToLRUHead(impl, newEntry);

        // DO NOT RELEASE the newEntry after adding it to the containers. We will let the LRU be responsible for the
        // final release. At this point, the reference count of the new LRUContentStoreEntry is 1 and the LRU
        // points to it. The other containers (hashmap indices, sorted lists, etc) can acquire and release on their
        // own, but the LRU holds the final reference.

        _AthenaLRUContentStoreEntry *existingEntry = NULL;
        CCNxName *name = ccnxContentObject_GetName(newEntry->contentObject);
        if (name != NULL) {
            PARCObject *nameKey = _createHashableKey(name, NULL, NULL);
            existingEntry = _addEntryToIndexTableIfNotAlreadyInIt(impl->tableByName, nameKey, newEntry);
            parcObject_Release((PARCObject **) &nameKey);
        }

        if (newEntry->hasKeyId) {
            PARCObject *nameAndKeyIdKey = _createHashableKey(name, newEntry->keyId, NULL);
            existingEntry = _addEntryToIndexTableIfNotAlreadyInIt(impl->tableByNameAndKeyId, nameAndKeyIdKey, newEntry);
            parcObject_Release((PARCObject **) &nameAndKeyIdKey);
        }

        if (newEntry->hasContentObjectHash) {
            PARCObject *nameAndObjectHashKey = _createHashableKey(name, NULL, newEntry->contentObjectHash);
            existingEntry = _addEntryToIndexTableIfNotAlreadyInIt(impl->tableByNameAndObjectHash, nameAndObjectHashKey,
                                                                  newEntry);
            parcObject_Release((PARCObject **) &nameAndObjectHashKey);
        }

        if (existingEntry != NULL && existingEntry->indexCount < 1) {
            // The existing entry is in no indexes, which means it cannot be matched and serves no further purpose.
            // Remove it completely from all containers.
            //printf("Releasing previously held store entry\n");
            _athenaLRUContentStore_PurgeContentStoreEntry(impl, existingEntry);
        }

        // Add the new entry to the time-ordered lists, if it has an RCT or ExpiryTime.
        if (newEntry->hasExpiryTime) {
            parcSortedList_Add(impl->listByExpiryTime, newEntry);
        }

        if (newEntry->hasRecommendedCacheTime) {
            parcSortedList_Add(impl->listByRecommendedCacheTime, newEntry);
        }

        impl->stats.numAdds++;
        impl->numEntries++;
        impl->currentSizeInBytes += newEntry->sizeInBytes;
        result = true;
    }

    return result;
}

static bool
_athenaLRUContentStore_PutContentObject(AthenaContentStoreImplementation *store, const CCNxContentObject *content)
{
    AthenaLRUContentStore *impl = (AthenaLRUContentStore *) store;

    // Check to see if the ContentObject is expired. If so, don't bother to cache it.
    if (ccnxContentObject_HasExpiryTime(content)) {
        if (ccnxContentObject_GetExpiryTime(content) <= parcClock_GetTime(impl->wallClock)) {
            // the expiration time is now or earlier, so don't bother adding it.
            return false;
        }
    }

    _AthenaLRUContentStoreEntry *newEntry = _athenaLRUContentStoreEntry_Create(content);

    bool result = _athenaLRUContentStore_PutLRUContentStoreEntry(store, newEntry);

    if (result == false) {
        // We didn't have enough room in the store to add the new item.
    }

    _athenaLRUContentStoreEntry_Release(&newEntry);

    return result;
}

static CCNxContentObject *
//_athenaLRUContentStore_GetMatch(AthenaContentStoreImplementation *store, const CCNxName *name, const PARCBuffer *keyIdRestriction,
//                                const PARCBuffer *contentObjectHash)
_athenaLRUContentStore_GetMatch(AthenaContentStoreImplementation *store, const CCNxInterest *interest)
{
    CCNxContentObject *result = NULL;
    AthenaLRUContentStore *impl = (AthenaLRUContentStore *) store;
    _AthenaLRUContentStoreEntry *entry = NULL;

    CCNxName *name = ccnxInterest_GetName(interest);
    PARCBuffer *contentObjectHashRestriction = ccnxInterest_GetContentObjectHashRestriction(interest);
    PARCBuffer *keyIdRestriction = ccnxInterest_GetKeyIdRestriction(interest);

    if (contentObjectHashRestriction != NULL) {
        PARCObject *nameAndHashKey = _createHashableKey(name, NULL, contentObjectHashRestriction);
        entry = (_AthenaLRUContentStoreEntry *) parcHashMap_Get(impl->tableByNameAndObjectHash, nameAndHashKey);
        parcObject_Release((PARCObject **) &nameAndHashKey);
    }

    if ((entry == NULL) && (keyIdRestriction != NULL)) {
        PARCObject *nameAndKeyIdKey = _createHashableKey(name, keyIdRestriction, NULL);
        entry = (_AthenaLRUContentStoreEntry *) parcHashMap_Get(impl->tableByNameAndKeyId, nameAndKeyIdKey);
        parcObject_Release((PARCObject **) &nameAndKeyIdKey);
    }

    if (entry == NULL) {
        PARCObject *nameKey = _createHashableKey(name, NULL, NULL);
        entry = (_AthenaLRUContentStoreEntry *) parcHashMap_Get(impl->tableByName, nameKey);
        parcObject_Release((PARCObject **) &nameKey);
    }

    // Matching is done. Now check for validity, if necessary.

    if (entry != NULL) {
        // We found matching content. Now make sure it's not expired before returning it. If it is expired,
        // remove it from the store and don't return anything.
        if (entry->hasExpiryTime && (entry->expiryTime < parcClock_GetTime(impl->wallClock))) {
            _athenaLRUContentStore_PurgeContentStoreEntry(impl, entry);
            entry = NULL;
        }

        // XXX: TODO: Check that the KeyId, if any, was verified.
    }

    // At this point, the cached content is considered valid for responding with. Return it.
    if (entry != NULL) {
        result = entry->contentObject;

        // Update LRU so that the matched entry is at the top of the list.
        _moveContentStoreEntryToLRUHead(impl, entry);

        impl->stats.numMatchHits++;
    } else {
        impl->stats.numMatchMisses++;
    }

    return result;
}

static bool
_athenaLRUContentStore_RemoveMatch(AthenaContentStoreImplementation *store, const CCNxName *name,
                                   const PARCBuffer *keyIdRestriction, const PARCBuffer *contentObjectHash)
{
    AthenaLRUContentStore *impl = (AthenaLRUContentStore *) store;
    bool wasRemoved = false;

    if (contentObjectHash != NULL) {
        PARCObject *nameAndHashKey = _createHashableKey(name, NULL, contentObjectHash);
        _AthenaLRUContentStoreEntry *entry =
            (_AthenaLRUContentStoreEntry *) parcHashMap_Get(impl->tableByNameAndObjectHash, nameAndHashKey);
        parcObject_Release((PARCObject **) &nameAndHashKey);

        if (entry != NULL) {
            _athenaLRUContentStore_PurgeContentStoreEntry(store, entry);
            wasRemoved = true;
        }
    }

    if (!wasRemoved && keyIdRestriction != NULL) {
        PARCObject *nameAndKeyIdKey = _createHashableKey(name, keyIdRestriction, NULL);
        _AthenaLRUContentStoreEntry *entry = (_AthenaLRUContentStoreEntry *) parcHashMap_Get(impl->tableByNameAndKeyId,
                                                                                             nameAndKeyIdKey);
        parcObject_Release((PARCObject **) &nameAndKeyIdKey);

        if (entry != NULL) {
            _athenaLRUContentStore_PurgeContentStoreEntry(store, entry);
            wasRemoved = true;
        }
    }

    if (!wasRemoved) {
        PARCObject *nameKey = _createHashableKey(name, NULL, NULL);
        _AthenaLRUContentStoreEntry *entry = (_AthenaLRUContentStoreEntry *) parcHashMap_Get(impl->tableByName,
                                                                                             nameKey);
        parcObject_Release((PARCObject **) &nameKey);

        if (entry != NULL) {
            _athenaLRUContentStore_PurgeContentStoreEntry(store, entry);
            wasRemoved = true;
        }
    }

    return wasRemoved;
}

static size_t
_athenaLRUContentStore_GetCapacity(AthenaContentStoreImplementation *store)
{
    AthenaLRUContentStore *impl = (AthenaLRUContentStore *) store;
    return impl->maxSizeInBytes / (1024 * 1024);
}

static bool
_athenaLRUContentStore_SetCapacity(AthenaContentStoreImplementation *store, size_t maxSizeInMB)
{
    AthenaLRUContentStore *impl = (AthenaLRUContentStore *) store;
    impl->maxSizeInBytes = maxSizeInMB * (1024 * 1024);

    // TODO: Trim existing entries to fit into the new limit, if necessary

    _athenaLRUContentStore_ReleaseAllData(impl);

    //
    // NOTE: Calculating the number of buckets for the hashmaps is a temporary workaround for
    //       PARCHashMap not yet implementing internal resizing. See BugzId: 3950
    //
    _athenaLRUContentStore_initializeIndexes(impl, impl->maxSizeInBytes);

    return true;
}

static void
_getChunkNumberFromName(const CCNxName *name, uint64_t *chunkNum, bool *hasChunkNum)
{
    // XXX: This could be a utility in CCNxName.

    size_t numSegments = ccnxName_GetSegmentCount(name);
    CCNxNameSegment *lastSeg = ccnxName_GetSegment(name, numSegments - 1);

    if (ccnxNameSegment_GetType(lastSeg) == CCNxNameLabelType_CHUNK) {
        *hasChunkNum = true;
        *chunkNum = ccnxNameSegmentNumber_Value(lastSeg);
    } else {
        *hasChunkNum = false;
        *chunkNum = 0;
    }
}

/**
 * Create a PARCBuffer payload containing a JSON string with information about this ContentStore's
 * size.
 */
static PARCBuffer *
_createStatSizeResponsePayload(const AthenaLRUContentStore *impl, const CCNxName *name, uint64_t chunkNumber)
{
    PARCJSON *json = parcJSON_Create();

    parcJSON_AddString(json, "moduleName", AthenaContentStore_LRUImplementation.description);
    parcJSON_AddInteger(json, "time", parcClock_GetTime(impl->wallClock));
    parcJSON_AddInteger(json, "numEntries", impl->numEntries);
    parcJSON_AddInteger(json, "sizeInBytes", impl->currentSizeInBytes);

    char *jsonString = parcJSON_ToString(json);

    parcJSON_Release(&json);

    PARCBuffer *result = parcBuffer_CreateFromArray(jsonString, strlen(jsonString));

    parcMemory_Deallocate(&jsonString);

    return parcBuffer_Flip(result);
}

/**
 * Create a PARCBuffer payload containing a JSON string with information about this ContentStore's
 * cache hit rate.
 */
static PARCBuffer *
_createStatHitsResponsePayload(const AthenaLRUContentStore *impl, const CCNxName *name, uint64_t chunkNumber)
{
    PARCJSON *json = parcJSON_Create();

    parcJSON_AddString(json, "moduleName", AthenaContentStore_LRUImplementation.description);
    parcJSON_AddInteger(json, "time", parcClock_GetTime(impl->wallClock));
    parcJSON_AddInteger(json, "numAdds", impl->stats.numAdds);
    parcJSON_AddInteger(json, "numHits", impl->stats.numMatchHits);
    parcJSON_AddInteger(json, "numMisses", impl->stats.numMatchMisses);
    parcJSON_AddInteger(json, "numRemovedByExpiration", impl->stats.numRemovedByExpiration);

    char *jsonString = parcJSON_ToString(json);

    parcJSON_Release(&json);

    PARCBuffer *result = parcBuffer_CreateFromArray(jsonString, strlen(jsonString));

    parcMemory_Deallocate(&jsonString);

    return parcBuffer_Flip(result);
}

static PARCBuffer *
_processStatQuery(const AthenaLRUContentStore *impl, CCNxName *queryName, size_t argIndex, uint64_t chunkNumber)
{
    PARCBuffer *result = NULL;

    if (argIndex < ccnxName_GetSegmentCount(queryName)) {
        CCNxNameSegment *segment = ccnxName_GetSegment(queryName, argIndex);
        char *queryString = ccnxNameSegment_ToString(segment);

        char *sizeString = "size";
        char *hitsString = "hits";

        if (strncasecmp(queryString, sizeString, strlen(sizeString)) == 0) {
            result = _createStatSizeResponsePayload(impl, queryName, chunkNumber);
        } else if (strncasecmp(queryString, hitsString, strlen(hitsString)) == 0) {
            result = _createStatHitsResponsePayload(impl, queryName, chunkNumber);
        }

        parcMemory_Deallocate(&queryString);
    }
    return result;
}

static bool
_getSegmentIndexOfQueryArgs(CCNxName *name, char *nameString, size_t *segmentNumber)
{
    bool result = false;
    size_t numSegments = ccnxName_GetSegmentCount(name);
    size_t curSegment = 0;
    while (curSegment < numSegments) {
        CCNxNameSegment *segment = ccnxName_GetSegment(name, curSegment);
        if (ccnxNameSegment_GetType(segment) == CCNxNameLabelType_NAME) {
            char *segString = ccnxNameSegment_ToString(segment);
            if (strncasecmp(segString, nameString, strlen(nameString)) == 0) {
                parcMemory_Deallocate(&segString);
                *segmentNumber = curSegment + 1;
                result = true;
                break;
            }
            parcMemory_Deallocate(&segString);
            curSegment++;
        }
    }
    return result;
}


static CCNxMetaMessage *
_athenaLRUContentStore_ProcessMessage(AthenaContentStoreImplementation *store, const CCNxMetaMessage *message)
{
    CCNxMetaMessage *result = NULL;
    AthenaLRUContentStore *impl = (AthenaLRUContentStore *) store;

    if (ccnxMetaMessage_IsInterest(message)) {
        CCNxInterest *interest = ccnxMetaMessage_GetInterest(message);
        CCNxName *queryName = ccnxInterest_GetName(interest);

        uint64_t chunkNumber = 0;
        bool hasChunkNumber = false;
        _getChunkNumberFromName(queryName, &chunkNumber, &hasChunkNumber);
        assertFalse(hasChunkNumber, "LRUContentStore queries don't yet support more than 1 chunk.");

        PARCBuffer *responsePayload = NULL;

        // Find the arguments to our query.
        size_t argSegmentIndex = 0;
        if (_getSegmentIndexOfQueryArgs(queryName, AthenaModule_ContentStore, &argSegmentIndex)) {
            CCNxNameSegment *queryTypeSegment = ccnxName_GetSegment(queryName, argSegmentIndex);
            char *queryTypeString = ccnxNameSegment_ToString(queryTypeSegment);  // e.g. "stat"

            char *statString = "stat";
            if (strncasecmp(queryTypeString, statString, strlen(statString)) == 0) {
                responsePayload = _processStatQuery(impl, queryName, argSegmentIndex + 1, chunkNumber);
            }
            parcMemory_Deallocate(&queryTypeString);
        }

        if (responsePayload != NULL) {
            CCNxContentObject *contentObjectResponse = ccnxContentObject_CreateWithDataPayload(
                ccnxInterest_GetName(interest), responsePayload);

            result = ccnxMetaMessage_CreateFromContentObject(contentObjectResponse);
            ccnxContentObject_SetExpiryTime(contentObjectResponse,
                                            parcClock_GetTime(impl->wallClock) +
                                            100); // this response is good for 100 millis

            ccnxContentObject_Release(&contentObjectResponse);
            parcBuffer_Release(&responsePayload);
        }
    }

    return result;  // could be NULL
}

AthenaContentStoreInterface AthenaContentStore_LRUImplementation = {
    .description      = "AthenaContentStore_LRUImplementation 20150913",
    .create           = _athenaLRUContentStore_Create,
    .release          = _athenaLRUContentStore_Release,

    .putContentObject = _athenaLRUContentStore_PutContentObject,
    .getMatch         = _athenaLRUContentStore_GetMatch,
    .removeMatch      = _athenaLRUContentStore_RemoveMatch,

    .getCapacity      = _athenaLRUContentStore_GetCapacity,
    .setCapacity      = _athenaLRUContentStore_SetCapacity,

    .processMessage   = _athenaLRUContentStore_ProcessMessage
};

