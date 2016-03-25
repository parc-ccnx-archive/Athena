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
 * @author Michael Slominski, Palo Alto Research Center (Xerox PARC)
 * @copyright 2015, Xerox Corporation (Xerox)and Palo Alto Research Center (PARC).  All rights reserved.
 */

// Include the file(s) containing the functions to be tested.
// This permits internal static functions to be visible to this Test Runner.
#include "../athena_PIT.c"

#include <LongBow/unit-test.h>

#include <stdio.h>

#include <parc/algol/parc_SafeMemory.h>
#include <parc/algol/parc_StdlibMemory.h>
#include <parc/testing/parc_MemoryTesting.h>
#include <parc/testing/parc_ObjectTesting.h>

#include <ccnx/common/validation/ccnxValidation_CRC32C.h>
#include <ccnx/common/codec/ccnxCodec_TlvPacket.h>



#include <math.h>


static
struct timeval _TestClockTimeval = {
    .tv_sec  = 0,
    .tv_usec = 0
};

static void
_testClock_GetTimeval(const PARCClock *dummy __attribute__((unused)), struct timeval *output)
{
    output->tv_sec = _TestClockTimeval.tv_sec;
    output->tv_usec = _TestClockTimeval.tv_usec;
}

static uint64_t
_testClock_GetTime(const PARCClock *clock)
{
    struct timeval tv;
    _testClock_GetTimeval(clock, &tv);
    uint64_t t = tv.tv_sec * 1000 + tv.tv_usec / 1000;

    return t;
}

static PARCClock *
_testClock_Acquire(const PARCClock *clock)
{
    return (PARCClock *) clock;
}

static void
_testClock_Release(PARCClock **clockPtr)
{
    *clockPtr = NULL;
}


static PARCClock _TestClock = {
    .closure    = NULL,
    .getTime    = _testClock_GetTime,
    .getTimeval = _testClock_GetTimeval,
    .acquire    = _testClock_Acquire,
    .release    = _testClock_Release
};

static PARCClock *
parcClock_Test(void)
{
    return &_TestClock;
}

LONGBOW_TEST_RUNNER(athena_PIT)
{
    // The following Test Fixtures will run their corresponding Test Cases.
    // Test Fixtures are run in the order specified here, but every test must be idempotent.
    // Never rely on the execution order of tests or share state between them.
    LONGBOW_RUN_TEST_FIXTURE(CreateAcquireRelease);
    LONGBOW_RUN_TEST_FIXTURE(Global);
    LONGBOW_RUN_TEST_FIXTURE(Performance);
}

// The Test Runner calls this function once before any Test Fixtures are run.
LONGBOW_TEST_RUNNER_SETUP(athena_PIT)
{
    return LONGBOW_STATUS_SUCCEEDED;
}

// The Test Runner calls this function once after all the Test Fixtures are run.
LONGBOW_TEST_RUNNER_TEARDOWN(athena_PIT)
{
    return LONGBOW_STATUS_SUCCEEDED;
}

LONGBOW_TEST_FIXTURE(CreateAcquireRelease)
{
    LONGBOW_RUN_TEST_CASE(CreateAcquireRelease, CreateRelease);
}

const PARCMemoryInterface *savedMemoryModule = NULL;

LONGBOW_TEST_FIXTURE_SETUP(CreateAcquireRelease)
{
    savedMemoryModule = parcMemory_SetInterface(&PARCSafeMemoryAsPARCMemory);
    return LONGBOW_STATUS_SUCCEEDED;
}

LONGBOW_TEST_FIXTURE_TEARDOWN(CreateAcquireRelease)
{
    if (!parcMemoryTesting_ExpectedOutstanding(0, "%s leaked memory.", longBowTestCase_GetFullName(testCase))) {
        return LONGBOW_STATUS_MEMORYLEAK;
    }

    parcMemory_SetInterface(savedMemoryModule);
    return LONGBOW_STATUS_SUCCEEDED;
}

LONGBOW_TEST_CASE(CreateAcquireRelease, CreateRelease)
{
    AthenaPIT *instance = athenaPIT_Create();
    assertNotNull(instance, "Expeced non-null result from parcHashMap_Create();");
    parcObjectTesting_AssertAcquireReleaseContract(athenaPIT_Acquire, instance);

    athenaPIT_Release(&instance);
    assertNull(instance, "Expeced null result from parcHashMap_Release();");
}


LONGBOW_TEST_FIXTURE(Global)
{
    LONGBOW_RUN_TEST_CASE(Global, athenaPIT_AddInterest);
    LONGBOW_RUN_TEST_CASE(Global, athenaPIT_RemoveInterest);
    LONGBOW_RUN_TEST_CASE(Global, athenaPIT_Match_NoRestriction);
    LONGBOW_RUN_TEST_CASE(Global, athenaPIT_Match_KeyIdRestriction);
    LONGBOW_RUN_TEST_CASE(Global, athenaPIT_Match_ContentHashRestriction);
    LONGBOW_RUN_TEST_CASE(Global, athenaPIT_Match_Nameless);
    LONGBOW_RUN_TEST_CASE(Global, athenaPIT_Match_MultipleRestrictions);
    LONGBOW_RUN_TEST_CASE(Global, athenaPIT_CreateCapacity);
    LONGBOW_RUN_TEST_CASE(Global, athenaPIT_PurgeExpired);
    LONGBOW_RUN_TEST_CASE(Global, athenaPIT_RemoveLink);
    LONGBOW_RUN_TEST_CASE(Global, athenaPIT_LinkCleanupFromMatch);
    LONGBOW_RUN_TEST_CASE(Global, athenaPIT_GetNumberOfTableEntries);
    LONGBOW_RUN_TEST_CASE(Global, athenaPIT_GetNumberOfPendingInterests);
    LONGBOW_RUN_TEST_CASE(Global, athenaPIT_GetMeanEntryLifetime);

    LONGBOW_RUN_TEST_CASE(Global, athenaPIT_ProcessMessage_Size);
    LONGBOW_RUN_TEST_CASE(Global, athenaPIT_ProcessMessage_AvgEntryLifetime);
    LONGBOW_RUN_TEST_CASE(Global, athenaPIT_CreateEntryList);
}

typedef struct test_data {
    AthenaPIT *testPIT;
    CCNxInterest *testInterest1;
    CCNxInterest *testInterest1WithKeyId;
    CCNxInterest *testInterest1WithContentId;
    CCNxInterest *testInterest2;
    CCNxInterest *testNamelessInterest;

    CCNxContentObject *testContent1;
    CCNxContentObject *testContent1Prime;
    CCNxContentObject *testContent1WithSig;
    CCNxContentObject *testContent2;
    CCNxContentObject *testNamelessContent;

    PARCBitVector *testVector1;
    PARCBitVector *testVector2;
    PARCBitVector *testVector3;
    PARCBitVector *testVector12;
    PARCBitVector *testVector123;
} TestData;

uint32_t TEST_INTEREST_LIFETIME = 100; /* lifetime, in milliseconds */
static CCNxInterest *
_createTestInterest(const CCNxName *name,
                    const PARCBuffer *keyIdRestriction,
                    const PARCBuffer *hashRestriction)
{
    CCNxInterest *interest = ccnxInterest_Create(name,
                                                 TEST_INTEREST_LIFETIME,     /* lifetime, 15 seconds in milliseconds */
                                                 keyIdRestriction,           /* KeyId */
                                                 hashRestriction           /* ContentObjectHash */
                                                 );
    return interest;
}

static PARCBuffer *
_iovecToBuffer(CCNxCodecNetworkBufferIoVec *iovec)
{
    size_t iovcnt = ccnxCodecNetworkBufferIoVec_GetCount(iovec);
    const struct iovec *array = ccnxCodecNetworkBufferIoVec_GetArray(iovec);
    ccnxCodecNetworkBufferIoVec_Release(&iovec);

    size_t totalbytes = 0;
    for (int i = 0; i < iovcnt; i++) {
        totalbytes += array[i].iov_len;
    }

    PARCBuffer *encodedMsg = parcBuffer_Allocate(totalbytes);
    for (int i = 0; i < iovcnt; i++) {
        parcBuffer_PutArray(encodedMsg, array[i].iov_len, array[i].iov_base);
    }

    parcBuffer_Flip(encodedMsg);

    return encodedMsg;
}

