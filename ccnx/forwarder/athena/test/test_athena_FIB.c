/*
 * Copyright (c) 2013-2015, Xerox Corporation (Xerox)and Palo Alto Research Center (PARC)
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
 * @copyright 2013-2015, Xerox Corporation (Xerox)and Palo Alto Research Center (PARC).  All rights reserved.
 */

// Include the file(s) containing the functions to be tested.
// This permits internal static functions to be visible to this Test Framework.
#include "../athena_FIB.c"

#include <parc/algol/parc_SafeMemory.h>
#include <LongBow/unit-test.h>

#include <stdio.h>

typedef struct test_data {
    AthenaFIB *testFIB;
    CCNxName *testName1;
    CCNxName *testName2;
    CCNxName *testName3;
    PARCBitVector *testVector1;
    PARCBitVector *testVector2;
    PARCBitVector *testVector12;
    PARCBitVector *testVector3;
} TestData;


LONGBOW_TEST_RUNNER(athena_FIB)
{
    // The following Test Fixtures will run their corresponding Test Cases.
    // Test Fixtures are run in the order specified, but all tests should be idempotent.
    // Never rely on the execution order of tests or share state between them.
    LONGBOW_RUN_TEST_FIXTURE(Global);
}

// The Test Runner calls this function once before any Test Fixtures are run.
LONGBOW_TEST_RUNNER_SETUP(athena_FIB)
{
    parcMemory_SetInterface(&PARCSafeMemoryAsPARCMemory);
    return LONGBOW_STATUS_SUCCEEDED;
}

// The Test Runner calls this function once after all the Test Fixtures are run.
LONGBOW_TEST_RUNNER_TEARDOWN(athena_FIB)
{
    return LONGBOW_STATUS_SUCCEEDED;
}

// ========================================================================================

LONGBOW_TEST_FIXTURE(Global)
{
    LONGBOW_RUN_TEST_CASE(Global, athenaFIB_Create);
    LONGBOW_RUN_TEST_CASE(Global, athenaFIB_AcquireRelease);
    LONGBOW_RUN_TEST_CASE(Global, athenaFIB_AddRoute);
    LONGBOW_RUN_TEST_CASE(Global, athenaFIB_Lookup);
    LONGBOW_RUN_TEST_CASE(Global, athenaFIB_Lookup_EmptyPath);
    LONGBOW_RUN_TEST_CASE(Global, athenaFIB_DeleteRoute);
    LONGBOW_RUN_TEST_CASE(Global, athenaFIB_RemoveLink);
    LONGBOW_RUN_TEST_CASE(Global, athenaFIB_CreateEntryList);
//    LONGBOW_RUN_TEST_CASE(Global, athenaFIB_Equals);
//    LONGBOW_RUN_TEST_CASE(Global, athenaFIB_NotEquals);
//    LONGBOW_RUN_TEST_CASE(Global, athenaFIB_ToString);
}

LONGBOW_TEST_FIXTURE_SETUP(Global)
{
    TestData *data = parcMemory_AllocateAndClear(sizeof(TestData));
    assertNotNull(data, "parcMemory_AllocateAndClear(%lu) returned NULL", sizeof(TestData));

    data->testFIB = athenaFIB_Create();
    data->testName1 = ccnxName_CreateFromURI("lci:/a/b/c");
    data->testName2 = ccnxName_CreateFromURI("lci:/a/b/a");
    data->testName3 = ccnxName_CreateFromURI("lci:/");
    data->testVector1 = parcBitVector_Create();
    parcBitVector_Set(data->testVector1, 0);
    data->testVector2 = parcBitVector_Create();
    parcBitVector_Set(data->testVector2, 42);
    data->testVector12 = parcBitVector_Create();
    parcBitVector_Set(data->testVector12, 0);
    parcBitVector_Set(data->testVector12, 42);
    data->testVector3 = parcBitVector_Create();
    parcBitVector_Set(data->testVector3, 23);

    longBowTestCase_SetClipBoardData(testCase, data);

    return LONGBOW_STATUS_SUCCEEDED;
}

