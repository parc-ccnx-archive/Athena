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

// Include the file(s) containing the functions to be tested.
// This permits internal static functions to be visible to this Test Framework.
#include "../athena_ContentStore.c"

#include <LongBow/testing.h>
#include <LongBow/debugging.h>

#include <stdio.h>

#include <parc/algol/parc_SafeMemory.h>
#include <parc/algol/parc_Buffer.h>
#include <parc/algol/parc_Clock.h>

#include <ccnx/common/ccnx_Interest.h>
#include <ccnx/common/ccnx_NameSegmentNumber.h>

#include <ccnx/common/codec/ccnxCodec_TlvPacket.h>
#include <ccnx/common/validation/ccnxValidation_CRC32C.h>

LONGBOW_TEST_RUNNER(test_athena_ContentStore)
{
    parcMemory_SetInterface(&PARCSafeMemoryAsPARCMemory);

    LONGBOW_RUN_TEST_FIXTURE(Local);

    LONGBOW_RUN_TEST_FIXTURE(Global);

    LONGBOW_RUN_TEST_FIXTURE(EmptyImplementation);
}

// The Test Runner calls this function once before any Test Fixtures are run.
LONGBOW_TEST_RUNNER_SETUP(test_athena_ContentStore)
{
    return LONGBOW_STATUS_SUCCEEDED;
}

// The Test Runner calls this function once after all the Test Fixtures are run.
LONGBOW_TEST_RUNNER_TEARDOWN(test_athena_ContentStore)
{
    return LONGBOW_STATUS_SUCCEEDED;
}

LONGBOW_TEST_FIXTURE(Global)
{
    LONGBOW_RUN_TEST_CASE(Global, createRelease);
    LONGBOW_RUN_TEST_CASE(Global, putContent);
    LONGBOW_RUN_TEST_CASE(Global, removeMatch);
    LONGBOW_RUN_TEST_CASE(Global, getMatchByName);
    LONGBOW_RUN_TEST_CASE(Global, setGetCapacity);
    LONGBOW_RUN_TEST_CASE(Global, processMessage);
}

LONGBOW_TEST_FIXTURE_SETUP(Global)
{
    return LONGBOW_STATUS_SUCCEEDED;
}

LONGBOW_TEST_FIXTURE_TEARDOWN(Global)
{
    uint32_t outstandingAllocations = parcSafeMemory_ReportAllocation(STDERR_FILENO);
    if (outstandingAllocations != 0) {
        printf("%s leaks memory by %d allocations\n", longBowTestCase_GetName(testCase), outstandingAllocations);
        return LONGBOW_STATUS_MEMORYLEAK;
    }
    return LONGBOW_STATUS_SUCCEEDED;
}


/**
 * Create a CCNxContentObject with the given LCI name, chunknumber, and payload.
 */
static CCNxContentObject *
_createContentObject(char *lci, uint64_t chunkNum, PARCBuffer *payload)
{
    CCNxName *name = ccnxName_CreateFromCString(lci);
    CCNxNameSegment *chunkSegment = ccnxNameSegmentNumber_Create(CCNxNameLabelType_CHUNK, chunkNum);
    ccnxName_Append(name, chunkSegment);

    CCNxContentObject *result = ccnxContentObject_CreateWithDataPayload(name, payload);

    ccnxName_Release(&name);
    ccnxNameSegment_Release(&chunkSegment);

    return result;
}

LONGBOW_TEST_CASE(Global, createRelease)
{
    AthenaLRUContentStoreConfig config;

    config.capacityInMB = 10;

    AthenaContentStore *store = athenaContentStore_Create(&AthenaContentStore_LRUImplementation, &config);

    athenaContentStore_Release(&store);
}

LONGBOW_TEST_CASE(Global, putContent)
{
    AthenaLRUContentStoreConfig config;
    config.capacityInMB = 10;
    AthenaContentStore *store = athenaContentStore_Create(&AthenaContentStore_LRUImplementation, &config);

    PARCBuffer *payload = parcBuffer_WrapCString("this is a payload");
    CCNxContentObject *contentObject = _createContentObject("lci:/cakes/and/pies", 0, payload);
    parcBuffer_Release(&payload);
    ccnxContentObject_SetExpiryTime(contentObject, 100);

    athenaContentStore_PutContentObject(store, contentObject);

    athenaContentStore_Release(&store);
    ccnxContentObject_Release(&contentObject);
}

