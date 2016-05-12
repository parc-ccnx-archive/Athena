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
 * @author Alan Walendowski, Palo Alto Research Center (Xerox PARC)
 * @copyright (c) 2015, Xerox Corporation (Xerox) and Palo Alto Research Center, Inc (PARC).  All rights reserved.
 */
#include <stdio.h>

#include "../athena_LRUContentStore.c"

#include <LongBow/testing.h>

#include <parc/algol/parc_SafeMemory.h>


static AthenaLRUContentStore *
_createLRUContentStore()
{
    AthenaLRUContentStoreConfig config;
    config.capacityInMB = 1;

    return _athenaLRUContentStore_Create(&config);
}

LONGBOW_TEST_RUNNER(ccnx_LRUContentStore)
{
    parcMemory_SetInterface(&PARCSafeMemoryAsPARCMemory);

    LONGBOW_RUN_TEST_FIXTURE(CreateAcquireRelease);
    LONGBOW_RUN_TEST_FIXTURE(Object);
    LONGBOW_RUN_TEST_FIXTURE(Local);
}

// The Test Runner calls this function once before any Test Fixtures are run.
LONGBOW_TEST_RUNNER_SETUP(ccnx_LRUContentStore)
{
    return LONGBOW_STATUS_SUCCEEDED;
}

// The Test Runner calls this function once after all the Test Fixtures are run.
LONGBOW_TEST_RUNNER_TEARDOWN(ccnx_LRUContentStore)
{
    return LONGBOW_STATUS_SUCCEEDED;
}

LONGBOW_TEST_FIXTURE(CreateAcquireRelease)
{
    LONGBOW_RUN_TEST_CASE(CreateAcquireRelease, CreateRelease);
}

LONGBOW_TEST_FIXTURE_SETUP(CreateAcquireRelease)
{
    return LONGBOW_STATUS_SUCCEEDED;
}

LONGBOW_TEST_FIXTURE_TEARDOWN(CreateAcquireRelease)
{
    uint32_t outstandingAllocations = parcSafeMemory_ReportAllocation(STDERR_FILENO);
    if (outstandingAllocations != 0) {
        printf("%s leaks memory by %d allocations\n", longBowTestCase_GetName(testCase), outstandingAllocations);
        return LONGBOW_STATUS_MEMORYLEAK;
    }
    return LONGBOW_STATUS_SUCCEEDED;
}

LONGBOW_TEST_CASE(CreateAcquireRelease, CreateRelease)
{
    AthenaLRUContentStore *instance = _createLRUContentStore();

    assertNotNull(instance, "Expected non-null result from _athenaLRUContentStore_Create(NULL);");

    //parcObjectTesting_AssertAcquireReleaseContract(instance);

    _athenaLRUContentStore_Release((AthenaContentStoreImplementation *) &instance);
    assertNull(instance, "Expected null result from athenaLRUContentStore_Release();");
}

LONGBOW_TEST_FIXTURE(Object)
{
    LONGBOW_RUN_TEST_CASE(Object, athenaLRUContentStore_Compare);
    LONGBOW_RUN_TEST_CASE(Object, athenaLRUContentStore_Display);
    LONGBOW_RUN_TEST_CASE(Object, athenaLRUContentStore_IsValid);
    LONGBOW_RUN_TEST_CASE(Object, athenaLRUContentStore_ToString);
}

LONGBOW_TEST_FIXTURE_SETUP(Object)
{
    return LONGBOW_STATUS_SUCCEEDED;
}

LONGBOW_TEST_FIXTURE_TEARDOWN(Object)
{
    uint32_t outstandingAllocations = parcSafeMemory_ReportAllocation(STDERR_FILENO);
    if (outstandingAllocations != 0) {
        printf("%s leaks memory by %d allocations\n", longBowTestCase_GetName(testCase), outstandingAllocations);
        return LONGBOW_STATUS_MEMORYLEAK;
    }
    return LONGBOW_STATUS_SUCCEEDED;
}

LONGBOW_TEST_CASE(Object, athenaLRUContentStore_Compare)
{
    testUnimplemented("");
}

LONGBOW_TEST_CASE(Object, athenaLRUContentStore_Display)
{
    AthenaLRUContentStore *instance = _athenaLRUContentStore_Create(NULL);
    athenaLRUContentStore_Display(instance, 0);
    _athenaLRUContentStore_Release((AthenaContentStoreImplementation *) &instance);
}


LONGBOW_TEST_CASE(Object, athenaLRUContentStore_IsValid)
{
    AthenaLRUContentStore *instance = _athenaLRUContentStore_Create(NULL);
    assertTrue(athenaLRUContentStore_IsValid(instance), "Expected athenaLRUContentStore_Create to result in a valid instance.");

    _athenaLRUContentStore_Release((AthenaContentStoreImplementation *) &instance);
    assertFalse(athenaLRUContentStore_IsValid(instance), "Expected athenaLRUContentStore_Release to result in an invalid instance.");
}

LONGBOW_TEST_CASE(Object, athenaLRUContentStore_ToString)
{
    AthenaLRUContentStore *instance = _athenaLRUContentStore_Create(NULL);

    char *string = athenaLRUContentStore_ToString(instance);

    assertNotNull(string, "Expected non-NULL result from athenaLRUContentStore_ToString");

    parcMemory_Deallocate((void **) &string);
    _athenaLRUContentStore_Release((AthenaContentStoreImplementation *) &instance);
}

/***************************************************************
***** Local Tests
***************************************************************/

/**
 * Create a CCNxContentObject with the given LCI name, chunknumber, and payload.
 */
static CCNxContentObject *
_createContentObject(char *lci, uint64_t chunkNum, PARCBuffer *payload)
{
    CCNxName *name = ccnxName_CreateFromCString(lci);
    CCNxNameSegment *chunkSegment = ccnxNameSegmentNumber_Create(CCNxNameLabelType_CHUNK, chunkNum);
    ccnxName_Append(name, chunkSegment);

    CCNxContentObject *result = ccnxContentObject_CreateWithNameAndPayload(name, payload);

    ccnxName_Release(&name);
    ccnxNameSegment_Release(&chunkSegment);

    return result;
}


LONGBOW_TEST_CASE(Local, _athenaLRUContentStoreEntry_CreateRelease)
{
    CCNxContentObject *contentObject = _createContentObject("lci:/boose/roo/pie", 0, NULL);
    _AthenaLRUContentStoreEntry *entry = _athenaLRUContentStoreEntry_Create(contentObject);

    _athenaLRUContentStoreEntry_Release(&entry);

    ccnxContentObject_Release(&contentObject);
}

LONGBOW_TEST_CASE(Local, _athenaLRUContentStore_PutLRUContentStoreEntry)
{
    AthenaLRUContentStore *impl = _createLRUContentStore();

    PARCBuffer *payload = parcBuffer_Allocate(1200);

    CCNxContentObject *contentObject = _createContentObject("lci:/boose/roo/pie", 10, NULL);

    parcBuffer_Release(&payload);
    _AthenaLRUContentStoreEntry *entry = _athenaLRUContentStoreEntry_Create(contentObject);
    ccnxContentObject_Release(&contentObject);

    entry->expiryTime = 10000;
    entry->contentObjectHash = parcBuffer_WrapCString("object hash buffer");
    entry->keyId = parcBuffer_WrapCString("key id buffer");
    entry->hasContentObjectHash = true;
    entry->hasKeyId = true;

    bool status = _athenaLRUContentStore_PutLRUContentStoreEntry(impl, entry);

    assertTrue(status, "Expected to put content into the store");

    assertTrue(status, "Expected to put content into the store a second time");
    assertTrue(impl->numEntries == 1, "Expected 1 entry in the store");
    assertTrue(impl->stats.numAdds == 1, "Expected stats to show 1 adds");

    _athenaLRUContentStore_PurgeContentStoreEntry(impl, entry);

    assertTrue(impl->numEntries == 0, "Expected 0 entries in the store");

    parcBuffer_Release(&entry->keyId);
    parcBuffer_Release(&entry->contentObjectHash);
    _athenaLRUContentStoreEntry_Release(&entry);

    _athenaLRUContentStore_Release((AthenaContentStoreImplementation *) &impl);
}


