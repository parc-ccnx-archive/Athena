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

#include "../athena.c"

#include <LongBow/unit-test.h>

#include <errno.h>

#include <parc/algol/parc_SafeMemory.h>
#include <parc/security/parc_CryptoHasher.h>
#include <ccnx/common/ccnx_WireFormatMessage.h>
#include <ccnx/common/codec/ccnxCodec_TlvPacket.h>
#include <ccnx/common/validation/ccnxValidation_CRC32C.h>
#include <ccnx/common/ccnx_NameSegmentNumber.h>
#include <ccnx/common/internal/ccnx_InterestDefault.h>

#include <stdio.h>


LONGBOW_TEST_RUNNER(athena)
{
    parcMemory_SetInterface(&PARCSafeMemoryAsPARCMemory);

    LONGBOW_RUN_TEST_FIXTURE(Global);
    LONGBOW_RUN_TEST_FIXTURE(Static);

    LONGBOW_RUN_TEST_FIXTURE(Misc);
}

// The Test Runner calls this function once before any Test Fixtures are run.
LONGBOW_TEST_RUNNER_SETUP(athena)
{
    return LONGBOW_STATUS_SUCCEEDED;
}

// The Test Runner calls this function once after all the Test Fixtures are run.
LONGBOW_TEST_RUNNER_TEARDOWN(athena)
{
    return LONGBOW_STATUS_SUCCEEDED;
}

LONGBOW_TEST_FIXTURE(Global)
{
    LONGBOW_RUN_TEST_CASE(Global, athena_CreateRelease);
    LONGBOW_RUN_TEST_CASE(Global, athena_ProcessInterest);
    LONGBOW_RUN_TEST_CASE(Global, athena_ProcessContentObject);
    LONGBOW_RUN_TEST_CASE(Global, athena_ProcessControl);
    LONGBOW_RUN_TEST_CASE(Global, athena_ProcessInterestReturn);
    LONGBOW_RUN_TEST_CASE(Global, athena_ForwarderEngine);
}

LONGBOW_TEST_FIXTURE_SETUP(Global)
{
    return LONGBOW_STATUS_SUCCEEDED;
}

LONGBOW_TEST_FIXTURE_TEARDOWN(Global)
{
    uint32_t outstandingAllocations = parcSafeMemory_ReportAllocation(STDOUT_FILENO);
    if (outstandingAllocations != 0) {
        printf("%s leaks memory by %d allocations\n", longBowTestCase_GetName(testCase), outstandingAllocations);
        return LONGBOW_STATUS_MEMORYLEAK;
    }
    return LONGBOW_STATUS_SUCCEEDED;
}

LONGBOW_TEST_CASE(Global, athena_CreateRelease)
{
    Athena *athena = athena_Create(100);
    athena_Release(&athena);
}