static CCNxContentObject *
_createReceivedContent(CCNxContentObject *preSendContent)
{
    PARCSigner *signer = ccnxValidationCRC32C_CreateSigner();
    CCNxCodecNetworkBufferIoVec *iovec = ccnxCodecTlvPacket_DictionaryEncode(preSendContent, signer);
    assertTrue(ccnxWireFormatMessage_PutIoVec(preSendContent, iovec), "ccnxWireFormatMessage_PutIoVec failed");;
    parcSigner_Release(&signer);

    PARCBuffer *encodedBuffer = _iovecToBuffer(iovec);

    CCNxContentObject *postSendContent = ccnxMetaMessage_CreateFromWireFormatBuffer(encodedBuffer);

    parcBuffer_Release(&encodedBuffer);

    return postSendContent;
}

static PARCBuffer *
_createMessageHash(const CCNxMetaMessage *metaMessage)
{
    CCNxWireFormatMessage *wireFormatMessage = (CCNxWireFormatMessage *) metaMessage;

    PARCCryptoHash *hash = ccnxWireFormatMessage_CreateContentObjectHash(wireFormatMessage);
    PARCBuffer *buffer = parcBuffer_Acquire(parcCryptoHash_GetDigest(hash));
    parcCryptoHash_Release(&hash);

    return buffer;
}


LONGBOW_TEST_FIXTURE_SETUP(Global)
{
    savedMemoryModule = parcMemory_SetInterface(&PARCSafeMemoryAsPARCMemory);

    TestData *data = parcMemory_AllocateAndClear(sizeof(TestData));
    assertNotNull(data, "parcMemory_AllocateAndClear(%lu) returned NULL", sizeof(TestData));

    data->testPIT = athenaPIT_Create();

    // Content 1
    CCNxName *name = ccnxName_CreateFromCString("lci:/test/content");
    PARCBuffer *payload = parcBuffer_WrapCString("Some really hot payload 1");

    CCNxContentObject *preSendCO = ccnxContentObject_CreateWithNameAndPayload(name, payload);
    data->testContent1 = _createReceivedContent(preSendCO);
    ccnxContentObject_Release(&preSendCO);
    parcBuffer_Release(&payload);

    // Nameless Content 1
    PARCBuffer *namelessPayload = parcBuffer_WrapCString("Some really super hot payload for a nameless object");
    preSendCO = ccnxContentObject_CreateWithPayload(namelessPayload);
    data->testNamelessContent = _createReceivedContent(preSendCO);
    ccnxContentObject_Release(&preSendCO);
    parcBuffer_Release(&namelessPayload);

    // Content 1 Prime
    PARCBuffer *payloadPrime = parcBuffer_WrapCString("Some really hot payload 1 prime");
    preSendCO = ccnxContentObject_CreateWithNameAndPayload(name, payloadPrime);
    parcBuffer_Release(&payloadPrime);
    data->testContent1Prime = _createReceivedContent(preSendCO);
    ccnxContentObject_Release(&preSendCO);

    // Content 1 With Sig
    payload = parcBuffer_WrapCString("Some really hot payload 1");
    preSendCO = ccnxContentObject_CreateWithNameAndPayload(name, payload);
    parcBuffer_Release(&payload);

    PARCBuffer *keyId = parcBuffer_WrapCString("keyhash");
    PARCBuffer *sigbits = parcBuffer_WrapCString("siggybits");
    PARCSignature *signature = parcSignature_Create(PARCSigningAlgorithm_RSA, PARC_HASH_SHA256, sigbits);
    parcBuffer_Release(&sigbits);
    ccnxContentObject_SetSignature(preSendCO, keyId, signature, NULL);
    parcSignature_Release(&signature);
    data->testContent1WithSig = _createReceivedContent(preSendCO);
    assertNotNull(data->testContent1WithSig, "Could not create signed contentObject");
    ccnxContentObject_Release(&preSendCO);

    // Interest 1
    data->testInterest1 = _createTestInterest(name, NULL, NULL);

    // Interest 1 With KeyId
    data->testInterest1WithKeyId = _createTestInterest(name, keyId, NULL);

    // Interest 1 With ContentId
    PARCCryptoHash *contentHash = ccnxWireFormatMessage_CreateContentObjectHash(data->testContent1WithSig);
    data->testInterest1WithContentId = _createTestInterest(name, keyId, parcCryptoHash_GetDigest(contentHash));
    parcCryptoHash_Release(&contentHash);

    // Interest for Nameless Content Object
    contentHash = ccnxWireFormatMessage_CreateContentObjectHash(data->testNamelessContent);
    data->testNamelessInterest = _createTestInterest(name, keyId, parcCryptoHash_GetDigest(contentHash));
    parcCryptoHash_Release(&contentHash);

    parcBuffer_Release(&keyId);
    ccnxName_Release(&name);

    // Content 2
    name = ccnxName_CreateFromCString("lci:/test/content2");
    payload = parcBuffer_WrapCString("Some really hot payload 2");
    preSendCO = ccnxContentObject_CreateWithNameAndPayload(name, payload);
    data->testContent2 = _createReceivedContent(preSendCO);
    ccnxContentObject_Release(&preSendCO);
    parcBuffer_Release(&payload);
    ccnxName_Release(&name);

    // Interest 2
    name = ccnxName_CreateFromCString("lci:/test/content2");
    data->testInterest2 = _createTestInterest(name, NULL, NULL);
    ccnxName_Release(&name);

    // test bitVectors
    data->testVector1 = parcBitVector_Create();
    parcBitVector_Set(data->testVector1, 0);
    data->testVector2 = parcBitVector_Create();
    parcBitVector_Set(data->testVector2, 42);
    data->testVector3 = parcBitVector_Create();
    parcBitVector_Set(data->testVector3, 23);
    data->testVector12 = parcBitVector_Create();
    parcBitVector_Set(data->testVector12, 0);
    parcBitVector_Set(data->testVector12, 42);
    data->testVector123 = parcBitVector_Create();
    parcBitVector_Set(data->testVector123, 0);
    parcBitVector_Set(data->testVector123, 23);
    parcBitVector_Set(data->testVector123, 42);

    longBowTestCase_SetClipBoardData(testCase, data);

    return LONGBOW_STATUS_SUCCEEDED;
}

LONGBOW_TEST_FIXTURE_TEARDOWN(Global)
{
    TestData *data = longBowTestCase_GetClipBoardData(testCase);
    athenaPIT_Release(&data->testPIT);

    ccnxInterest_Release((&data->testInterest1));
    ccnxInterest_Release((&data->testInterest1WithKeyId));
    ccnxInterest_Release((&data->testInterest1WithContentId));
    ccnxInterest_Release((&data->testInterest2));
    ccnxInterest_Release((&data->testNamelessInterest));

    ccnxContentObject_Release(&data->testContent1);
    ccnxContentObject_Release(&data->testContent1Prime);
    ccnxContentObject_Release(&data->testContent1WithSig);
    ccnxContentObject_Release(&data->testContent2);
    ccnxContentObject_Release(&data->testNamelessContent);

    parcBitVector_Release(&data->testVector1);
    parcBitVector_Release(&data->testVector2);
    parcBitVector_Release(&data->testVector3);
    parcBitVector_Release(&data->testVector12);
    parcBitVector_Release(&data->testVector123);

    parcMemory_Deallocate((void **) &data);

    if (parcSafeMemory_ReportAllocation(STDOUT_FILENO) != 0) {
        printf("('%s' leaks memory by %d (allocs - frees)) ", longBowTestCase_GetName(testCase), parcMemory_Outstanding());
        return LONGBOW_STATUS_TEARDOWN_FAILED;
    }

    parcMemory_SetInterface(savedMemoryModule);

    return LONGBOW_STATUS_SUCCEEDED;
}