LONGBOW_TEST_CASE(Global, removeMatch)
{
    AthenaLRUContentStoreConfig config;
    config.capacityInMB = 10;
    AthenaContentStore *store = athenaContentStore_Create(&AthenaContentStore_LRUImplementation, &config);

    PARCClock *clock = parcClock_Wallclock();
    char *lci = "lci:/cakes/and/pies";

    CCNxName *origName = ccnxName_CreateFromCString(lci);
    CCNxContentObject *contentObject = ccnxContentObject_CreateWithDataPayload(origName, NULL);

    ccnxContentObject_SetExpiryTime(contentObject, parcClock_GetTime(clock) + 100);
    ccnxName_Release(&origName);

    athenaContentStore_PutContentObject(store, contentObject);
    ccnxContentObject_Release(&contentObject);

    CCNxName *testName = ccnxName_CreateFromCString(lci);
    bool status = athenaContentStore_RemoveMatch(store, testName, NULL, NULL);
    // TODO: match on other than name!
    assertTrue(status, "Expected to remove the contentobject we had");

    ccnxName_Release(&testName);
    parcClock_Release(&clock);
    athenaContentStore_Release(&store);
}

LONGBOW_TEST_CASE(Global, getMatchByName)
{
    AthenaLRUContentStoreConfig config;
    config.capacityInMB = 10;
    AthenaContentStore *store = athenaContentStore_Create(&AthenaContentStore_LRUImplementation, &config);

    char *lci = "lci:/cakes/and/pies";

    PARCClock *clock = parcClock_Wallclock();

    CCNxName *truthName = ccnxName_CreateFromCString(lci);
    CCNxContentObject *truthObject = ccnxContentObject_CreateWithDataPayload(truthName, NULL);
    ccnxContentObject_SetExpiryTime(truthObject, parcClock_GetTime(clock) + 100);
    ccnxName_Release(&truthName);

    athenaContentStore_PutContentObject(store, truthObject);

    CCNxName *testName = ccnxName_CreateFromCString(lci);
    CCNxInterest *interest = ccnxInterest_CreateSimple(testName);
    ccnxName_Release(&testName);

    //athena_EncodeMessage(interest);

    CCNxContentObject *testObject = athenaContentStore_GetMatch(store, interest);
    ccnxInterest_Release(&interest);

    // TODO: match on other than name!
    assertTrue(ccnxContentObject_Equals(truthObject, testObject), "Expected to get the same ContentObject back");
    assertTrue(truthObject == testObject, "Expected the same pointer back");

    athenaContentStore_Release(&store);
    ccnxContentObject_Release(&truthObject);
    parcClock_Release(&clock);
}

LONGBOW_TEST_CASE(Global, setGetCapacity)
{
    AthenaLRUContentStoreConfig config;
    config.capacityInMB = 10;
    AthenaContentStore *store = athenaContentStore_Create(&AthenaContentStore_LRUImplementation, &config);

    size_t capacity = athenaContentStore_GetCapacity(store);
    assertTrue(capacity == config.capacityInMB, "Expected the same capacity as we specified at init");

    size_t newCapacity = 20;
    athenaContentStore_SetCapacity(store, newCapacity);

    assertTrue(newCapacity == athenaContentStore_GetCapacity(store), "Expected to see the new capacity");

    athenaContentStore_Release(&store);
}

LONGBOW_TEST_CASE(Global, processMessage)
{
    AthenaLRUContentStoreConfig config;
    config.capacityInMB = 10;
    AthenaContentStore *store = athenaContentStore_Create(&AthenaContentStore_LRUImplementation, &config);

    CCNxName *name = ccnxName_CreateFromCString(CCNxNameAthena_ContentStore "/stat/size");
    CCNxInterest *interest = ccnxInterest_CreateSimple(name);

    CCNxMetaMessage *message = ccnxMetaMessage_CreateFromInterest(interest);

    athena_EncodeMessage(message);

    CCNxMetaMessage *result = athenaContentStore_ProcessMessage(store, message);

    assertNotNull(result, "Expected a response from the store");

    ccnxMetaMessage_Release(&result);
    ccnxMetaMessage_Release(&message);
    ccnxInterest_Release(&interest);
    ccnxName_Release(&name);

    athenaContentStore_Release(&store);
}


/***
 ***  Local Tests
 ***/