LONGBOW_TEST_CASE(Global, athena_ProcessInterest)
{
    PARCURI *connectionURI;
    Athena *athena = athena_Create(100);
    CCNxName *name = ccnxName_CreateFromURI("lci:/foo/bar/baz");
    CCNxInterest *interest = ccnxInterest_CreateSimple(name);

    uint64_t chunkNum = 0;
    CCNxNameSegment *chunkSegment = ccnxNameSegmentNumber_Create(CCNxNameLabelType_CHUNK, chunkNum);
    ccnxName_Append(name, chunkSegment);
    ccnxNameSegment_Release(&chunkSegment);

    PARCBuffer *payload = parcBuffer_WrapCString("this is a payload");
    CCNxContentObject *contentObject = ccnxContentObject_CreateWithDataPayload(name, payload);
    parcBuffer_Release(&payload);

    struct timeval tv;
    gettimeofday(&tv, NULL);
    uint64_t nowInMillis = (tv.tv_sec * 1000) + (tv.tv_usec / 1000);
    ccnxContentObject_SetExpiryTime(contentObject, nowInMillis + 100000); // expire in 100 seconds

    connectionURI = parcURI_Parse("tcp://localhost:50100/listener/name=TCPListener");
    const char *result = athenaTransportLinkAdapter_Open(athena->athenaTransportLinkAdapter, connectionURI);
    assertTrue(result != NULL, "athenaTransportLinkAdapter_Open failed(%s)", strerror(errno));
    parcURI_Release(&connectionURI);

    connectionURI = parcURI_Parse("tcp://localhost:50100/name=TCP_0/local=false");
    result = athenaTransportLinkAdapter_Open(athena->athenaTransportLinkAdapter, connectionURI);
    assertTrue(result != NULL, "athenaTransportLinkAdapter_Open failed (%s)", strerror(errno));
    parcURI_Release(&connectionURI);

    connectionURI = parcURI_Parse("tcp://localhost:50100/name=TCP_1/local=false");
    result = athenaTransportLinkAdapter_Open(athena->athenaTransportLinkAdapter, connectionURI);
    assertTrue(result != NULL, "athenaTransportLinkAdapter_Open failed (%s)", strerror(errno));
    parcURI_Release(&connectionURI);

    int linkId = athenaTransportLinkAdapter_LinkNameToId(athena->athenaTransportLinkAdapter, "TCP_0");
    PARCBitVector *interestIngressVector = parcBitVector_Create();
    parcBitVector_Set(interestIngressVector, linkId);

    linkId = athenaTransportLinkAdapter_LinkNameToId(athena->athenaTransportLinkAdapter, "TCP_1");
    PARCBitVector *contentObjectIngressVector = parcBitVector_Create();
    parcBitVector_Set(contentObjectIngressVector, linkId);

    athena_EncodeMessage(interest);
    athena_EncodeMessage(contentObject);

    // Before FIB entry interest should not be forwarded
    athena_ProcessMessage(athena, interest, interestIngressVector);

    // Add route for interest, it should now be forwarded
    athenaFIB_AddRoute(athena->athenaFIB, name, contentObjectIngressVector);
    CCNxName *defaultName = ccnxName_CreateFromURI("lci:/");
    athenaFIB_AddRoute(athena->athenaFIB, defaultName, contentObjectIngressVector);
    ccnxName_Release(&defaultName);

    // Process exact interest match
    athena_ProcessMessage(athena, interest, interestIngressVector);

    // Process a super-interest match
    CCNxName *superName = ccnxName_CreateFromURI("lci:/foo/bar/baz/unmatched");
    CCNxInterest *superInterest = ccnxInterest_CreateSimple(superName);
    athena_EncodeMessage(superInterest);
    athena_ProcessMessage(athena, superInterest, interestIngressVector);
    ccnxName_Release(&superName);
    ccnxInterest_Release(&superInterest);

    // Process no-match/default route interest
    CCNxName *noMatchName = ccnxName_CreateFromURI("lci:/buggs/bunny");
    CCNxInterest *noMatchInterest = ccnxInterest_CreateSimple(noMatchName);
    athena_EncodeMessage(noMatchInterest);
    athena_ProcessMessage(athena, noMatchInterest, interestIngressVector);
    ccnxName_Release(&noMatchName);
    ccnxInterest_Release(&noMatchInterest);

    // Create a matching content object that the store should retain and reply to the following interest with
    athena_ProcessMessage(athena, contentObject, contentObjectIngressVector);
    athena_ProcessMessage(athena, interest, interestIngressVector);

    parcBitVector_Release(&interestIngressVector);
    parcBitVector_Release(&contentObjectIngressVector);

    ccnxName_Release(&name);
    ccnxInterest_Release(&interest);
    ccnxInterest_Release(&contentObject);
    athena_Release(&athena);
}

LONGBOW_TEST_CASE(Global, athena_ProcessContentObject)
{
    PARCURI *connectionURI;
    Athena *athena = athena_Create(100);

    CCNxName *name = ccnxName_CreateFromURI("lci:/cakes/and/pies");
    uint64_t chunkNum = 0;
    CCNxNameSegment *chunkSegment = ccnxNameSegmentNumber_Create(CCNxNameLabelType_CHUNK, chunkNum);
    ccnxName_Append(name, chunkSegment);
    ccnxNameSegment_Release(&chunkSegment);

    PARCBuffer *payload = parcBuffer_WrapCString("this is a payload");
    CCNxContentObject *contentObject = ccnxContentObject_CreateWithDataPayload(name, payload);

    ccnxName_Release(&name);
    parcBuffer_Release(&payload);

    connectionURI = parcURI_Parse("tcp://localhost:50100/listener/name=TCPListener");
    const char *result = athenaTransportLinkAdapter_Open(athena->athenaTransportLinkAdapter, connectionURI);
    assertTrue(result != NULL, "athenaTransportLinkAdapter_Open failed (%s)", strerror(errno));
    parcURI_Release(&connectionURI);

    connectionURI = parcURI_Parse("tcp://localhost:50100/name=TCP_0");
    result = athenaTransportLinkAdapter_Open(athena->athenaTransportLinkAdapter, connectionURI);
    assertTrue(result != NULL, "athenaTransportLinkAdapter_Open failed (%s)", strerror(errno));
    parcURI_Release(&connectionURI);

    int linkId = athenaTransportLinkAdapter_LinkNameToId(athena->athenaTransportLinkAdapter, "TCP_0");
    PARCBitVector *ingressVector = parcBitVector_Create();
    parcBitVector_Set(ingressVector, linkId);

    athena_EncodeMessage(contentObject);

    athena_ProcessMessage(athena, contentObject, ingressVector);

    parcBitVector_Release(&ingressVector);

    ccnxInterest_Release(&contentObject);
    athena_Release(&athena);
}