LONGBOW_TEST_CASE(Global, athenaPIT_AddInterest)
{
    TestData *data = longBowTestCase_GetClipBoardData(testCase);

    PARCBitVector *expectedReturnVector;
    AthenaPITResolution addResult =
        athenaPIT_AddInterest(data->testPIT, data->testInterest1, data->testVector1, &expectedReturnVector);
    assertTrue(addResult == AthenaPITResolution_Forward, "Expect AddInterest() result to be Forward");
    assertNotNull(expectedReturnVector, "Expected a return vector to be created");

    parcBitVector_Set(expectedReturnVector, 1);
    PARCBitVector *savedReturnVector = expectedReturnVector;
    addResult =
        athenaPIT_AddInterest(data->testPIT, data->testInterest1WithKeyId, data->testVector1, &expectedReturnVector);
    assertTrue(addResult == AthenaPITResolution_Forward, "Expect AddInterest() result to be Forward");
    assertFalse(parcBitVector_Equals(expectedReturnVector, savedReturnVector), "Expect a different return vector");

    parcBitVector_Set(expectedReturnVector, 3);
    addResult =
        athenaPIT_AddInterest(data->testPIT, data->testInterest1WithContentId, data->testVector1, &expectedReturnVector);
    assertTrue(addResult == AthenaPITResolution_Forward, "Expect AddInterest() result to be Forward");
    assertFalse(parcBitVector_Equals(expectedReturnVector, savedReturnVector), "Expect a different return vector");

    // Aggregation of testInterest1
    parcBitVector_Set(expectedReturnVector, 5);
    addResult =
        athenaPIT_AddInterest(data->testPIT, data->testInterest1, data->testVector2, &expectedReturnVector);
    assertTrue(addResult == AthenaPITResolution_Aggregated, "Expect AddInterest() result to be Aggregated");
    assertTrue(parcBitVector_Equals(expectedReturnVector, savedReturnVector), "Expect an existing return vector");

    // Duplicate of testInterest1
    addResult =
        athenaPIT_AddInterest(data->testPIT, data->testInterest1, data->testVector1, &expectedReturnVector);
    assertTrue(addResult == AthenaPITResolution_Forward, "Expect AddInterest() result to be Forward");
    assertTrue(parcBitVector_Equals(expectedReturnVector, savedReturnVector), "Expect an existing return vectors");
}

LONGBOW_TEST_CASE(Global, athenaPIT_RemoveInterest)
{
    TestData *data = longBowTestCase_GetClipBoardData(testCase);

    PARCBitVector *expectedReturnVector;
    AthenaPITResolution addResult =
        athenaPIT_AddInterest(data->testPIT, data->testInterest1, data->testVector1, &expectedReturnVector);
    assertNotNull(expectedReturnVector, "Expected a return vector to be created");
    size_t interestCount = athenaPIT_GetNumberOfPendingInterests(data->testPIT);
    assertTrue(interestCount == 1, "Expect there to be 1 interest");

    // Remove a non-existant interest
    bool result = athenaPIT_RemoveInterest(data->testPIT, data->testInterest2, data->testVector1);
    assertFalse(result, "Expect RemoveInterest() of non-existing interest to fail");
    interestCount = athenaPIT_GetNumberOfPendingInterests(data->testPIT);
    assertTrue(interestCount == 1, "Expect there to be 1 interest");

    result = athenaPIT_RemoveInterest(data->testPIT, data->testInterest1, data->testVector1);
    assertTrue(result, "Expect RemoveInterest() of existing interest to succeed");
    interestCount = athenaPIT_GetNumberOfPendingInterests(data->testPIT);
    assertTrue(interestCount == 0, "Expect there to be 0 interest");

    // Remove Aggregated interest
    parcBitVector_Set(expectedReturnVector, 5);
    addResult =
        athenaPIT_AddInterest(data->testPIT, data->testInterest1, data->testVector1, &expectedReturnVector);

    addResult =
        athenaPIT_AddInterest(data->testPIT, data->testInterest1, data->testVector2, &expectedReturnVector);
    interestCount = athenaPIT_GetNumberOfPendingInterests(data->testPIT);
    assertTrue(interestCount == 2, "Expect there to be 2 interest");

    result = athenaPIT_RemoveInterest(data->testPIT, data->testInterest1, data->testVector12);
    assertTrue(result, "Expect RemoveInterest() of existing interest to succeed");
    interestCount = athenaPIT_GetNumberOfPendingInterests(data->testPIT);
    assertTrue(interestCount == 0, "Expect there to be 0 interest");

    addResult =
        athenaPIT_AddInterest(data->testPIT, data->testInterest1, data->testVector1, &expectedReturnVector);

    addResult =
        athenaPIT_AddInterest(data->testPIT, data->testInterest1, data->testVector2, &expectedReturnVector);
    interestCount = athenaPIT_GetNumberOfPendingInterests(data->testPIT);
    assertTrue(interestCount == 2, "Expect there to be 2 interest");

    result = athenaPIT_RemoveInterest(data->testPIT, data->testInterest1, data->testVector2);
    assertTrue(result, "Expect RemoveInterest() of existing interest to succeed");
    interestCount = athenaPIT_GetNumberOfPendingInterests(data->testPIT);
    assertTrue(interestCount == 1, "Expect there to be 1 interest");

    result = athenaPIT_RemoveInterest(data->testPIT, data->testInterest1, data->testVector2);
    assertFalse(result, "Expect RemoveInterest() of non-existing interest to fail");
    interestCount = athenaPIT_GetNumberOfPendingInterests(data->testPIT);
    assertTrue(interestCount == 1, "Expect there to be 1 interest");

    result = athenaPIT_RemoveInterest(data->testPIT, data->testInterest1, data->testVector1);
    assertTrue(result, "Expect RemoveInterest() of existing interest to succeed");
    interestCount = athenaPIT_GetNumberOfPendingInterests(data->testPIT);
    assertTrue(interestCount == 0, "Expect there to be 0 interest");
}

LONGBOW_TEST_CASE(Global, athenaPIT_Match_NoRestriction)
{
    TestData *data = longBowTestCase_GetClipBoardData(testCase);

    PARCBitVector *expectedReturnVector;
    AthenaPITResolution addResult =
        athenaPIT_AddInterest(data->testPIT, data->testInterest1, data->testVector1, &expectedReturnVector);
    assertTrue(addResult == AthenaPITResolution_Forward, "Expect AddInterest() result to be Forward");

    parcBitVector_Set(expectedReturnVector, 1);
    PARCBitVector *savedReturnVector = expectedReturnVector;

    CCNxContentObject *object2 = data->testContent2;
    CCNxName *name2 = ccnxContentObject_GetName(object2);
    PARCBuffer *keyId2 = ccnxContentObject_GetKeyId(object2);
    PARCBuffer *contentId2 = _createMessageHash(object2);
    PARCBitVector *backLinkVector = athenaPIT_Match(data->testPIT, name2, keyId2, contentId2, savedReturnVector);
    parcBuffer_Release(&contentId2);
    assertTrue(parcBitVector_NextBitSet(backLinkVector, 0), "Expect to find match to forward to");
    parcBitVector_Release(&backLinkVector);

    CCNxContentObject *object1 = data->testContent1;
    CCNxName *name1 = ccnxContentObject_GetName(object1);
    PARCBuffer *keyId1 = ccnxContentObject_GetKeyId(object1);
    PARCBuffer *contentId1 = _createMessageHash(object1);
    backLinkVector = athenaPIT_Match(data->testPIT, name1, keyId1, contentId1, savedReturnVector);
    assertTrue(parcBitVector_Equals(backLinkVector, data->testVector1), "Expect to find match to forward to");
    parcBitVector_Release(&backLinkVector);

    //Aggregation of testInterest1
    assertTrue(athenaPIT_GetNumberOfTableEntries(data->testPIT) == 0, "There should be 0 PIT entries at this point");
    addResult =
        athenaPIT_AddInterest(data->testPIT, data->testInterest1, data->testVector1, &expectedReturnVector);
    addResult =
        athenaPIT_AddInterest(data->testPIT, data->testInterest1, data->testVector2, &expectedReturnVector);
    parcBitVector_Set(expectedReturnVector, 5);
    assertTrue(athenaPIT_GetNumberOfTableEntries(data->testPIT) == 1, "There should be 1 PIT table entries at this point");
    assertTrue(athenaPIT_GetNumberOfPendingInterests(data->testPIT) == 2, "There should be 2 Pending Interests at this point");

    backLinkVector = athenaPIT_Match(data->testPIT, name1, keyId1, contentId1, savedReturnVector);
    assertTrue(parcBitVector_Equals(backLinkVector, data->testVector12), "Expect to find match to forward to");

    parcBuffer_Release(&contentId1);
    parcBitVector_Release(&backLinkVector);
}