LONGBOW_TEST_CASE(Local, _athenaLRUContentStore_PutContentObject)
{
    AthenaLRUContentStore *impl = _createLRUContentStore();

    PARCBuffer *payload = parcBuffer_Allocate(1200);

    CCNxContentObject *contentObject = _createContentObject("lci:/boose/roo/pie", 10, NULL);

    parcBuffer_Release(&payload);

    bool status = _athenaLRUContentStore_PutContentObject(impl, contentObject);

    assertTrue(status, "Expected to put content into the store");

    // This should replace the existing entry.
    status = _athenaLRUContentStore_PutContentObject(impl, contentObject);

    assertTrue(status, "Expected to put content into the store a second time");
    assertTrue(impl->numEntries == 1, "Expected 1 entry in the store (after implicit removal of original entry");
    assertTrue(impl->stats.numAdds == 2, "Expected stats to show 2 adds");

    ccnxContentObject_Release(&contentObject);
    _athenaLRUContentStore_Release((AthenaContentStoreImplementation *) &impl);
}

LONGBOW_TEST_CASE(Local, _athenaLRUContentStore_PutManyContentObjects)
{
    AthenaLRUContentStore *impl = _createLRUContentStore();

    PARCBuffer *payload = parcBuffer_Allocate(1200);

    int numEntriesToPut = 100;
    for (int i = 0; i < numEntriesToPut; i++) {
        // Re-use the payload per contentObject, though the store will think each is seperate (for size calculations)
        CCNxContentObject *contentObject = _createContentObject("lci:/boose/roo/pie", i, payload);
        bool status = _athenaLRUContentStore_PutContentObject(impl, contentObject);
        assertTrue(status, "Expected CO %d to be put in to the store", i);
        ccnxContentObject_Release(&contentObject);
    }

    assertTrue(impl->numEntries == numEntriesToPut, "Expected the numbe of entries put in the store to match");

    // Now put the same ones (by name) again. These should kick out the old ones.

    for (int i = 0; i < numEntriesToPut; i++) {
        // Re-use the payload per contentObject, though the store will think each is seperate (for size calculations)
        CCNxContentObject *contentObject = _createContentObject("lci:/boose/roo/pie", i, payload);
        bool status = _athenaLRUContentStore_PutContentObject(impl, contentObject);
        assertTrue(status, "Expected CO %d to be put in to the store", i);
        ccnxContentObject_Release(&contentObject);
    }

    assertTrue(impl->numEntries == numEntriesToPut, "Expected the numbe of entries put in the store to match");

    parcBuffer_Release(&payload);

    athenaLRUContentStore_Display(impl, 0);

    _athenaLRUContentStore_Release((AthenaContentStoreImplementation *) &impl);
}

LONGBOW_TEST_CASE(Local, _athenaLRUContentStore_GetMatchByName)
{
    AthenaLRUContentStore *impl = _createLRUContentStore();

    PARCBuffer *payload = parcBuffer_Allocate(500);

    CCNxName *name = ccnxName_CreateFromCString("lci:/boose/roo/pie");
    CCNxContentObject *contentObject = ccnxContentObject_CreateWithNameAndPayload(name, payload);
    parcBuffer_Release(&payload);

    bool status = _athenaLRUContentStore_PutContentObject(impl, contentObject);
    assertTrue(status, "Expected to put content into the store");

    PARCBuffer *payload2 = parcBuffer_Allocate(500);
    CCNxName *name2 = ccnxName_CreateFromCString("lci:/roo/pie/boose");
    CCNxContentObject *contentObject2 = ccnxContentObject_CreateWithNameAndPayload(name2, payload2);
    parcBuffer_Release(&payload2);

    bool status2 = _athenaLRUContentStore_PutContentObject(impl, contentObject2);
    assertTrue(status2, "Expected to put content into the store");

    // At this point, both objects should be in the store.
    athenaLRUContentStore_Display(impl, 2);

    assertTrue(impl->stats.numAdds == 2, "Expected 2 store adds");

    // Now try to fetch each of them.

    CCNxInterest *interest1 = ccnxInterest_CreateSimple(name);
    CCNxContentObject *match = _athenaLRUContentStore_GetMatch(impl, interest1);
    assertTrue(match == contentObject, "Expected to match the first content object");

    CCNxInterest *interest2 = ccnxInterest_CreateSimple(name2);
    CCNxContentObject *match2 = _athenaLRUContentStore_GetMatch(impl, interest2);
    assertTrue(match2 == contentObject2, "Expected to match the second content object");

    // Now try to match a non-existent name.
    CCNxName *nameNoMatch = ccnxName_CreateFromCString("lci:/pie/roo/boose/this/should/not/match");
    CCNxInterest *interest3 = ccnxInterest_CreateSimple(nameNoMatch);
    CCNxContentObject *noMatch = _athenaLRUContentStore_GetMatch(impl, interest3);
    assertNull(noMatch, "Expected a NULL response from an unmatchable name");

    ccnxInterest_Release(&interest1);
    ccnxInterest_Release(&interest2);
    ccnxInterest_Release(&interest3);
    ccnxName_Release(&nameNoMatch);
    ccnxName_Release(&name);
    ccnxName_Release(&name2);
    ccnxContentObject_Release(&contentObject);
    ccnxContentObject_Release(&contentObject2);

    _athenaLRUContentStore_Release((AthenaContentStoreImplementation *) &impl);
}

LONGBOW_TEST_CASE(Local, _athenaLRUContentStore_GetMatchByNameAndKeyId)
{
    AthenaLRUContentStore *impl = _createLRUContentStore();

    PARCBuffer *payload = parcBuffer_Allocate(1200);

    CCNxName *name = ccnxName_CreateFromCString("lci:/boose/roo/pie");
    CCNxContentObject *contentObject = ccnxContentObject_CreateWithNameAndPayload(name, NULL);

    parcBuffer_Release(&payload);
    _AthenaLRUContentStoreEntry *entry1 = _athenaLRUContentStoreEntry_Create(contentObject);

    entry1->hasKeyId = false;
    entry1->hasContentObjectHash = false;

    bool status = _athenaLRUContentStore_PutLRUContentStoreEntry(impl, entry1);
    assertTrue(status, "Expected to add the entry");

    // Now add another content object with the same name, but a KeyId too.

    CCNxContentObject *contentObject2 = ccnxContentObject_CreateWithNameAndPayload(name, NULL);

    _AthenaLRUContentStoreEntry *entry2 = _athenaLRUContentStoreEntry_Create(contentObject2);

    entry2->keyId = parcBuffer_WrapCString("key id buffer");
    entry2->hasKeyId = true;

    status = _athenaLRUContentStore_PutLRUContentStoreEntry(impl, entry2);
    assertTrue(status, "Expected to add the entry");
    assertTrue(impl->numEntries == 2, "Expected 2 store items");

    // Now match on Name + KeyIdRestriction.

    CCNxInterest *interest = ccnxInterest_CreateSimple(name);
    PARCBuffer *keyIdRestriction = parcBuffer_Copy(entry2->keyId);
    ccnxInterest_SetKeyIdRestriction(interest, keyIdRestriction);
    parcBuffer_Release(&keyIdRestriction);

    CCNxContentObject *match = _athenaLRUContentStore_GetMatch(impl, interest);
    assertNotNull(match, "Expected to match something");
    assertTrue(match == contentObject2, "Expected the content object with the keyId");

    _athenaLRUContentStoreEntry_Release(&entry1);
    _athenaLRUContentStoreEntry_Release(&entry2);
    ccnxContentObject_Release(&contentObject2);
    ccnxContentObject_Release(&contentObject);
    ccnxName_Release(&name);
    ccnxInterest_Release(&interest);

    _athenaLRUContentStore_Release((AthenaContentStoreImplementation *) &impl);
}