LONGBOW_TEST_CASE(Global, athena_ProcessControl)
{
    PARCURI *connectionURI;
    Athena *athena = athena_Create(100);

    CCNxControl *control = ccnxControl_CreateFlushRequest();

    connectionURI = parcURI_Parse("tcp://localhost:50100/listener/name=TCPListener");
    const char *result = athenaTransportLinkAdapter_Open(athena->athenaTransportLinkAdapter, connectionURI);
    assertTrue(result != NULL, "athenaTransportLinkAdapter_Open failed (%s)", strerror(errno));
    parcURI_Release(&connectionURI);

    connectionURI = parcURI_Parse("tcp://localhost:50100/name=TCP_0");
    result = athenaTransportLinkAdapter_Open(athena->athenaTransportLinkAdapter, connectionURI);
    assertTrue(result != NULL, "athenaTransportLinkAdapter_Open failed (%s)", strerror(errno));
    parcURI_Release(&connectionURI);

    int linkId = athenaTransportLinkAdapter_LinkNameToId(athena->athenaTransportLinkAdapter, "TCP_0");
    PARCBitVector *ingressVector = parcBitVector_Create();
    parcBitVector_Set(ingressVector, linkId);

    athena_ProcessMessage(athena, control, ingressVector);

    parcBitVector_Release(&ingressVector);

    ccnxInterest_Release(&control);
    athena_Release(&athena);
}

LONGBOW_TEST_CASE(Global, athena_ProcessInterestReturn)
{
    PARCURI *connectionURI;
    Athena *athena = athena_Create(100);

    CCNxName *name = ccnxName_CreateFromURI("lci:/boose/roo/pie");

    CCNxInterest *interest =
        ccnxInterest_CreateWithImpl(&CCNxInterestFacadeV1_Implementation,
                                    name,
                                    CCNxInterestDefault_LifetimeMilliseconds,
                                    NULL,
                                    NULL,
                                    CCNxInterestDefault_HopLimit);
    ccnxName_Release(&name);
    CCNxInterestReturn *interestReturn = ccnxInterestReturn_Create(interest, CCNxInterestReturn_ReturnCode_Congestion);
    ccnxInterest_Release(&interest);

    connectionURI = parcURI_Parse("tcp://localhost:50100/listener/name=TCPListener");
    const char *result = athenaTransportLinkAdapter_Open(athena->athenaTransportLinkAdapter, connectionURI);
    assertTrue(result != NULL, "athenaTransportLinkAdapter_Open failed (%s)", strerror(errno));
    parcURI_Release(&connectionURI);

    connectionURI = parcURI_Parse("tcp://localhost:50100/name=TCP_0");
    result = athenaTransportLinkAdapter_Open(athena->athenaTransportLinkAdapter, connectionURI);
    assertTrue(result != NULL, "athenaTransportLinkAdapter_Open failed (%s)", strerror(errno));
    parcURI_Release(&connectionURI);

    int linkId = athenaTransportLinkAdapter_LinkNameToId(athena->athenaTransportLinkAdapter, "TCP_0");
    PARCBitVector *ingressVector = parcBitVector_Create();
    parcBitVector_Set(ingressVector, linkId);

    athena_EncodeMessage(interestReturn);

    athena_ProcessMessage(athena, interestReturn, ingressVector);

    parcBitVector_Release(&ingressVector);

    ccnxInterest_Release(&interestReturn);
    athena_Release(&athena);
}