LONGBOW_TEST_FIXTURE_TEARDOWN(Global)
{
    TestData *data = longBowTestCase_GetClipBoardData(testCase);
    athenaFIB_Release(&data->testFIB);
    ccnxName_Release(&data->testName1);
    ccnxName_Release(&data->testName2);
    ccnxName_Release(&data->testName3);
    parcBitVector_Release(&data->testVector1);
    parcBitVector_Release(&data->testVector2);
    parcBitVector_Release(&data->testVector12);
    parcBitVector_Release(&data->testVector3);

    parcMemory_Deallocate((void **) &data);

    if (parcSafeMemory_ReportAllocation(STDOUT_FILENO) != 0) {
        printf("('%s' leaks memory by %d (allocs - frees)) ", longBowTestCase_GetName(testCase), parcMemory_Outstanding());
        return LONGBOW_STATUS_TEARDOWN_FAILED;
    }
    return LONGBOW_STATUS_SUCCEEDED;
}

LONGBOW_TEST_CASE(Global, athenaFIB_Create)
{
    AthenaFIB *fib = athenaFIB_Create();
    assertNotNull(fib, "Expected athenaFIB_Create to return a non-NULL value");

    athenaFIB_Release(&fib);
    assertNull(fib, "Expected athenaFIB_Release to NULL the pointer");
}


LONGBOW_TEST_CASE(Global, athenaFIB_AcquireRelease)
{
    TestData *data = longBowTestCase_GetClipBoardData(testCase);

    AthenaFIB *acquiredFib = athenaFIB_Acquire(data->testFIB);
    assertNotNull(acquiredFib, "Expected athenaFIB_Acquire to return a non-NULL value");

    athenaFIB_Release(&acquiredFib);
    assertNull(acquiredFib, "Expected athenaFIB_Release to NULL the pointer");
}

LONGBOW_TEST_CASE(Global, athenaFIB_AddRoute)
{
    TestData *data = longBowTestCase_GetClipBoardData(testCase);

    athenaFIB_AddRoute(data->testFIB, data->testName1, data->testVector1);
}

LONGBOW_TEST_CASE(Global, athenaFIB_Lookup)
{
    TestData *data = longBowTestCase_GetClipBoardData(testCase);

    athenaFIB_AddRoute(data->testFIB, data->testName1, data->testVector1);
    PARCBitVector *result = athenaFIB_Lookup(data->testFIB, data->testName1);
    assertTrue(parcBitVector_Equals(result, data->testVector1), "Expected lookup to equal test vector");
}

LONGBOW_TEST_CASE(Global, athenaFIB_Lookup_EmptyPath)
{
    TestData *data = longBowTestCase_GetClipBoardData(testCase);

    athenaFIB_AddRoute(data->testFIB, data->testName3, data->testVector1);
    PARCBitVector *result = athenaFIB_Lookup(data->testFIB, data->testName3);
    assertNotNull(result, "Expect non-null match to global path (\"/\")");
    assertTrue(parcBitVector_Equals(result, data->testVector1), "Expected lookup to equal test vector");
    result = athenaFIB_Lookup(data->testFIB, data->testName1);
    assertNotNull(result, "Expect non-null match to global path (\"/\")");
    assertTrue(parcBitVector_Equals(result, data->testVector1), "Expected lookup to equal test vector");
    result = athenaFIB_Lookup(data->testFIB, data->testName2);
    assertNotNull(result, "Expect non-null match to global path (\"/\")");
    assertTrue(parcBitVector_Equals(result, data->testVector1), "Expected lookup to equal test vector");

    athenaFIB_AddRoute(data->testFIB, data->testName3, data->testVector2);
    result = athenaFIB_Lookup(data->testFIB, data->testName3);
    assertNotNull(result, "Expect non-null match to global path (\"/\")");
    assertTrue(parcBitVector_Equals(result, data->testVector12), "Expected lookup to equal test vector");
    result = athenaFIB_Lookup(data->testFIB, data->testName1);
    assertNotNull(result, "Expect non-null match to global path (\"/\")");
    assertTrue(parcBitVector_Equals(result, data->testVector12), "Expected lookup to equal test vector");
    result = athenaFIB_Lookup(data->testFIB, data->testName2);
    assertNotNull(result, "Expect non-null match to global path (\"/\")");
    assertTrue(parcBitVector_Equals(result, data->testVector12), "Expected lookup to equal test vector");
}