LONGBOW_TEST_CASE(Local, _athenaLRUContentStore_GetMatchByNameAndObjectHash)
{
    AthenaLRUContentStore *impl = _createLRUContentStore();

    PARCBuffer *payload = parcBuffer_Allocate(1200);

    CCNxName *name = ccnxName_CreateFromCString("lci:/boose/roo/pie");
    CCNxContentObject *contentObject = ccnxContentObject_CreateWithNameAndPayload(name, NULL);

    parcBuffer_Release(&payload);
    _AthenaLRUContentStoreEntry *entry1 = _athenaLRUContentStoreEntry_Create(contentObject);

    entry1->hasKeyId = false;
    entry1->hasContentObjectHash = false;

    bool status = _athenaLRUContentStore_PutLRUContentStoreEntry(impl, entry1);
    assertTrue(status, "Expected to add the entry");

    // Now add another content object with the same name, but an object hash too.

    CCNxContentObject *contentObject2 = ccnxContentObject_CreateWithNameAndPayload(name, NULL);

    _AthenaLRUContentStoreEntry *entry2 = _athenaLRUContentStoreEntry_Create(contentObject2);

    entry2->contentObjectHash = parcBuffer_WrapCString("corned beef");
    entry2->hasContentObjectHash = true;

    status = _athenaLRUContentStore_PutLRUContentStoreEntry(impl, entry2);
    assertTrue(status, "Expected to add the entry");
    assertTrue(impl->numEntries == 2, "Expected 2 store items");

    // Now match on Name + ObjectHash.

    CCNxInterest *interest = ccnxInterest_CreateSimple(name);
    PARCBuffer *hashRestriction = parcBuffer_Copy(entry2->contentObjectHash);
    ccnxInterest_SetContentObjectHashRestriction(interest, hashRestriction);
    parcBuffer_Release(&hashRestriction);

    CCNxContentObject *match = _athenaLRUContentStore_GetMatch(impl, interest);
    assertNotNull(match, "Expected to match something");
    assertTrue(match == contentObject2, "Expected the content object with the keyId");

    _athenaLRUContentStoreEntry_Release(&entry1);
    _athenaLRUContentStoreEntry_Release(&entry2);
    ccnxContentObject_Release(&contentObject2);
    ccnxContentObject_Release(&contentObject);
    ccnxName_Release(&name);
    ccnxInterest_Release(&interest);

    _athenaLRUContentStore_Release((AthenaContentStoreImplementation *) &impl);
}

LONGBOW_TEST_CASE(Local, _moveContentStoreEntryToLRUHead)
{
    AthenaLRUContentStore *impl = _createLRUContentStore();

    CCNxName *name = ccnxName_CreateFromCString("lci:/first/entry");
    CCNxContentObject *contentObject1 = ccnxContentObject_CreateWithNameAndPayload(name, NULL);
    ccnxName_Release(&name);

    name = ccnxName_CreateFromCString("lci:/second/entry");
    CCNxContentObject *contentObject2 = ccnxContentObject_CreateWithNameAndPayload(name, NULL);
    ccnxName_Release(&name);

    name = ccnxName_CreateFromCString("lci:/third/entry");
    CCNxContentObject *contentObject3 = ccnxContentObject_CreateWithNameAndPayload(name, NULL);
    ccnxName_Release(&name);

    name = ccnxName_CreateFromCString("lci:/fourth/entry");
    CCNxContentObject *contentObject4 = ccnxContentObject_CreateWithNameAndPayload(name, NULL);
    ccnxName_Release(&name);

    bool status = _athenaLRUContentStore_PutContentObject(impl, contentObject1);
    assertTrue(status, "Expected to insert content");

    status = _athenaLRUContentStore_PutContentObject(impl, contentObject2);
    assertTrue(status, "Expected to insert content");
    assertTrue(impl->lruHead->contentObject == contentObject2, "Expected 2 at lruHead");

    status = _athenaLRUContentStore_PutContentObject(impl, contentObject3);
    assertTrue(status, "Expected to insert content");

    assertTrue(impl->lruHead->contentObject == contentObject3, "Expected 3 at lruHead");
    assertTrue(impl->lruTail->contentObject == contentObject1, "Expected 1 at lruTail");

    athenaLRUContentStore_Display(impl, 2);

    _moveContentStoreEntryToLRUHead(impl, impl->lruTail);
    athenaLRUContentStore_Display(impl, 2);
    assertTrue(impl->lruHead->contentObject == contentObject1, "Expected 1 at lruHead");
    assertTrue(impl->lruTail->contentObject == contentObject2, "Expected 2 at lruTail");

    _moveContentStoreEntryToLRUHead(impl, impl->lruTail);
    athenaLRUContentStore_Display(impl, 2);
    assertTrue(impl->lruHead->contentObject == contentObject2, "Expected 2 at lruHead");
    assertTrue(impl->lruTail->contentObject == contentObject3, "Expected 3 at lruTail");

    status = _athenaLRUContentStore_PutContentObject(impl, contentObject4);
    assertTrue(status, "Expected to insert content");
    athenaLRUContentStore_Display(impl, 2);
    assertTrue(impl->lruHead->contentObject == contentObject4, "Expected 4 at lruHead");
    assertTrue(impl->lruTail->contentObject == contentObject3, "Expected 3 at lruTail");

    _moveContentStoreEntryToLRUHead(impl, impl->lruTail);
    athenaLRUContentStore_Display(impl, 2);
    assertTrue(impl->lruHead->contentObject == contentObject3, "Expected 3 at lruHead");
    assertTrue(impl->lruTail->contentObject == contentObject1, "Expected 1 at lruTail");

    ccnxContentObject_Release(&contentObject1);
    ccnxContentObject_Release(&contentObject2);
    ccnxContentObject_Release(&contentObject3);
    ccnxContentObject_Release(&contentObject4);

    _athenaLRUContentStore_Release((AthenaContentStoreImplementation *) &impl);
}