LONGBOW_TEST_CASE(Global, athena_ForwarderEngine)
{
    // Create a new athena instance
    Athena *newAthena = athena_Create(AthenaDefaultContentStoreSize);
    assertNotNull(newAthena, "Could not create a new Athena instance");

    // Add a link
    PARCURI *connectionURI = parcURI_Parse("tcp://localhost:50100/listener");
    const char *result = athenaTransportLinkAdapter_Open(newAthena->athenaTransportLinkAdapter, connectionURI);
    assertTrue(result != NULL, "athenaTransportLinkAdapter_Open failed\n");
    parcURI_Release(&connectionURI);

    pthread_t thread;
    // Passing in a reference that will be released by the new thread as the thread may not
    // have time to acquire a reference itself before we release our reference.
    int ret = pthread_create(&thread, NULL, athena_ForwarderEngine, (void *) athena_Acquire(newAthena));
    assertTrue(ret == 0, "pthread_create failed");
    athena_Release(&newAthena);

    // Create a new local instance we can send a quit message from
    Athena *athena = athena_Create(AthenaDefaultContentStoreSize);
    assertNotNull(athena, "Could not create a new Athena instance");

    connectionURI = parcURI_Parse("tcp://localhost:50100/name=TCP_1");
    result = athenaTransportLinkAdapter_Open(athena->athenaTransportLinkAdapter, connectionURI);
    assertTrue(result != NULL, "athenaTransportLinkAdapter_Open failed (%s)", strerror(errno));
    parcURI_Release(&connectionURI);

    PARCBitVector *linkVector = parcBitVector_Create();

    int linkId = athenaTransportLinkAdapter_LinkNameToId(athena->athenaTransportLinkAdapter, "TCP_1");
    parcBitVector_Set(linkVector, linkId);

    CCNxName *name = ccnxName_CreateFromURI(CCNxNameAthenaCommand_Quit);
    CCNxMetaMessage *interest = ccnxInterest_CreateSimple(name);
    ccnxName_Release(&name);

    athena_EncodeMessage(interest);

    PARCBitVector *resultVector = athenaTransportLinkAdapter_Send(athena->athenaTransportLinkAdapter, interest, linkVector);
    assertNotNull(resultVector, "athenaTransportLinkAdapter_Send failed");
    assertTrue(parcBitVector_NumberOfBitsSet(resultVector) == 1, "Exit message not sent");
    ccnxMetaMessage_Release(&interest);
    parcBitVector_Release(&linkVector);
    parcBitVector_Release(&resultVector);

    CCNxMetaMessage *response = athenaTransportLinkAdapter_Receive(athena->athenaTransportLinkAdapter, &resultVector, -1);
    assertNotNull(resultVector, "athenaTransportLinkAdapter_Receive failed");
    assertTrue(parcBitVector_NumberOfBitsSet(resultVector) > 0, "athenaTransportLinkAdapter_Receive failed");
    parcBitVector_Release(&resultVector);
    ccnxMetaMessage_Release(&response);

    athenaTransportLinkAdapter_CloseByName(athena->athenaTransportLinkAdapter, "TCP_1");

    pthread_join(thread, NULL); // Wait for the child athena to actually finish

    athena_Release(&athena);
}

LONGBOW_TEST_FIXTURE(Static)
{
}

LONGBOW_TEST_FIXTURE_SETUP(Static)
{
    return LONGBOW_STATUS_SUCCEEDED;
}

LONGBOW_TEST_FIXTURE_TEARDOWN(Static)
{
    uint32_t outstandingAllocations = parcSafeMemory_ReportAllocation(STDOUT_FILENO);
    if (outstandingAllocations != 0) {
        printf("%s leaks memory by %d allocations\n", longBowTestCase_GetName(testCase), outstandingAllocations);
        return LONGBOW_STATUS_MEMORYLEAK;
    }
    return LONGBOW_STATUS_SUCCEEDED;
}

// Misc. tests

LONGBOW_TEST_FIXTURE(Misc)
{
}

LONGBOW_TEST_FIXTURE_SETUP(Misc)
{
    return LONGBOW_STATUS_SUCCEEDED;
}

LONGBOW_TEST_FIXTURE_TEARDOWN(Misc)
{
    uint32_t outstandingAllocations = parcSafeMemory_ReportAllocation(STDOUT_FILENO);
    if (outstandingAllocations != 0) {
        printf("%s leaks memory by %d allocations\n", longBowTestCase_GetName(testCase), outstandingAllocations);
        return LONGBOW_STATUS_MEMORYLEAK;
    }
    return LONGBOW_STATUS_SUCCEEDED;
}

int
main(int argc, char *argv[])
{
    LongBowRunner *testRunner = LONGBOW_TEST_RUNNER_CREATE(athena);
    int exitStatus = longBowMain(argc, argv, testRunner, NULL);
    longBowTestRunner_Destroy(&testRunner);
    exit(exitStatus);
}