LONGBOW_TEST_CASE(Global, athenaPIT_Match_KeyIdRestriction)
{
    TestData *data = longBowTestCase_GetClipBoardData(testCase);

    PARCBitVector *expectedReturnVector;

    AthenaPITResolution addResult =
        athenaPIT_AddInterest(data->testPIT, data->testInterest1WithKeyId, data->testVector1, &expectedReturnVector);
    assertTrue(addResult == AthenaPITResolution_Forward, "Expect AddInterest() result to be Forward");

    parcBitVector_Set(expectedReturnVector, 1);

    PARCBitVector *savedReturnVector = expectedReturnVector;

    CCNxContentObject *object1 = data->testContent1;
    CCNxName *name1 = ccnxContentObject_GetName(object1);
    PARCBuffer *keyId1 = ccnxContentObject_GetKeyId(object1);
    PARCBuffer *contentId1 = _createMessageHash(object1);
    PARCBitVector *backLinkVector = athenaPIT_Match(data->testPIT, name1, keyId1, contentId1, savedReturnVector);
    parcBuffer_Release(&contentId1);
    assertTrue(parcBitVector_NumberOfBitsSet(backLinkVector) == 0, "Expect to find no match in PIT");
    parcBitVector_Release(&backLinkVector);

    CCNxContentObject *object1Prime = data->testContent1Prime;
    CCNxName *name1Prime = ccnxContentObject_GetName(object1Prime);
    PARCBuffer *keyId1Prime = ccnxContentObject_GetKeyId(object1Prime);
    PARCBuffer *contentId1Prime = _createMessageHash(object1Prime);
    backLinkVector = athenaPIT_Match(data->testPIT, name1Prime, keyId1Prime, contentId1Prime, savedReturnVector);
    parcBuffer_Release(&contentId1Prime);
    assertTrue(parcBitVector_NumberOfBitsSet(backLinkVector) == 0, "Expect to find no match in PIT");
    parcBitVector_Release(&backLinkVector);

    CCNxContentObject *object1WithSig = data->testContent1WithSig;
    CCNxName *name1WithSig = ccnxContentObject_GetName(object1WithSig);
    PARCBuffer *keyId1WithSig = ccnxContentObject_GetKeyId(object1WithSig);
    PARCBuffer *contentId1WithSig = _createMessageHash(object1WithSig);
    backLinkVector = athenaPIT_Match(data->testPIT, name1WithSig, keyId1WithSig, contentId1WithSig, savedReturnVector);
    parcBuffer_Release(&contentId1WithSig);
    assertTrue(parcBitVector_Equals(backLinkVector, data->testVector1), "Expect to find match to forward to");
    parcBitVector_Release(&backLinkVector);
}

LONGBOW_TEST_CASE(Global, athenaPIT_Match_ContentHashRestriction)
{
    TestData *data = longBowTestCase_GetClipBoardData(testCase);

    PARCBitVector *expectedReturnVector;

    // Match with ContentId
    AthenaPITResolution addResult =
        athenaPIT_AddInterest(data->testPIT, data->testInterest1WithContentId, data->testVector1, &expectedReturnVector);
    assertTrue(addResult == AthenaPITResolution_Forward, "Expect AddInterest() result to be Forward");

    parcBitVector_Set(expectedReturnVector, 1);

    PARCBitVector *savedReturnVector = expectedReturnVector;

    CCNxContentObject *object1Prime = data->testContent1Prime;
    CCNxName *name1Prime = ccnxContentObject_GetName(object1Prime);
    PARCBuffer *keyId1Prime = ccnxContentObject_GetKeyId(object1Prime);
    PARCBuffer *contentId1Prime = _createMessageHash(object1Prime);
    PARCBitVector *backLinkVector = athenaPIT_Match(data->testPIT, name1Prime, keyId1Prime, contentId1Prime, savedReturnVector);
    parcBuffer_Release(&contentId1Prime);
    assertTrue(parcBitVector_NumberOfBitsSet(backLinkVector) == 0, "Expect to find no match in PIT");
    parcBitVector_Release(&backLinkVector);

    CCNxContentObject *object1WithSig = data->testContent1WithSig;
    CCNxName *name1WithSig = ccnxContentObject_GetName(object1WithSig);
    PARCBuffer *keyId1WithSig = ccnxContentObject_GetKeyId(object1WithSig);
    PARCBuffer *contentId1WithSig = _createMessageHash(object1WithSig);
    backLinkVector = athenaPIT_Match(data->testPIT, name1WithSig, keyId1WithSig, contentId1WithSig, savedReturnVector);
    parcBuffer_Release(&contentId1WithSig);
    assertTrue(parcBitVector_Equals(backLinkVector, data->testVector1), "Expect to find match to forward to");
    parcBitVector_Release(&backLinkVector);
}

LONGBOW_TEST_CASE(Global, athenaPIT_Match_Nameless)
{
    TestData *data = longBowTestCase_GetClipBoardData(testCase);

    PARCBitVector *expectedReturnVector;

    // Match with ContentId
    AthenaPITResolution addResult =
            athenaPIT_AddInterest(data->testPIT, data->testNamelessInterest, data->testVector1, &expectedReturnVector);
    assertTrue(addResult == AthenaPITResolution_Forward, "Expect AddInterest() result to be Forward");

    parcBitVector_Set(expectedReturnVector, 1);

    PARCBitVector *savedReturnVector = expectedReturnVector;

    CCNxName *namePrime = ccnxContentObject_GetName(data->testContent1Prime);
    PARCBuffer *keyIdPrime = ccnxContentObject_GetKeyId(data->testContent1Prime);
    PARCBuffer *contentIdPrime = _createMessageHash(data->testContent1Prime);

    PARCBitVector *backLinkVector = athenaPIT_Match(data->testPIT, namePrime, keyIdPrime, contentIdPrime, savedReturnVector);
    assertTrue(parcBitVector_NumberOfBitsSet(backLinkVector) == 0, "Expect to find no match in PIT");
    parcBitVector_Release(&backLinkVector);
    parcBuffer_Release(&contentIdPrime);

    CCNxName *nameNameless = ccnxContentObject_GetName(data->testNamelessContent);
    PARCBuffer *keyIdNameless = ccnxContentObject_GetKeyId(data->testNamelessContent);
    PARCBuffer *contentIdNameless = _createMessageHash(data->testNamelessContent);

    backLinkVector = athenaPIT_Match(data->testPIT, nameNameless, keyIdNameless, contentIdNameless, savedReturnVector);
    assertTrue(parcBitVector_Equals(backLinkVector, data->testVector1), "Expect to find match to forward to");
    parcBitVector_Release(&backLinkVector);
    parcBuffer_Release(&contentIdNameless);
}