LONGBOW_TEST_CASE(Global, athenaFIB_DeleteRoute)
{
    TestData *data = longBowTestCase_GetClipBoardData(testCase);

    athenaFIB_AddRoute(data->testFIB, data->testName1, data->testVector12);

    PARCBitVector *result = athenaFIB_Lookup(data->testFIB, data->testName1);
    assertTrue(parcBitVector_Equals(result, data->testVector12), "Expected lookup to equal test vector");

    athenaFIB_DeleteRoute(data->testFIB, data->testName1, data->testVector1);
    result = athenaFIB_Lookup(data->testFIB, data->testName1);
    assertTrue(parcBitVector_Equals(result, data->testVector2), "Expected lookup to equal test vector");

    athenaFIB_DeleteRoute(data->testFIB, data->testName1, data->testVector12);
    result = athenaFIB_Lookup(data->testFIB, data->testName1);
    assertNull(result, "Expecting a NULL result from Lookup after Delete Route");
}

LONGBOW_TEST_CASE(Global, athenaFIB_RemoveLink)
{
    TestData *data = longBowTestCase_GetClipBoardData(testCase);

    athenaFIB_AddRoute(data->testFIB, data->testName1, data->testVector1);
    athenaFIB_AddRoute(data->testFIB, data->testName2, data->testVector2);

    PARCBitVector *result = athenaFIB_Lookup(data->testFIB, data->testName1);
    assertTrue(parcBitVector_Equals(result, data->testVector1), "Expected lookup to equal test vector");
    result = athenaFIB_Lookup(data->testFIB, data->testName2);
    assertTrue(parcBitVector_Equals(result, data->testVector2), "Expected lookup to equal test vector");

    athenaFIB_RemoveLink(data->testFIB, data->testVector1);
    result = athenaFIB_Lookup(data->testFIB, data->testName1);
    assertNull(result, "Expecting a NULL result from Lookup after Delete Route");
    result = athenaFIB_Lookup(data->testFIB, data->testName2);
    assertTrue(parcBitVector_Equals(result, data->testVector2), "Expected lookup to equal test vector");

    athenaFIB_AddRoute(data->testFIB, data->testName1, data->testVector12);

    athenaFIB_RemoveLink(data->testFIB, data->testVector2);
    result = athenaFIB_Lookup(data->testFIB, data->testName2);
    assertNull(result, "Expecting a NULL result from Lookup after Delete Route");
    result = athenaFIB_Lookup(data->testFIB, data->testName1);
    assertTrue(parcBitVector_Equals(result, data->testVector1), "Expected lookup to equal test vector");
}

LONGBOW_TEST_CASE(Global, athenaFIB_CreateEntryList)
{
    TestData *data = longBowTestCase_GetClipBoardData(testCase);

    athenaFIB_AddRoute(data->testFIB, data->testName1, data->testVector12);

    PARCBitVector *result = athenaFIB_Lookup(data->testFIB, data->testName1);
    assertTrue(parcBitVector_Equals(result, data->testVector12), "Expected lookup to equal test vector");

    PARCList *entryList = athenaFIB_CreateEntryList(data->testFIB);
    assertTrue(parcList_Size(entryList) == 2, "Expected the EntryList to have 2 elements");

    AthenaFIBListEntry *entry = parcList_GetAtIndex(entryList, 0);
    assertNotNull(entry, "Expect entry at 0 to be non-NULL");
    assertTrue(ccnxName_Equals(data->testName1, entry->name), "Expect the name at 0 to be testName1");
    assertTrue(entry->linkId == 0, "Expect the routeId at 0 to be 0");

    entry = parcList_GetAtIndex(entryList, 1);
    assertNotNull(entry, "Expect entry at 1 to be non-NULL");
    assertTrue(ccnxName_Equals(data->testName1, entry->name), "Expect the name at 1 to be testName1");
    assertTrue(entry->linkId == 42, "Expect the routeId at 0 to be 42");

    parcList_Release(&entryList);
}


//LONGBOW_TEST_CASE(Global, athenaFIB_Equals)
//{
//    TestData *data = longBowTestCase_GetClipBoardData(testCase);
//}

//LONGBOW_TEST_CASE(Global, athenaFIB_NotEquals)
//{
//}

//LONGBOW_TEST_CASE(Global, athenaFIB_ToString)
//{
//    TestData *data = longBowTestCase_GetClipBoardData(testCase);
//}



int
main(int argc, char *argv[])
{
    LongBowRunner *testRunner = LONGBOW_TEST_RUNNER_CREATE(athena_FIB);
    int exitStatus = longBowMain(argc, argv, testRunner, NULL);
    longBowTestRunner_Destroy(&testRunner);
    exit(exitStatus);
}