LONGBOW_TEST_FIXTURE(Local)
{
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

/***
 ***  Empty Implementation Tests
 ***/

typedef struct emptyImpl {
} EmptyImpl;


parcObject_ImplementAcquire(emptyImpl, EmptyImpl);
parcObject_ImplementRelease(emptyImpl, EmptyImpl);

// _destroy, _copy, _toString, _equals, _compare, _hashCode, _toJSON
parcObject_ExtendPARCObject(EmptyImpl,
                            NULL, // finalize
                            NULL, // copy
                            NULL, // toString
                            NULL, // equals,
                            NULL, // compare
                            NULL, // hashCode
                            NULL  // toJSON
                            );


static AthenaContentStoreImplementation *
_emptyImplCreate()
{
    EmptyImpl *result = parcObject_CreateAndClearInstance(EmptyImpl);
    return result;
}

AthenaContentStoreInterface EmptyContentStoreImplementation = {
    .description      = "Empty Implementation",
    .create           = _emptyImplCreate,
    .release          = NULL,

    .putContentObject = NULL,
    .getMatch         = NULL,
    .removeMatch      = NULL,

    .getCapacity      = NULL,
    .setCapacity      = NULL,
    .processMessage   = NULL
};


static AthenaContentStore *
_emptyImplSetup()
{
    return athenaContentStore_Create(&EmptyContentStoreImplementation, NULL);
}

static void
_emptyImplTeardown(AthenaContentStore *store)
{
    athenaContentStore_Release(&store);
}

LONGBOW_TEST_FIXTURE(EmptyImplementation)
{
    LONGBOW_RUN_TEST_CASE(EmptyImplementation, apiFunctions);
    LONGBOW_RUN_TEST_CASE(EmptyImplementation, booleanApiFunctions);
    LONGBOW_RUN_TEST_CASE(EmptyImplementation, trappingApiFunctions);
}

LONGBOW_TEST_FIXTURE_SETUP(EmptyImplementation)
{
    longBowTestCase_SetClipBoardData(testCase, _emptyImplSetup());

    return LONGBOW_STATUS_SUCCEEDED;
}

LONGBOW_TEST_FIXTURE_TEARDOWN(EmptyImplementation)
{
    _emptyImplTeardown(longBowTestCase_GetClipBoardData(testCase));

    uint32_t outstandingAllocations = parcSafeMemory_ReportAllocation(STDERR_FILENO);
    if (outstandingAllocations != 0) {
        printf("%s leaks memory by %d allocations\n", longBowTestCase_GetName(testCase), outstandingAllocations);
        return LONGBOW_STATUS_MEMORYLEAK;
    }
    return LONGBOW_STATUS_SUCCEEDED;
}

LONGBOW_TEST_CASE(EmptyImplementation, apiFunctions)
{
    AthenaContentStore *store = athenaContentStore_Create(&EmptyContentStoreImplementation, NULL);
    CCNxName *name = ccnxName_CreateFromCString("lci:/pie/is/always/good");
    CCNxInterest *interest = ccnxInterest_CreateSimple(name);

    athena_EncodeMessage(interest);

    assertNull(athenaContentStore_ProcessMessage(store, interest), "Expected a NULL response.");

    ccnxName_Release(&name);
    ccnxInterest_Release(&interest);
    athenaContentStore_Release(&store);
}

LONGBOW_TEST_CASE(EmptyImplementation, booleanApiFunctions)
{
    AthenaContentStore *store = athenaContentStore_Create(&EmptyContentStoreImplementation, NULL);

    CCNxContentObject *contentObject = _createContentObject("lci:/dogs/are/better/than/cats", 10, NULL);
    CCNxName *name = ccnxName_CreateFromCString("lci:/pie/is/always/good");
    CCNxInterest *interest = ccnxInterest_CreateSimple(name);

    //athena_EncodeMessage(interest);

    AthenaContentStore *ref = athenaContentStore_Acquire(store);
    athenaContentStore_Release(&ref);

    assertFalse(athenaContentStore_PutContentObject(store, contentObject), "Expected false from PutContentObject");
    assertFalse(athenaContentStore_GetMatch(store, interest), "Expected false from GetMatch");
    assertFalse(athenaContentStore_SetCapacity(store, 1), "Expected false from SetCapacity");
    assertFalse(athenaContentStore_RemoveMatch(store, name, NULL, NULL), "Expected false from RemoveMatch");

    ccnxName_Release(&name);
    ccnxInterest_Release(&interest);
    ccnxContentObject_Release(&contentObject);
    athenaContentStore_Release(&store);
}

LONGBOW_TEST_CASE_EXPECTS(EmptyImplementation, trappingApiFunctions, .event = &LongBowTrapNotImplemented)
{
    AthenaContentStore *store = longBowTestCase_GetClipBoardData(testCase);
    size_t capacity = athenaContentStore_GetCapacity(store); // This will trap.
    printf("We should never see this line: %zu\n", capacity);
}

int
main(int argc, char *argv[argc])
{
    LongBowRunner *testRunner = LONGBOW_TEST_RUNNER_CREATE(test_athena_ContentStore);
    int exitStatus = longBowMain(argc, argv, testRunner, NULL);
    longBowTestRunner_Destroy(&testRunner);
    exit(exitStatus);
}