LONGBOW_TEST_CASE(Global, athenaPIT_Match_MultipleRestrictions)
{
    TestData *data = longBowTestCase_GetClipBoardData(testCase);

    PARCBitVector *expectedReturnVector;

    // Interest for Content w/ KeyId restriction
    AthenaPITResolution addResult =
        athenaPIT_AddInterest(data->testPIT, data->testInterest1, data->testVector1, &expectedReturnVector);
    assertTrue(addResult == AthenaPITResolution_Forward, "Expect AddInterest() result to be Forward");

    // Interest for Content w/ KeyId restriction
    addResult =
        athenaPIT_AddInterest(data->testPIT, data->testInterest1WithKeyId, data->testVector2, &expectedReturnVector);
    assertTrue(addResult == AthenaPITResolution_Forward, "Expect AddInterest() result to be Forward");

    // Interest for Content w/ ContentId restriction
    addResult =
        athenaPIT_AddInterest(data->testPIT, data->testInterest1WithContentId, data->testVector3, &expectedReturnVector);
    assertTrue(addResult == AthenaPITResolution_Forward, "Expect AddInterest() result to be Forward");

    parcBitVector_Set(expectedReturnVector, 1);

    PARCBitVector *savedReturnVector = expectedReturnVector;

    CCNxContentObject *object1WithSig = data->testContent1WithSig;
    CCNxName *name1WithSig = ccnxContentObject_GetName(object1WithSig);
    PARCBuffer *keyId1WithSig = ccnxContentObject_GetKeyId(object1WithSig);
    PARCBuffer *contentId1WithSig = _createMessageHash(object1WithSig);
    PARCBitVector *backLinkVector = athenaPIT_Match(data->testPIT, name1WithSig, keyId1WithSig, contentId1WithSig, savedReturnVector);
    parcBuffer_Release(&contentId1WithSig);
    assertTrue(parcBitVector_NumberOfBitsSet(backLinkVector) == 3, "Expect to find 3 PIT matches");
    assertTrue(parcBitVector_Equals(backLinkVector, data->testVector123), "Expect to find match to forward to");
    parcBitVector_Release(&backLinkVector);
}

LONGBOW_TEST_CASE(Global, athenaPIT_CreateCapacity)
{
    TestData *data = longBowTestCase_GetClipBoardData(testCase);

    AthenaPIT *limitedPIT = athenaPIT_CreateCapacity(1);

    limitedPIT->clock = parcClock_Test();

    PARCBitVector *expectedReturnVector;
    AthenaPITResolution addResult =
        athenaPIT_AddInterest(limitedPIT, data->testInterest1, data->testVector1, &expectedReturnVector);
    assertTrue(addResult == AthenaPITResolution_Forward, "Expected forward result");

    addResult =
        athenaPIT_AddInterest(limitedPIT, data->testInterest1, data->testVector1, &expectedReturnVector);
    assertTrue(addResult == AthenaPITResolution_Forward, "Expected forward result");

    addResult =
        athenaPIT_AddInterest(limitedPIT, data->testInterest1, data->testVector2, &expectedReturnVector);
    assertTrue(addResult == AthenaPITResolution_Aggregated, "Expected aggregated result");

    addResult =
        athenaPIT_AddInterest(limitedPIT, data->testInterest1WithKeyId, data->testVector2, &expectedReturnVector);
    assertTrue(addResult == AthenaPITResolution_Error, "Expected error result");

    athenaPIT_Release(&limitedPIT);
}

LONGBOW_TEST_CASE(Global, athenaPIT_PurgeExpired)
{
    TestData *data = longBowTestCase_GetClipBoardData(testCase);

    AthenaPIT *limitedPIT = athenaPIT_CreateCapacity(1);

    limitedPIT->clock = parcClock_Test();

    PARCBitVector *expectedReturnVector;
    AthenaPITResolution addResult =
        athenaPIT_AddInterest(limitedPIT, data->testInterest1, data->testVector1, &expectedReturnVector);
    assertTrue(addResult == AthenaPITResolution_Forward, "Expected forward result");
    assertTrue(athenaPIT_GetNumberOfTableEntries(limitedPIT) == 1, "Expect a single PIT entry");
    assertTrue(athenaPIT_GetNumberOfPendingInterests(limitedPIT) == 1, "Expect 1 pending interests");

    _TestClockTimeval.tv_usec += 50 * 1000;

    addResult =
        athenaPIT_AddInterest(limitedPIT, data->testInterest1, data->testVector1, &expectedReturnVector);
    assertTrue(addResult == AthenaPITResolution_Forward, "Expected forward result");
    assertTrue(athenaPIT_GetNumberOfTableEntries(limitedPIT) == 1, "Expect a single PIT entry");
    assertTrue(athenaPIT_GetNumberOfPendingInterests(limitedPIT) == 1, "Expect 1 pending interests");

    addResult =
        athenaPIT_AddInterest(limitedPIT, data->testInterest1, data->testVector2, &expectedReturnVector);
    assertTrue(addResult == AthenaPITResolution_Aggregated, "Expected aggregate result");
    assertTrue(athenaPIT_GetNumberOfTableEntries(limitedPIT) == 1, "Expect a single PIT entry");
    assertTrue(athenaPIT_GetNumberOfPendingInterests(limitedPIT) == 2, "Expect 2 pending interests");

    // Expire the 1st Aggrigation but not the second
    _TestClockTimeval.tv_usec += 50 * 1000;

    addResult =
        athenaPIT_AddInterest(limitedPIT, data->testInterest2, data->testVector2, &expectedReturnVector);
    assertTrue(addResult == AthenaPITResolution_Error, "Expected error result");

    // Expire the pit entry
    _TestClockTimeval.tv_usec += 1000 * 1000;

    addResult =
        athenaPIT_AddInterest(limitedPIT, data->testInterest1WithKeyId, data->testVector2, &expectedReturnVector);
    assertTrue(addResult == AthenaPITResolution_Forward, "Expected forward result");

    athenaPIT_Release(&limitedPIT);
}

LONGBOW_TEST_CASE(Global, athenaPIT_RemoveLink)
{
    TestData *data = longBowTestCase_GetClipBoardData(testCase);

    // Test 0 - Nothing to remove.
    assertTrue(athenaPIT_RemoveLink(data->testPIT, data->testVector1), "Expected True result from RemoveLink()");

    // Test 1 - remove link with 3 different interest on the removed link
    PARCBitVector *expectedReturnVector;
    AthenaPITResolution addResult =
        athenaPIT_AddInterest(data->testPIT, data->testInterest1, data->testVector2, &expectedReturnVector);
    assertTrue(athenaPIT_RemoveLink(data->testPIT, data->testVector1), "Expected True result from RemoveLink()");
    assertTrue(athenaPIT_RemoveLink(data->testPIT, data->testVector2), "Expected True result from RemoveLink()");

    // Test 2 - remove link with 3 different interest on the removed link
    addResult =
        athenaPIT_AddInterest(data->testPIT, data->testInterest1, data->testVector1, &expectedReturnVector);
    parcBitVector_Set(expectedReturnVector, 1);
    PARCBitVector *savedReturnVector = expectedReturnVector;
    addResult =
        athenaPIT_AddInterest(data->testPIT, data->testInterest1WithKeyId, data->testVector1, &expectedReturnVector);
    parcBitVector_Set(expectedReturnVector, 3);
    addResult =
        athenaPIT_AddInterest(data->testPIT, data->testInterest1WithContentId, data->testVector1, &expectedReturnVector);

    // Should be no pending interests after remove
    assertTrue(athenaPIT_RemoveLink(data->testPIT, data->testVector1), "Expected True result from RemoveLink()");

    CCNxContentObject *object1 = data->testContent1;
    CCNxName *name1 = ccnxContentObject_GetName(object1);
    PARCBuffer *keyId1 = ccnxContentObject_GetKeyId(object1);
    PARCBuffer *contentId1 = _createMessageHash(object1);
    PARCBitVector *backLinkVector =
        athenaPIT_Match(data->testPIT, name1, keyId1, contentId1, savedReturnVector);
    parcBuffer_Release(&contentId1);
    assertTrue((int) parcBitVector_NextBitSet(backLinkVector, 0) == -1, "Expect an empty back link vector");
    parcBitVector_Release(&backLinkVector);

    // Test 3 - remove link with two different interest on the removed link and one on a different link
    addResult =
        athenaPIT_AddInterest(data->testPIT, data->testInterest1, data->testVector1, &expectedReturnVector);
    parcBitVector_Set(expectedReturnVector, 1);
    savedReturnVector = expectedReturnVector;
    addResult =
        athenaPIT_AddInterest(data->testPIT, data->testInterest1WithKeyId, data->testVector2, &expectedReturnVector);
    parcBitVector_Set(expectedReturnVector, 3);
    addResult =
        athenaPIT_AddInterest(data->testPIT, data->testInterest1WithContentId, data->testVector2, &expectedReturnVector);

    // Should be one pending interests after remove
    assertTrue(athenaPIT_RemoveLink(data->testPIT, data->testVector2), "Expected True result from RemoveLink()");
    backLinkVector =
        athenaPIT_Match(data->testPIT, name1, keyId1, contentId1, savedReturnVector);
    assertTrue(parcBitVector_Equals(backLinkVector, data->testVector1), "Expect back link vector to equal vector2 ");
    parcBitVector_Release(&backLinkVector);

    // Test 4 - remove link with the same interest on two different links
    addResult =
        athenaPIT_AddInterest(data->testPIT, data->testInterest1, data->testVector1, &expectedReturnVector);

    parcBitVector_Set(expectedReturnVector, 1);
    savedReturnVector = expectedReturnVector;
    addResult =
        athenaPIT_AddInterest(data->testPIT, data->testInterest1, data->testVector2, &expectedReturnVector);

    // Should be one pending interests after remove
    assertTrue(athenaPIT_RemoveLink(data->testPIT, data->testVector1), "Expected True result from RemoveLink()");
    backLinkVector =
        athenaPIT_Match(data->testPIT, name1, keyId1, contentId1, savedReturnVector);
    assertTrue(parcBitVector_Equals(backLinkVector, data->testVector2), "Expect back link vector to equal vector2 ");
    parcBitVector_Release(&backLinkVector);
}