LONGBOW_TEST_CASE(Local, _getLeastUsedFromLRU)
{
    AthenaLRUContentStore *impl = _createLRUContentStore();

    CCNxName *name1 = ccnxName_CreateFromCString("lci:/first/entry");
    CCNxContentObject *contentObject1 = ccnxContentObject_CreateWithNameAndPayload(name1, NULL);

    CCNxName *name2 = ccnxName_CreateFromCString("lci:/second/entry");
    CCNxContentObject *contentObject2 = ccnxContentObject_CreateWithNameAndPayload(name2, NULL);

    CCNxName *name3 = ccnxName_CreateFromCString("lci:/third/entry");
    CCNxContentObject *contentObject3 = ccnxContentObject_CreateWithNameAndPayload(name3, NULL);

    bool status = _athenaLRUContentStore_PutContentObject(impl, contentObject1);
    assertTrue(status, "Exepected to insert content");

    status = _athenaLRUContentStore_PutContentObject(impl, contentObject2);
    assertTrue(status, "Exepected to insert content");

    status = _athenaLRUContentStore_PutContentObject(impl, contentObject3);
    assertTrue(status, "Exepected to insert content");

    athenaLRUContentStore_Display(impl, 2);

    _AthenaLRUContentStoreEntry *entry = _getLeastUsedFromLRU(impl);
    assertTrue(ccnxContentObject_Equals(entry->contentObject, contentObject1), "Expected to retrieve contentObject1");
    _athenaLRUContentStore_PurgeContentStoreEntry(impl, entry);

    entry = _getLeastUsedFromLRU(impl);
    assertTrue(ccnxContentObject_Equals(entry->contentObject, contentObject2), "Expected to retrieve contentObject2");
    _athenaLRUContentStore_PurgeContentStoreEntry(impl, entry);

    entry = _getLeastUsedFromLRU(impl);
    assertTrue(ccnxContentObject_Equals(entry->contentObject, contentObject3), "Expected to retrieve contentObject3");
    _athenaLRUContentStore_PurgeContentStoreEntry(impl, entry);

    ccnxContentObject_Release(&contentObject1);
    ccnxContentObject_Release(&contentObject2);
    ccnxContentObject_Release(&contentObject3);
    ccnxName_Release(&name1);
    ccnxName_Release(&name2);
    ccnxName_Release(&name3);

    _athenaLRUContentStore_Release((AthenaContentStoreImplementation *) &impl);
}

LONGBOW_TEST_CASE(Local, _compareByExpiryTime)
{
    CCNxName *name1 = ccnxName_CreateFromCString("lci:/first/entry");
    CCNxContentObject *contentObject1 = ccnxContentObject_CreateWithNameAndPayload(name1, NULL);

    CCNxName *name2 = ccnxName_CreateFromCString("lci:/second/entry");
    CCNxContentObject *contentObject2 = ccnxContentObject_CreateWithNameAndPayload(name2, NULL);

    CCNxName *name3 = ccnxName_CreateFromCString("lci:/third/entry");
    CCNxContentObject *contentObject3 = ccnxContentObject_CreateWithNameAndPayload(name3, NULL);

    ccnxContentObject_SetExpiryTime(contentObject1, 100);
    ccnxContentObject_SetExpiryTime(contentObject2, 200);
    // contentObject3 has no expiry time.

    _AthenaLRUContentStoreEntry *entry1 = _athenaLRUContentStoreEntry_Create(contentObject1);
    _AthenaLRUContentStoreEntry *entry2 = _athenaLRUContentStoreEntry_Create(contentObject2);
    _AthenaLRUContentStoreEntry *entry3 = _athenaLRUContentStoreEntry_Create(contentObject3);

    assertTrue(_compareByExpiryTime(entry1, entry2) == -1, "Expected result -1");
    assertTrue(_compareByExpiryTime(entry2, entry1) == 1, "Expected result 1");
    assertTrue(_compareByExpiryTime(entry2, entry2) == 0, "Expected result 0");

    assertTrue(_compareByExpiryTime(entry1, entry3) == -1, "Expected result -1");
    assertTrue(_compareByExpiryTime(entry3, entry2) == 1, "Expected result 1");
    assertTrue(_compareByExpiryTime(entry3, entry3) == 0, "Expected result 0");

    _athenaLRUContentStoreEntry_Release(&entry1);
    _athenaLRUContentStoreEntry_Release(&entry2);
    _athenaLRUContentStoreEntry_Release(&entry3);

    ccnxContentObject_Release(&contentObject1);
    ccnxContentObject_Release(&contentObject2);
    ccnxContentObject_Release(&contentObject3);
    ccnxName_Release(&name1);
    ccnxName_Release(&name2);
    ccnxName_Release(&name3);
}

LONGBOW_TEST_CASE(Local, _compareByRecommendedCacheTime)
{
    CCNxName *name1 = ccnxName_CreateFromCString("lci:/first/entry");
    CCNxContentObject *contentObject1 = ccnxContentObject_CreateWithNameAndPayload(name1, NULL);

    CCNxName *name2 = ccnxName_CreateFromCString("lci:/second/entry");
    CCNxContentObject *contentObject2 = ccnxContentObject_CreateWithNameAndPayload(name2, NULL);

    CCNxName *name3 = ccnxName_CreateFromCString("lci:/third/entry");
    CCNxContentObject *contentObject3 = ccnxContentObject_CreateWithNameAndPayload(name3, NULL);


    _AthenaLRUContentStoreEntry *entry1 = _athenaLRUContentStoreEntry_Create(contentObject1);
    _AthenaLRUContentStoreEntry *entry2 = _athenaLRUContentStoreEntry_Create(contentObject2);
    _AthenaLRUContentStoreEntry *entry3 = _athenaLRUContentStoreEntry_Create(contentObject3);

    // There is no interface (yet) for assigning the recommended cache time. So update the store entries directly.

    entry1->hasRecommendedCacheTime = true;
    entry1->recommendedCacheTime = 1000;

    entry2->hasRecommendedCacheTime = true;
    entry2->recommendedCacheTime = 5000;

    entry3->hasRecommendedCacheTime = false;   // treat contentObject3 as if it has no RCT
    entry3->recommendedCacheTime = 10;

    assertTrue(_compareByRecommendedCacheTime(entry1, entry2) == -1, "Expected result -1");
    assertTrue(_compareByRecommendedCacheTime(entry2, entry1) == 1, "Expected result 1");
    assertTrue(_compareByRecommendedCacheTime(entry2, entry2) == 0, "Expected result 0");

    assertTrue(_compareByRecommendedCacheTime(entry1, entry3) == -1, "Expected result -1");
    assertTrue(_compareByRecommendedCacheTime(entry3, entry2) == 1, "Expected result 1");
    assertTrue(_compareByRecommendedCacheTime(entry3, entry3) == 0, "Expected result 0");

    _athenaLRUContentStoreEntry_Release(&entry1);
    _athenaLRUContentStoreEntry_Release(&entry2);
    _athenaLRUContentStoreEntry_Release(&entry3);

    ccnxContentObject_Release(&contentObject1);
    ccnxContentObject_Release(&contentObject2);
    ccnxContentObject_Release(&contentObject3);
    ccnxName_Release(&name1);
    ccnxName_Release(&name2);
    ccnxName_Release(&name3);
}