LONGBOW_TEST_CASE(Global, athenaPIT_GetNumberOfTableEntries)
{
    TestData *data = longBowTestCase_GetClipBoardData(testCase);

    size_t tableEntries = athenaPIT_GetNumberOfTableEntries(data->testPIT);
    assertTrue(tableEntries == 0, "Expect 0 table entry at this point");


    PARCBitVector *expectedReturnVector;
    AthenaPITResolution addResult =
        athenaPIT_AddInterest(data->testPIT, data->testInterest1, data->testVector1, &expectedReturnVector);
    assertTrue(addResult == AthenaPITResolution_Forward, "Expect AddInterest() result to be Forward");

    tableEntries = athenaPIT_GetNumberOfTableEntries(data->testPIT);
    assertTrue(tableEntries == 1, "Expect 1 table entry at this point");

    parcBitVector_Set(expectedReturnVector, 1);
    addResult =
        athenaPIT_AddInterest(data->testPIT, data->testInterest1WithKeyId, data->testVector1, &expectedReturnVector);
    assertTrue(addResult == AthenaPITResolution_Forward, "Expect AddInterest() result to be Forward");

    tableEntries = athenaPIT_GetNumberOfTableEntries(data->testPIT);
    assertTrue(tableEntries == 2, "Expect 2 table entry at this point");

    parcBitVector_Set(expectedReturnVector, 3);
    addResult =
        athenaPIT_AddInterest(data->testPIT, data->testInterest1WithContentId, data->testVector1, &expectedReturnVector);
    assertTrue(addResult == AthenaPITResolution_Forward, "Expect AddInterest() result to be Forward");

    tableEntries = athenaPIT_GetNumberOfTableEntries(data->testPIT);
    assertTrue(tableEntries == 4, "Expect 4 table entry at this point");

    // Aggregation of testInterest1
    parcBitVector_Set(expectedReturnVector, 5);
    addResult =
        athenaPIT_AddInterest(data->testPIT, data->testInterest1, data->testVector2, &expectedReturnVector);
    assertTrue(addResult == AthenaPITResolution_Aggregated, "Expect AddInterest() result to be Aggregated");

    tableEntries = athenaPIT_GetNumberOfTableEntries(data->testPIT);
    assertTrue(tableEntries == 4, "Expect 4 table entry at this point");

    // Duplicate of testInterest1
    addResult =
        athenaPIT_AddInterest(data->testPIT, data->testInterest1, data->testVector1, &expectedReturnVector);
    assertTrue(addResult == AthenaPITResolution_Forward, "Expect AddInterest() result to be Forward");

    tableEntries = athenaPIT_GetNumberOfTableEntries(data->testPIT);
    assertTrue(tableEntries == 4, "Expect 4 table entry at this point");
}

LONGBOW_TEST_CASE(Global, athenaPIT_GetNumberOfPendingInterests)
{
    TestData *data = longBowTestCase_GetClipBoardData(testCase);

    size_t pendingInterests = athenaPIT_GetNumberOfPendingInterests(data->testPIT);
    assertTrue(pendingInterests == 0, "Expect 0 table entry at this point");

    PARCBitVector *expectedReturnVector;
    AthenaPITResolution addResult =
        athenaPIT_AddInterest(data->testPIT, data->testInterest1, data->testVector1, &expectedReturnVector);
    assertTrue(addResult == AthenaPITResolution_Forward, "Expect AddInterest() result to be Forward");

    pendingInterests = athenaPIT_GetNumberOfPendingInterests(data->testPIT);
    assertTrue(pendingInterests == 1, "Expect 1 table entry at this point");

    parcBitVector_Set(expectedReturnVector, 1);
    addResult =
        athenaPIT_AddInterest(data->testPIT, data->testInterest1WithKeyId, data->testVector1, &expectedReturnVector);
    assertTrue(addResult == AthenaPITResolution_Forward, "Expect AddInterest() result to be Forward");

    pendingInterests = athenaPIT_GetNumberOfPendingInterests(data->testPIT);
    assertTrue(pendingInterests == 2, "Expect 2 table entry at this point");

    parcBitVector_Set(expectedReturnVector, 3);
    addResult =
        athenaPIT_AddInterest(data->testPIT, data->testInterest1WithContentId, data->testVector1, &expectedReturnVector);
    assertTrue(addResult == AthenaPITResolution_Forward, "Expect AddInterest() result to be Forward");

    pendingInterests = athenaPIT_GetNumberOfPendingInterests(data->testPIT);
    assertTrue(pendingInterests == 3, "Expect 3 table entry at this point");

    // Aggregation of testInterest1
    parcBitVector_Set(expectedReturnVector, 5);
    addResult =
        athenaPIT_AddInterest(data->testPIT, data->testInterest1, data->testVector2, &expectedReturnVector);
    assertTrue(addResult == AthenaPITResolution_Aggregated, "Expect AddInterest() result to be Aggregated");

    pendingInterests = athenaPIT_GetNumberOfPendingInterests(data->testPIT);
    assertTrue(pendingInterests == 4, "Expect 4 table entry at this point");

    // Duplicate of testInterest1
    addResult =
        athenaPIT_AddInterest(data->testPIT, data->testInterest1, data->testVector1, &expectedReturnVector);
    assertTrue(addResult == AthenaPITResolution_Forward, "Expect AddInterest() result to be Forward");

    pendingInterests = athenaPIT_GetNumberOfPendingInterests(data->testPIT);
    assertTrue(pendingInterests == 4, "Expect 4 table entry at this point");
}

LONGBOW_TEST_CASE(Global, athenaPIT_GetMeanEntryLifetime)
{
    TestData *data = longBowTestCase_GetClipBoardData(testCase);

    data->testPIT->clock = parcClock_Test();

    time_t meanLifetime = athenaPIT_GetMeanEntryLifetime(data->testPIT);
    assertTrue(meanLifetime == 0, "Expect 0 for mean lifetime");

    _TestClockTimeval.tv_usec = 0;

    PARCBitVector *expectedReturnVector;
    AthenaPITResolution addResult =
        athenaPIT_AddInterest(data->testPIT, data->testInterest1, data->testVector1, &expectedReturnVector);

    // Test lifetime
    _TestClockTimeval.tv_usec += 50 * 1000;

    parcBitVector_Set(expectedReturnVector, 1);
    PARCBitVector *savedReturnVector = expectedReturnVector;
    CCNxContentObject *object1 = data->testContent1;
    CCNxName *name1 = ccnxContentObject_GetName(object1);
    PARCBuffer *keyId1 = ccnxContentObject_GetKeyId(object1);
    PARCBuffer *contentId1 = _createMessageHash(object1);
    PARCBitVector *backLinkVector = athenaPIT_Match(data->testPIT, name1, keyId1, contentId1, savedReturnVector);
    parcBitVector_Release(&backLinkVector);

    meanLifetime = athenaPIT_GetMeanEntryLifetime(data->testPIT);
    assertTrue(meanLifetime == 50, "Expect mean lifetime == 50ms, was %d", (int) meanLifetime);

    addResult =
        athenaPIT_AddInterest(data->testPIT, data->testInterest1, data->testVector1, &expectedReturnVector);

    // Test interest lifetime
    _TestClockTimeval.tv_usec += 100 * 1000;

    parcBitVector_Set(expectedReturnVector, 1);
    backLinkVector = athenaPIT_Match(data->testPIT, name1, keyId1, contentId1, savedReturnVector);
    parcBitVector_Release(&backLinkVector);

    meanLifetime = athenaPIT_GetMeanEntryLifetime(data->testPIT);
    assertTrue(meanLifetime == 75, "Expect mean lifetime == 75ms, was %d", (int) meanLifetime);

    for (size_t i = 0; i < 150; ++i) {
        addResult =
            athenaPIT_AddInterest(data->testPIT, data->testInterest1, data->testVector1, &expectedReturnVector);
        _TestClockTimeval.tv_usec += 1000;
        backLinkVector = athenaPIT_Match(data->testPIT, name1, keyId1, contentId1, data->testVector1);
        parcBitVector_Release(&backLinkVector);
    }
    addResult =
        athenaPIT_AddInterest(data->testPIT, data->testInterest1, data->testVector1, &expectedReturnVector);
    _TestClockTimeval.tv_usec += 201 * 1000;
    backLinkVector = athenaPIT_Match(data->testPIT, name1, keyId1, contentId1, data->testVector1);
    parcBitVector_Release(&backLinkVector);

    parcBuffer_Release(&contentId1);

    meanLifetime = athenaPIT_GetMeanEntryLifetime(data->testPIT);
    assertTrue(meanLifetime == 3, "Expect mean lifetime == 3ms, was %d", (int) meanLifetime);
}

LONGBOW_TEST_CASE(Global, athenaPIT_LinkCleanupFromMatch)
{
    TestData *data = longBowTestCase_GetClipBoardData(testCase);

    PARCBitVector *expectedReturnVector;
    AthenaPITResolution addResult =
        athenaPIT_AddInterest(data->testPIT, data->testInterest1, data->testVector1, &expectedReturnVector);
    assertTrue(addResult == AthenaPITResolution_Forward, "Expect AddInterest() result to be Forward");

    parcBitVector_Set(expectedReturnVector, 1);
    PARCBitVector *savedReturnVector = expectedReturnVector;
    CCNxContentObject *object1 = data->testContent1;
    CCNxName *name1 = ccnxContentObject_GetName(object1);
    PARCBuffer *keyId1 = ccnxContentObject_GetKeyId(object1);
    PARCBuffer *contentId1 = _createMessageHash(object1);
    PARCBitVector *backLinkVector = athenaPIT_Match(data->testPIT, name1, keyId1, contentId1, savedReturnVector);
    parcBuffer_Release(&contentId1);
    parcBitVector_Release(&backLinkVector);

    assertTrue(athenaPIT_RemoveLink(data->testPIT, data->testVector1), "Expected True result from RemoveLink()");
}

LONGBOW_TEST_CASE(Global, athenaPIT_ProcessMessage_Size)
{
    TestData *data = longBowTestCase_GetClipBoardData(testCase);

    CCNxName *name = ccnxName_CreateFromCString(CCNxNameAthena_PIT "/stat/size");
    CCNxInterest *interest = ccnxInterest_CreateSimple(name);
    ccnxName_Release(&name);

    CCNxMetaMessage *message = ccnxMetaMessage_CreateFromInterest(interest);
    ccnxInterest_Release(&interest);

    CCNxMetaMessage *response = athenaPIT_ProcessMessage(data->testPIT, message);

    assertNotNull(response, "Expected a response to ProcessMessage()");
    assertTrue(ccnxMetaMessage_IsContentObject(response), "Expected a content object");

    CCNxContentObject *content = ccnxMetaMessage_GetContentObject(response);

    PARCBuffer *payload = ccnxContentObject_GetPayload(content);
    assertNotNull(payload, "Expecting non-NULL payload");

    ccnxMetaMessage_Release(&message);
    ccnxMetaMessage_Release(&response);
}

LONGBOW_TEST_CASE(Global, athenaPIT_ProcessMessage_AvgEntryLifetime)
{
    TestData *data = longBowTestCase_GetClipBoardData(testCase);

    CCNxName *name = ccnxName_CreateFromCString(CCNxNameAthena_PIT "/stat/avgEntryLifetime");
    CCNxInterest *interest = ccnxInterest_CreateSimple(name);
    ccnxName_Release(&name);

    CCNxMetaMessage *message = ccnxMetaMessage_CreateFromInterest(interest);
    ccnxInterest_Release(&interest);

    CCNxMetaMessage *response = athenaPIT_ProcessMessage(data->testPIT, message);

    assertNotNull(response, "Expected a response to ProcessMessage()");
    assertTrue(ccnxMetaMessage_IsContentObject(response), "Expected a content object");

    CCNxContentObject *content = ccnxMetaMessage_GetContentObject(response);

    PARCBuffer *payload = ccnxContentObject_GetPayload(content);
    assertNotNull(payload, "Expecting non-NULL payload");

    ccnxMetaMessage_Release(&message);
    ccnxMetaMessage_Release(&response);
}

LONGBOW_TEST_CASE(Global, athenaPIT_CreateEntryList)
{
    TestData *data = longBowTestCase_GetClipBoardData(testCase);

    PARCBitVector *expectedReturnVector;

    AthenaPITResolution addResult =
            athenaPIT_AddInterest(data->testPIT, data->testInterest1, data->testVector1, &expectedReturnVector);
    parcBitVector_Set(expectedReturnVector, 1);
    assertTrue(addResult == AthenaPITResolution_Forward, "Expect AddInterest() result to be Forward");

    addResult =
            athenaPIT_AddInterest(data->testPIT, data->testInterest1WithKeyId, data->testVector1, &expectedReturnVector);
    parcBitVector_Set(expectedReturnVector, 3);
    assertTrue(addResult == AthenaPITResolution_Forward, "Expect AddInterest() result to be Forward");

    addResult =
            athenaPIT_AddInterest(data->testPIT, data->testInterest1WithContentId, data->testVector1, &expectedReturnVector);
    parcBitVector_Set(expectedReturnVector, 5);
    assertTrue(addResult == AthenaPITResolution_Forward, "Expect AddInterest() result to be Forward");

    // Aggregation of testInterest1
    addResult =
            athenaPIT_AddInterest(data->testPIT, data->testInterest1, data->testVector2, &expectedReturnVector);
    assertTrue(addResult == AthenaPITResolution_Aggregated, "Expect AddInterest() result to be Aggregated");

    // Duplicate of testInterest1
    addResult =
            athenaPIT_AddInterest(data->testPIT, data->testInterest1, data->testVector1, &expectedReturnVector);
    assertTrue(addResult == AthenaPITResolution_Forward, "Expect AddInterest() result to be Forward");

    PARCList *entryList = athenaPIT_CreateEntryList(data->testPIT);
    printf("\n");
    for (size_t i=0; i<parcList_Size(entryList); ++i) {
        PARCBuffer *strbuf = parcList_GetAtIndex(entryList, i);
        char *toprint = parcBuffer_ToString(strbuf);
        printf("%s\n", toprint);
        parcMemory_Deallocate(&toprint);
    }

    parcList_Release(&entryList);
}