LONGBOW_TEST_CASE(Local, putWithExpiryTime)
{
    AthenaLRUContentStore *impl = _createLRUContentStore();

    CCNxName *name1 = ccnxName_CreateFromCString("lci:/first/entry");
    CCNxContentObject *contentObject1 = ccnxContentObject_CreateWithNameAndPayload(name1, NULL);

    CCNxName *name2 = ccnxName_CreateFromCString("lci:/second/entry");
    CCNxContentObject *contentObject2 = ccnxContentObject_CreateWithNameAndPayload(name2, NULL);

    CCNxName *name3 = ccnxName_CreateFromCString("lci:/third/entry");
    CCNxContentObject *contentObject3 = ccnxContentObject_CreateWithNameAndPayload(name3, NULL);

    uint64_t now = parcClock_GetTime(impl->wallClock);

    ccnxContentObject_SetExpiryTime(contentObject1, now + 200);  // Expires AFTER object 2
    ccnxContentObject_SetExpiryTime(contentObject2, now + 100);
    // contentObject3 has no expiry time, so it expires last.

    _AthenaLRUContentStoreEntry *entry1 = _athenaLRUContentStoreEntry_Create(contentObject1);
    _AthenaLRUContentStoreEntry *entry2 = _athenaLRUContentStoreEntry_Create(contentObject2);
    _AthenaLRUContentStoreEntry *entry3 = _athenaLRUContentStoreEntry_Create(contentObject3);

    bool status = _athenaLRUContentStore_PutContentObject(impl, contentObject1);
    assertTrue(status, "Exepected to insert content");

    status = _athenaLRUContentStore_PutContentObject(impl, contentObject2);
    assertTrue(status, "Exepected to insert content");

    _AthenaLRUContentStoreEntry *oldestEntry = _getEarliestExpiryTime(impl);

    assertTrue(oldestEntry->contentObject == entry2->contentObject, "Expected entry 2 to be the earliest expiring entry");

    status = _athenaLRUContentStore_PutContentObject(impl, contentObject3);
    assertTrue(status, "Exepected to insert content");

    // The entry with no expiration time should not affect list ordering.
    oldestEntry = _getEarliestExpiryTime(impl);
    assertTrue(oldestEntry->contentObject == entry2->contentObject, "Expected entry 2 to be the earliest expiring entry");

    // Now remove the oldest one we added. The next oldest one should be contentObject1
    _athenaLRUContentStore_RemoveMatch(impl, name2, NULL, NULL);

    // The entry with no expiration time should not affect list ordering.
    oldestEntry = _getEarliestExpiryTime(impl);
    assertTrue(oldestEntry->contentObject == entry1->contentObject, "Expected entry 1 to be the earliest expiring entry");

    _athenaLRUContentStoreEntry_Release(&entry1);
    _athenaLRUContentStoreEntry_Release(&entry2);
    _athenaLRUContentStoreEntry_Release(&entry3);

    ccnxContentObject_Release(&contentObject1);
    ccnxContentObject_Release(&contentObject2);
    ccnxContentObject_Release(&contentObject3);
    ccnxName_Release(&name1);
    ccnxName_Release(&name2);
    ccnxName_Release(&name3);

    _athenaLRUContentStore_Release((AthenaContentStoreImplementation *) &impl);
}

LONGBOW_TEST_CASE(Local, putWithExpiryTime_Expired)
{
    AthenaLRUContentStore *impl = _createLRUContentStore();

    CCNxName *name1 = ccnxName_CreateFromCString("lci:/first/entry");
    CCNxContentObject *contentObject1 = ccnxContentObject_CreateWithNameAndPayload(name1, NULL);

    CCNxName *name2 = ccnxName_CreateFromCString("lci:/second/entry");
    CCNxContentObject *contentObject2 = ccnxContentObject_CreateWithNameAndPayload(name2, NULL);

    CCNxName *name3 = ccnxName_CreateFromCString("lci:/third/entry");
    CCNxContentObject *contentObject3 = ccnxContentObject_CreateWithNameAndPayload(name3, NULL);

    uint64_t now = parcClock_GetTime(impl->wallClock);

    // NOTE: These two are considered expired and should NOT be added to the store.
    ccnxContentObject_SetExpiryTime(contentObject1, now);
    _AthenaLRUContentStoreEntry *entry1 = _athenaLRUContentStoreEntry_Create(contentObject1);

    ccnxContentObject_SetExpiryTime(contentObject2, now - 100);
    _AthenaLRUContentStoreEntry *entry2 = _athenaLRUContentStoreEntry_Create(contentObject2);

    // NOTE: This one does not have an expiry time, so should be added.
    _AthenaLRUContentStoreEntry *entry3 = _athenaLRUContentStoreEntry_Create(contentObject3);

    bool status = _athenaLRUContentStore_PutContentObject(impl, contentObject1);
    assertFalse(status, "Exepected to fail on inserting expired content");

    status = _athenaLRUContentStore_PutContentObject(impl, contentObject2);
    assertFalse(status, "Exepected to fail on inserting expired content");

    status = _athenaLRUContentStore_PutContentObject(impl, contentObject3);
    assertTrue(status, "Exepected to insert a ContentObject with no expiry time.");

    _athenaLRUContentStoreEntry_Release(&entry1);
    _athenaLRUContentStoreEntry_Release(&entry2);
    _athenaLRUContentStoreEntry_Release(&entry3);

    ccnxContentObject_Release(&contentObject1);
    ccnxContentObject_Release(&contentObject2);
    ccnxContentObject_Release(&contentObject3);
    ccnxName_Release(&name1);
    ccnxName_Release(&name2);
    ccnxName_Release(&name3);

    _athenaLRUContentStore_Release((AthenaContentStoreImplementation *) &impl);
}

LONGBOW_TEST_CASE(Local, capacitySetGet)
{
    AthenaLRUContentStore *impl = _createLRUContentStore();
    size_t truth = 100;
    _athenaLRUContentStore_SetCapacity(impl, truth);
    size_t test = _athenaLRUContentStore_GetCapacity(impl);

    assertTrue(test == truth, "expected the same size capacity as was set");

    _athenaLRUContentStore_Release((AthenaContentStoreImplementation *) &impl);
}

LONGBOW_TEST_CASE(Local, putContentAndEnforceCapacity)
{
    AthenaLRUContentStore *impl = _createLRUContentStore();

    size_t lastSizeOfStore = 0;

    size_t payloadSize = 100 * 1024;
    _athenaLRUContentStore_SetCapacity(impl, 1); // set to 1 MB, or ~10 of our payloads

    PARCBuffer *payload = parcBuffer_Allocate(payloadSize); // 100K buffer
    int i;

    for (i = 0; i < 20; i++) {  // Add more than 10 items.
        CCNxContentObject *content = _createContentObject("lci:/this/is/content", i, payload);
        assertNotNull(content, "Expected to allocated a content object");

        bool status = _athenaLRUContentStore_PutContentObject(impl, content);
        assertTrue(status, "Expected to be able to insert content");
        assertTrue(impl->currentSizeInBytes > lastSizeOfStore, "expected store size in bytes to grow");

        ccnxContentObject_Release(&content);
    }

    // Make sure that the contentobjects were added, but that the size didn't grow past the capacity
    assertTrue(impl->currentSizeInBytes < (11 * payloadSize), "expected the current store size to be less than 11 x payload size");
    assertTrue(impl->currentSizeInBytes >= (10 * payloadSize), "expected the current store size to be roughly 10 x payload size");

    parcBuffer_Release(&payload);

    _athenaLRUContentStore_Release((AthenaContentStoreImplementation *) &impl);
}

LONGBOW_TEST_CASE(Local, putContentAndExpireByExpiryTime)
{
    AthenaLRUContentStoreConfig config;
    config.capacityInMB = 1; // 2M

    AthenaLRUContentStore *impl = _athenaLRUContentStore_Create(&config);

    uint64_t now = parcClock_GetTime(impl->wallClock);

    PARCBuffer *payload = parcBuffer_Allocate(300 * 1000); // 300K payload. Should fit 3 into the store.

    CCNxName *name1 = ccnxName_CreateFromCString("lci:/object/1");
    CCNxName *name2 = ccnxName_CreateFromCString("lci:/object/2");
    CCNxName *name3 = ccnxName_CreateFromCString("lci:/object/3");
    CCNxName *name4 = ccnxName_CreateFromCString("lci:/object/4");

    CCNxContentObject *contentObject1 = ccnxContentObject_CreateWithNameAndPayload(name1, payload);
    _AthenaLRUContentStoreEntry *entry = _athenaLRUContentStoreEntry_Create(contentObject1);
    entry->hasExpiryTime = true;
    entry->expiryTime = now + 2000000;
    bool status = _athenaLRUContentStore_PutLRUContentStoreEntry(impl, entry);
    _athenaLRUContentStoreEntry_Release(&entry);
    assertTrue(status, "Expected to put the content in the store");

    CCNxContentObject *contentObject2 = ccnxContentObject_CreateWithNameAndPayload(name2, payload);
    entry = _athenaLRUContentStoreEntry_Create(contentObject2);
    entry->expiryTime = now - 10000; // This one expires first. (it's already expired)
    entry->hasExpiryTime = true;
    status = _athenaLRUContentStore_PutLRUContentStoreEntry(impl, entry);
    _athenaLRUContentStoreEntry_Release(&entry);
    assertTrue(status, "Expected to put the content in the store");

    CCNxContentObject *contentObject3 = ccnxContentObject_CreateWithNameAndPayload(name3, payload);
    entry = _athenaLRUContentStoreEntry_Create(contentObject3);
    entry->expiryTime = now + 3000000;
    entry->hasExpiryTime = true;
    status = _athenaLRUContentStore_PutLRUContentStoreEntry(impl, entry);
    _athenaLRUContentStoreEntry_Release(&entry);
    assertTrue(status, "Expected to put the content in the store");

    // At this point, there are three items in the store. Try to put in a 4th, which should force the one
    // with the earliest expiration time to be expired.

    CCNxContentObject *contentObject4 = ccnxContentObject_CreateWithNameAndPayload(name4, payload);
    entry = _athenaLRUContentStoreEntry_Create(contentObject4);
    entry->expiryTime = now + 3000000;
    entry->hasExpiryTime = true;
    status = _athenaLRUContentStore_PutLRUContentStoreEntry(impl, entry);
    _athenaLRUContentStoreEntry_Release(&entry);
    assertTrue(status, "Expected to put the content in the store");

    assertTrue(impl->currentSizeInBytes < impl->maxSizeInBytes, "Expected the current store size to be less than the capacity");

    athenaLRUContentStore_Display(impl, 0);

    // Now check that the oldest was removed.
    CCNxInterest *interest = ccnxInterest_CreateSimple(name2);
    CCNxContentObject *match = _athenaLRUContentStore_GetMatch(impl, interest);
    assertNull(match, "Expected the content for name2 to have been removed from the store");
    ccnxInterest_Release(&interest);

    // Now check that the others still exist.
    interest = ccnxInterest_CreateSimple(name1);
    match = _athenaLRUContentStore_GetMatch(impl, interest);
    assertNotNull(match, "Expected the content for name1 to be in the store");
    ccnxInterest_Release(&interest);

    interest = ccnxInterest_CreateSimple(name3);
    match = _athenaLRUContentStore_GetMatch(impl, interest);
    assertNotNull(match, "Expected the content for name3 to be in the store");
    ccnxInterest_Release(&interest);

    interest = ccnxInterest_CreateSimple(name4);
    match = _athenaLRUContentStore_GetMatch(impl, interest);
    assertNotNull(match, "Expected the content for name4 to be in the store");
    ccnxInterest_Release(&interest);

    ccnxContentObject_Release(&contentObject1);
    ccnxContentObject_Release(&contentObject2);
    ccnxContentObject_Release(&contentObject3);
    ccnxContentObject_Release(&contentObject4);
    ccnxName_Release(&name1);
    ccnxName_Release(&name2);
    ccnxName_Release(&name3);
    ccnxName_Release(&name4);
    parcBuffer_Release(&payload);

    _athenaLRUContentStore_Release((AthenaContentStoreImplementation *) &impl);
}

LONGBOW_TEST_CASE(Local, putTooBig)
{
    AthenaLRUContentStoreConfig config;
    config.capacityInMB = 1;

    AthenaLRUContentStore *impl = _athenaLRUContentStore_Create(&config);

    size_t payloadSize = 2 * 1024 * 1024; // 2M

    PARCBuffer *payload = parcBuffer_Allocate(payloadSize);

    CCNxContentObject *content = _createContentObject("lci:/this/is/content", 10, payload);
    assertNotNull(content, "Expected to allocated a content object");

    bool status = _athenaLRUContentStore_PutContentObject(impl, content);
    assertFalse(status, "Expected insertion of too large a content object to fail.");

    ccnxContentObject_Release(&content);
    parcBuffer_Release(&payload);

    // Make sure that the contentobjects were added, but that the size didn't grow past the capacity
    assertTrue(impl->currentSizeInBytes == 0, "expected the current store size to be 0.");

    _athenaLRUContentStore_Release((AthenaContentStoreImplementation *) &impl);
}

LONGBOW_TEST_CASE(Loca, _createHashableKey_Name)
{
    CCNxName *name1 = ccnxName_CreateFromCString("lci:/name/1");
    CCNxName *name2 = ccnxName_CreateFromCString("lci:/name/2");

    PARCObject *keyObj1 = _createHashableKey(name1, NULL, NULL);
    PARCObject *keyObj2 = _createHashableKey(name2, NULL, NULL);

    assertNotNull(keyObj1, "Expected non-null key object");
    assertNotNull(keyObj2, "Expected non-null key object");
    assertFalse(parcObject_HashCode(keyObj1) == 0, "Expected non zero hashcode");
    assertFalse(parcObject_HashCode(keyObj2) == 0, "Expected non zero hashcode");
    assertFalse(parcObject_HashCode(keyObj1) == parcObject_HashCode(keyObj2), "Expected different hashcodes");

    parcObject_Release((PARCObject **) &keyObj1);
    parcObject_Release((PARCObject **) &keyObj2);
    ccnxName_Release(&name1);
    ccnxName_Release(&name2);
}

LONGBOW_TEST_CASE(Loca, _createHashableKey_NameAndKeyId)
{
    // Now try with key Ids.

    CCNxName *name1 = ccnxName_CreateFromCString("lci:/name/1");
    CCNxName *name2 = ccnxName_CreateFromCString("lci:/name/2");

    PARCBuffer *keyId1 = parcBuffer_WrapCString("keyId 1");
    PARCBuffer *keyId2 = parcBuffer_WrapCString("keyId 2");

    PARCObject *keyObj1 = _createHashableKey(name1, NULL, NULL);
    PARCObject *keyObj2 = _createHashableKey(name1, keyId1, NULL);

    assertFalse(parcObject_HashCode(keyObj1) == 0, "Expected non zero hashcode");
    assertFalse(parcObject_HashCode(keyObj2) == 0, "Expected non zero hashcode");
    assertFalse(parcObject_HashCode(keyObj1) == parcObject_HashCode(keyObj2), "Expected different hashcodes");

    parcObject_Release((PARCObject **) &keyObj1);
    parcObject_Release((PARCObject **) &keyObj2);

    // Different KeyIds.

    keyObj1 = _createHashableKey(name1, keyId1, NULL);
    keyObj2 = _createHashableKey(name1, keyId2, NULL);

    assertFalse(parcObject_HashCode(keyObj1) == 0, "Expected non zero hashcode");
    assertFalse(parcObject_HashCode(keyObj2) == 0, "Expected non zero hashcode");
    assertFalse(parcObject_HashCode(keyObj1) == parcObject_HashCode(keyObj2), "Expected different hashcodes");

    parcObject_Release((PARCObject **) &keyObj1);
    parcObject_Release((PARCObject **) &keyObj2);
    parcBuffer_Release(&keyId1);
    parcBuffer_Release(&keyId2);
    ccnxName_Release(&name1);
    ccnxName_Release(&name2);
}