LONGBOW_TEST_FIXTURE(Performance)
{
    LONGBOW_RUN_TEST_CASE(Performance, athenaPIT_AddInterest);
    LONGBOW_RUN_TEST_CASE(Performance, athenaPIT_Match);
    LONGBOW_RUN_TEST_CASE(Performance, athenaPIT_RemoveLink);
    LONGBOW_RUN_TEST_CASE(Performance, athenaPIT_Add_Remove);
}


#define ITERATIONS 2000
#define NUMBER_OF_LINKS 100

typedef struct performance_test_data {
    AthenaPIT *testPIT;
    CCNxInterest *interests[ITERATIONS];
    CCNxContentObject *content[ITERATIONS];
    PARCBitVector *ingressVectors[ITERATIONS];
} PerformanceTestData;


LONGBOW_TEST_FIXTURE_SETUP(Performance)
{
    PerformanceTestData *data = parcMemory_AllocateAndClear(sizeof(PerformanceTestData));
    assertNotNull(data, "parcMemory_AllocateAndClear(%lu) returned NULL", sizeof(TestData));

    data->testPIT = athenaPIT_Create();

    CCNxName *name = NULL;
    PARCBuffer *payload = parcBuffer_WrapCString("Some Payload");
    const char *lciPrefix = "lci:/";
    char uri[30];
    srand((uint32_t) parcClock_GetTime(data->testPIT->clock));
    for (size_t i = 0; i < ITERATIONS; ++i) {
        sprintf(uri, "%s%d", lciPrefix, rand());
        name = ccnxName_CreateFromCString(uri);
        data->interests[i] = _createTestInterest(name, NULL, NULL);
        CCNxContentObject *psObject = ccnxContentObject_CreateWithNameAndPayload(name, payload);
        data->content[i] = _createReceivedContent(psObject);
        ccnxContentObject_Release(&psObject);
        ccnxName_Release(&name);

        data->ingressVectors[i] = parcBitVector_Create();
        parcBitVector_Set(data->ingressVectors[i], i % NUMBER_OF_LINKS);
    }

    longBowTestCase_SetClipBoardData(testCase, data);

    return LONGBOW_STATUS_SUCCEEDED;
}

LONGBOW_TEST_FIXTURE_TEARDOWN(Performance)
{
    PerformanceTestData *data = longBowTestCase_GetClipBoardData(testCase);
    athenaPIT_Release(&data->testPIT);

    for (size_t i = 0; i < ITERATIONS; ++i) {
        ccnxInterest_Release(&data->interests[i]);
        ccnxContentObject_Release(&data->content[i]);
        parcBitVector_Release(&data->ingressVectors[i]);
    }

    parcMemory_Deallocate((void **) &data);

    return LONGBOW_STATUS_SUCCEEDED;
}


LONGBOW_TEST_CASE(Performance, athenaPIT_AddInterest)
{
    PerformanceTestData *data = longBowTestCase_GetClipBoardData(testCase);

    PARCBitVector *expectedReturnVector;

    for (size_t i = 0; i < ITERATIONS; ++i) {
        AthenaPITResolution addResult =
            athenaPIT_AddInterest(data->testPIT, data->interests[i], data->ingressVectors[i], &expectedReturnVector);
        assertTrue(addResult == AthenaPITResolution_Forward, "Expect AddInterest() result to be Forward");
    }
}

LONGBOW_TEST_CASE(Performance, athenaPIT_Match)
{
    PerformanceTestData *data = longBowTestCase_GetClipBoardData(testCase);

    PARCBitVector *expectedReturnVector = NULL;

    for (size_t i = 0; i < ITERATIONS; ++i) {
        AthenaPITResolution addResult =
            athenaPIT_AddInterest(data->testPIT, data->interests[i], data->ingressVectors[i], &expectedReturnVector);
        assertTrue(addResult == AthenaPITResolution_Forward, "Expect AddInterest() result to be Forward");
    }

    for (size_t i = 0; i < ITERATIONS; ++i) {
        CCNxContentObject *object = data->content[i];
        CCNxName *name = ccnxContentObject_GetName(object);
        PARCBuffer *keyId = ccnxContentObject_GetKeyId(object);
        PARCBuffer *contentId = _createMessageHash(object);
        PARCBitVector *backLinkVector = athenaPIT_Match(data->testPIT, name, keyId, contentId, expectedReturnVector);
        parcBuffer_Release(&contentId);
        parcBitVector_Release(&backLinkVector);
    }
}

LONGBOW_TEST_CASE(Performance, athenaPIT_RemoveLink)
{
    PerformanceTestData *data = longBowTestCase_GetClipBoardData(testCase);

    PARCBitVector *expectedReturnVector = NULL;

    for (size_t i = 0; i < ITERATIONS; ++i) {
        AthenaPITResolution addResult =
            athenaPIT_AddInterest(data->testPIT, data->interests[i], data->ingressVectors[i], &expectedReturnVector);
        assertTrue(addResult == AthenaPITResolution_Forward, "Expect AddInterest() result to be Forward");
    }

    PARCBitVector *link = parcBitVector_Create();
    for (uint32_t i = 0; i < NUMBER_OF_LINKS; ++i) {
        parcBitVector_Set(link, i);
        athenaPIT_RemoveLink(data->testPIT, link);
    }
    parcBitVector_Release(&link);
}

LONGBOW_TEST_CASE(Performance, athenaPIT_Add_Remove)
{
    PerformanceTestData *data = longBowTestCase_GetClipBoardData(testCase);

    PARCBitVector *expectedReturnVector = NULL;

    PARCClock *clock = parcClock_Monotonic();

    uint64_t start = parcClock_GetTime(clock);
    for (size_t i = 0; i < ITERATIONS * 2; ++i) {
        AthenaPITResolution addResult =
            athenaPIT_AddInterest(data->testPIT, data->interests[1], data->ingressVectors[1], &expectedReturnVector);
        assertTrue(addResult == AthenaPITResolution_Forward, "Expect AddInterest() result to be Forward");

        CCNxContentObject *object = data->content[1];
        CCNxName *name = ccnxContentObject_GetName(object);
        PARCBuffer *keyId = ccnxContentObject_GetKeyId(object);
        PARCBuffer *contentId = _createMessageHash(object);
        PARCBitVector *backLinkVector = athenaPIT_Match(data->testPIT, name, keyId, contentId, expectedReturnVector);
        parcBitVector_Release(&backLinkVector);
        parcBuffer_Release(&contentId);
    }
    int delta1 = (int) (parcClock_GetTime(clock) - start);

    start = parcClock_GetTime(clock);
    for (size_t i = 0; i < ITERATIONS * 2; ++i) {
        AthenaPITResolution addResult =
            athenaPIT_AddInterest(data->testPIT, data->interests[1], data->ingressVectors[1], &expectedReturnVector);
        assertTrue(addResult == AthenaPITResolution_Forward, "Expect AddInterest() result to be Forward");

        CCNxContentObject *object = data->content[1];
        CCNxName *name = ccnxContentObject_GetName(object);
        PARCBuffer *keyId = ccnxContentObject_GetKeyId(object);
        PARCBuffer *contentId = _createMessageHash(object);
        PARCBitVector *backLinkVector = athenaPIT_Match(data->testPIT, name, keyId, contentId, expectedReturnVector);
        parcBitVector_Release(&backLinkVector);
        parcBuffer_Release(&contentId);
    }
    int delta2 = (int) (parcClock_GetTime(clock) - start);

    if (abs(delta2 - delta1) > (delta1 * 0.20)) {
        testWarn("Steady state time is not constant");
    }
}


int
main(int argc, char *argv[])
{
    LongBowRunner *testRunner = LONGBOW_TEST_RUNNER_CREATE(athena_PIT);
    int exitStatus = longBowMain(argc, argv, testRunner, NULL);
    longBowTestRunner_Destroy(&testRunner);
    exit(exitStatus);
}