LONGBOW_TEST_CASE(Loca, _createHashableKey_NameAndObjectHash)
{
    // Now try with key Ids.

    CCNxName *name1 = ccnxName_CreateFromCString("lci:/name/1");
    CCNxName *name2 = ccnxName_CreateFromCString("lci:/name/2");

    PARCBuffer *objHash1 = parcBuffer_WrapCString("hash 1");
    PARCBuffer *objHash2 = parcBuffer_WrapCString("hash 2");

    PARCObject *keyObj1 = _createHashableKey(name1, NULL, objHash1);
    PARCObject *keyObj2 = _createHashableKey(name1, NULL, NULL);

    assertFalse(parcObject_HashCode(keyObj1) == 0, "Expected non zero hashcode");
    assertFalse(parcObject_HashCode(keyObj2) == 0, "Expected non zero hashcode");
    assertFalse(parcObject_HashCode(keyObj1) == parcObject_HashCode(keyObj2), "Expected different hashcodes");

    parcObject_Release((PARCObject **) &keyObj1);
    parcObject_Release((PARCObject **) &keyObj2);

    // Different object hashes.

    keyObj1 = _createHashableKey(name1, NULL, objHash1);
    keyObj2 = _createHashableKey(name1, NULL, objHash2);

    assertFalse(parcObject_HashCode(keyObj1) == 0, "Expected non zero hashcode");
    assertFalse(parcObject_HashCode(keyObj2) == 0, "Expected non zero hashcode");
    assertFalse(parcObject_HashCode(keyObj1) == parcObject_HashCode(keyObj2), "Expected different hashcodes");

    parcObject_Release((PARCObject **) &keyObj1);
    parcObject_Release((PARCObject **) &keyObj2);
    parcBuffer_Release(&objHash1);
    parcBuffer_Release(&objHash2);

    // Now try with

    ccnxName_Release(&name1);
    ccnxName_Release(&name2);
}

LONGBOW_TEST_CASE(Local, _athenaLRUContentStoreEntry_Display)
{
    CCNxName *name1 = ccnxName_CreateFromCString("lci:/first/entry");
    CCNxContentObject *contentObject1 = ccnxContentObject_CreateWithNameAndPayload(name1, NULL);

    ccnxContentObject_SetExpiryTime(contentObject1, 87654321);
    _AthenaLRUContentStoreEntry *entry1 = _athenaLRUContentStoreEntry_Create(contentObject1);

    _athenaLRUContentStoreEntry_Display(entry1, 4);

    _athenaLRUContentStoreEntry_Release(&entry1);
    ccnxContentObject_Release(&contentObject1);
    ccnxName_Release(&name1);
}

LONGBOW_TEST_CASE(Local, getMatch_Expired)
{
    AthenaLRUContentStore *impl = _createLRUContentStore();

    CCNxName *name = ccnxName_CreateFromCString("lci:/boose/roo/pie");
    CCNxContentObject *contentObject = ccnxContentObject_CreateWithNameAndPayload(name, NULL);

    _AthenaLRUContentStoreEntry *entry = _athenaLRUContentStoreEntry_Create(contentObject);
    ccnxContentObject_Release(&contentObject);

    entry->expiryTime = 10000;
    entry->hasExpiryTime = true;

    bool status = _athenaLRUContentStore_PutLRUContentStoreEntry(impl, entry);

    assertTrue(status, "Expected to put content into the store");

    assertTrue(status, "Expected to put content into the store a second time");
    assertTrue(impl->numEntries == 1, "Expected 1 entry in the store");
    assertTrue(impl->stats.numAdds == 1, "Expected stats to show 1 adds");

    CCNxInterest *interest = ccnxInterest_CreateSimple(name);
    ccnxName_Release(&name);

    CCNxContentObject *match = _athenaLRUContentStore_GetMatch(impl, interest);
    assertNull(match, "Expected to NOT match an interest, due to expired content");

    assertTrue(impl->numEntries == 0, "Expected 0 entries in the store, after removing expired content");

    _athenaLRUContentStoreEntry_Release(&entry);
    ccnxInterest_Release(&interest);

    _athenaLRUContentStore_Release((AthenaContentStoreImplementation *) &impl);
}

LONGBOW_TEST_CASE(Local, _athenaLRUContentStore_RemoveMatch)
{
    testUnimplemented("_athenaLRUContentStore_RemoveMatch not yet implemented");
}

LONGBOW_TEST_CASE(Local, _athenaLRUContentStore_ProcessMessage_StatSize)
{
    AthenaLRUContentStore *impl = _createLRUContentStore();

    CCNxName *name = ccnxName_CreateFromCString(CCNxNameAthena_ContentStore "/stat/size");
    CCNxInterest *interest = ccnxInterest_CreateSimple(name);
    ccnxName_Release(&name);

    CCNxMetaMessage *message = ccnxMetaMessage_CreateFromInterest(interest);
    ccnxInterest_Release(&interest);

    CCNxMetaMessage *response = _athenaLRUContentStore_ProcessMessage(impl, message);

    assertNotNull(response, "Expected a response to ProcessMessage()");
    assertTrue(ccnxMetaMessage_IsContentObject(response), "Expected a content object");

    CCNxContentObject *content = ccnxMetaMessage_GetContentObject(response);

    PARCBuffer *payload = ccnxContentObject_GetPayload(content);
    parcBuffer_Display(payload, 0);

    ccnxMetaMessage_Release(&message);
    ccnxMetaMessage_Release(&response);
    _athenaLRUContentStore_Release((AthenaContentStoreImplementation *) &impl);
}

LONGBOW_TEST_CASE(Local, _athenaLRUContentStore_ProcessMessage_StatHits)
{
    AthenaLRUContentStore *impl = _createLRUContentStore();

    CCNxName *name = ccnxName_CreateFromCString(CCNxNameAthena_ContentStore "/stat/hits");
    CCNxInterest *interest = ccnxInterest_CreateSimple(name);
    ccnxName_Release(&name);

    CCNxMetaMessage *message = ccnxMetaMessage_CreateFromInterest(interest);
    ccnxInterest_Release(&interest);

    CCNxMetaMessage *response = _athenaLRUContentStore_ProcessMessage(impl, message);

    assertNotNull(response, "Expected a response to ProcessMessage()");
    assertTrue(ccnxMetaMessage_IsContentObject(response), "Expected a content object");

    CCNxContentObject *content = ccnxMetaMessage_GetContentObject(response);

    PARCBuffer *payload = ccnxContentObject_GetPayload(content);
    parcBuffer_Display(payload, 0);

    ccnxMetaMessage_Release(&message);
    ccnxMetaMessage_Release(&response);
    _athenaLRUContentStore_Release((AthenaContentStoreImplementation *) &impl);
}


LONGBOW_TEST_CASE(Local, _athenaLRUContentStore_ReleaseAllData)
{
    AthenaLRUContentStore *impl = _createLRUContentStore();
    PARCBuffer *payload = parcBuffer_Allocate(1024); // 1K buffer

    CCNxContentObject *content = _createContentObject("lci:/this/is/content", 1, payload);

    bool status = _athenaLRUContentStore_PutContentObject(impl, content);
    assertTrue(status, "Expected to be able to insert content");
    ccnxContentObject_Release(&content);
    parcBuffer_Release(&payload);
    _athenaLRUContentStore_ReleaseAllData(impl);
    _athenaLRUContentStore_Release((AthenaContentStoreImplementation *) &impl);
}

LONGBOW_TEST_CASE(Local, capacitySetWithExistingContent)
{
    AthenaLRUContentStore *impl = _createLRUContentStore();
    PARCBuffer *payload = parcBuffer_Allocate(1024); // 1K buffer

    // Put stuff in.
    for (int i = 0; i < 10; i++) {
        CCNxContentObject *content = _createContentObject("lci:/this/is/content", i, payload);
        assertNotNull(content, "Expected to allocated a content object");

        bool status = _athenaLRUContentStore_PutContentObject(impl, content);
        assertTrue(status, "Expected to be able to insert content");
        ccnxContentObject_Release(&content);
    }

    assertTrue(impl->numEntries == 10, "Expected 10 entries");

    size_t newSize = 20;
    _athenaLRUContentStore_SetCapacity(impl, newSize);
    assertTrue(impl->numEntries == 0, "Expected 0 entries");

    size_t test = _athenaLRUContentStore_GetCapacity(impl);
    assertTrue(test == newSize, "expected the same size capacity as was set");

    // Put stuff in again.
    for (int i = 0; i < 7; i++) {
        CCNxContentObject *content = _createContentObject("lci:/this/is/content", i, payload);
        assertNotNull(content, "Expected to allocated a content object");

        bool status = _athenaLRUContentStore_PutContentObject(impl, content);
        assertTrue(status, "Expected to be able to insert content");
        ccnxContentObject_Release(&content);
    }

    assertTrue(impl->numEntries == 7, "Expected 7 entries");

    _athenaLRUContentStore_Release((AthenaContentStoreImplementation *) &impl);
    parcBuffer_Release(&payload);
}

LONGBOW_TEST_CASE(Local, _calculateNumberOfInitialBucketsBasedOnCapacityInBytes)
{
    unsigned int numBuckets = _calculateNumberOfInitialBucketsBasedOnCapacityInBytes(1);
    assertTrue(numBuckets >= 43, "expected greater minimum buckets");

    unsigned int numBuckets2 = _calculateNumberOfInitialBucketsBasedOnCapacityInBytes(10 * 1024 * 1024); // 10M
    assertTrue(numBuckets2 > numBuckets, "expected greater minimum buckets");

    unsigned int numBuckets3 = _calculateNumberOfInitialBucketsBasedOnCapacityInBytes(20 * 1024 * 1024); // 20M
    assertTrue(numBuckets3 > numBuckets2, "expected greater minimum buckets");
}


LONGBOW_TEST_FIXTURE(Local)
{
    LONGBOW_RUN_TEST_CASE(Local, _athenaLRUContentStoreEntry_CreateRelease);
    LONGBOW_RUN_TEST_CASE(Local, _athenaLRUContentStore_PutContentObject);
    LONGBOW_RUN_TEST_CASE(Local, _athenaLRUContentStore_PutManyContentObjects);
    LONGBOW_RUN_TEST_CASE(Local, _athenaLRUContentStore_GetMatchByName);
    LONGBOW_RUN_TEST_CASE(Local, _athenaLRUContentStore_GetMatchByNameAndKeyId);
    LONGBOW_RUN_TEST_CASE(Local, _athenaLRUContentStore_GetMatchByNameAndObjectHash);
    LONGBOW_RUN_TEST_CASE(Local, _athenaLRUContentStore_RemoveMatch);
    //LONGBOW_RUN_TEST_CASE(Local, _athenaLRUContentStore_PurgeContentStoreEntry);
    LONGBOW_RUN_TEST_CASE(Local, _athenaLRUContentStoreEntry_Display);

    LONGBOW_RUN_TEST_CASE(Local, _athenaLRUContentStore_PutLRUContentStoreEntry);

    LONGBOW_RUN_TEST_CASE(Local, _moveContentStoreEntryToLRUHead);
    LONGBOW_RUN_TEST_CASE(Local, _getLeastUsedFromLRU);

    LONGBOW_RUN_TEST_CASE(Local, _compareByExpiryTime);
    LONGBOW_RUN_TEST_CASE(Local, _compareByRecommendedCacheTime);

    LONGBOW_RUN_TEST_CASE(Local, putWithExpiryTime);
    LONGBOW_RUN_TEST_CASE(Local, putWithExpiryTime_Expired);

    LONGBOW_RUN_TEST_CASE(Local, putContentAndEnforceCapacity);
    LONGBOW_RUN_TEST_CASE(Local, putTooBig);
    LONGBOW_RUN_TEST_CASE(Local, putContentAndExpireByExpiryTime);

    LONGBOW_RUN_TEST_CASE(Loca, _createHashableKey_Name);
    LONGBOW_RUN_TEST_CASE(Loca, _createHashableKey_NameAndKeyId);
    LONGBOW_RUN_TEST_CASE(Loca, _createHashableKey_NameAndObjectHash);

    LONGBOW_RUN_TEST_CASE(Local, getMatch_Expired);

    LONGBOW_RUN_TEST_CASE(Local, _athenaLRUContentStore_ProcessMessage_StatHits);
    LONGBOW_RUN_TEST_CASE(Local, _athenaLRUContentStore_ProcessMessage_StatSize);

    LONGBOW_RUN_TEST_CASE(Local, _athenaLRUContentStore_ReleaseAllData);

    LONGBOW_RUN_TEST_CASE(Local, capacitySetGet);
    LONGBOW_RUN_TEST_CASE(Local, capacitySetWithExistingContent);

    LONGBOW_RUN_TEST_CASE(Local, _calculateNumberOfInitialBucketsBasedOnCapacityInBytes);
}

LONGBOW_TEST_FIXTURE_SETUP(Local)
{
    return LONGBOW_STATUS_SUCCEEDED;
}

LONGBOW_TEST_FIXTURE_TEARDOWN(Local)
{
    uint32_t outstandingAllocations = parcSafeMemory_ReportAllocation(STDERR_FILENO);
    if (outstandingAllocations != 0) {
        printf("%s leaks memory by %d allocations\n", longBowTestCase_GetName(testCase), outstandingAllocations);
        return LONGBOW_STATUS_MEMORYLEAK;
    }
    return LONGBOW_STATUS_SUCCEEDED;
}

int
main(int argc, char *argv[argc])
{
    LongBowRunner *testRunner = LONGBOW_TEST_RUNNER_CREATE(ccnx_LRUContentStore);
    int exitStatus = longBowMain(argc, argv, testRunner, NULL);
    longBowTestRunner_Destroy(&testRunner);
    exit(exitStatus);
}
