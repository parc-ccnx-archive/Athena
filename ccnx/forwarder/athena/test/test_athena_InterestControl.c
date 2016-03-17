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

#include "../athena_InterestControl.c"

#include <LongBow/unit-test.h>

#include <errno.h>
#include <pthread.h>

#include <parc/algol/parc_SafeMemory.h>
#include <parc/security/parc_Security.h>
#include <parc/security/parc_PublicKeySignerPkcs12Store.h>
#include <ccnx/common/ccnx_KeystoreUtilities.h>

#include <ccnx/common/codec/ccnxCodec_TlvPacket.h>
#include <ccnx/common/validation/ccnxValidation_CRC32C.h>

LONGBOW_TEST_RUNNER(athena_InterestControl)
{
    parcMemory_SetInterface(&PARCSafeMemoryAsPARCMemory);

    LONGBOW_RUN_TEST_FIXTURE(Global);
    LONGBOW_RUN_TEST_FIXTURE(Static);

    LONGBOW_RUN_TEST_FIXTURE(Misc);
}

// The Test Runner calls this function once before any Test Fixtures are run.
LONGBOW_TEST_RUNNER_SETUP(athena_InterestControl)
{
    return LONGBOW_STATUS_SUCCEEDED;
}

// The Test Runner calls this function once after all the Test Fixtures are run.
LONGBOW_TEST_RUNNER_TEARDOWN(athena_InterestControl)
{
    return LONGBOW_STATUS_SUCCEEDED;
}

LONGBOW_TEST_FIXTURE(Global)
{
    LONGBOW_RUN_TEST_CASE(Global, athenaInterestControl_TransportLinkAdapter);
    LONGBOW_RUN_TEST_CASE(Global, athenaInterestControl_FIB);
    LONGBOW_RUN_TEST_CASE(Global, athenaInterestControl_Set);
    LONGBOW_RUN_TEST_CASE(Global, athenaInterestControl_Quit);
    LONGBOW_RUN_TEST_CASE(Global, athenaInterestControl_Stats);
    LONGBOW_RUN_TEST_CASE(Global, athenaInterestControl_Spawn);
    LONGBOW_RUN_TEST_CASE(Global, athenaInterestControl_Control);
    LONGBOW_RUN_TEST_CASE(Global, athenaInterestControl_ContentStore);
    LONGBOW_RUN_TEST_CASE(Global, athenaInterestControl_PIT);
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

LONGBOW_TEST_CASE(Global, athenaInterestControl_FIB)
{
    const char *linkSpecification;
    PARCBuffer *payload;
    Athena *athena = athena_Create(0);
    PARCBitVector *ingressVector = parcBitVector_Create();

    CCNxName *name = ccnxName_CreateFromCString(CCNxNameAthenaCommand_LinkConnect);
    CCNxInterest *interest = ccnxInterest_CreateSimple(name);
    ccnxName_Release(&name);
    linkSpecification = "tcp://localhost:50600/listener/name=TCPListener";

    payload = parcBuffer_AllocateCString(linkSpecification);
    ccnxInterest_SetPayload(interest, payload);
    parcBuffer_Release(&payload);

    athena_EncodeMessage(interest);

    athenaInterestControl(athena, interest, ingressVector);

    ccnxMetaMessage_Release(&interest);

    name = ccnxName_CreateFromCString(CCNxNameAthenaCommand_LinkConnect);
    interest = ccnxInterest_CreateSimple(name);
    ccnxName_Release(&name);
    linkSpecification = "tcp://localhost:50600/name=TCP_0";

    payload = parcBuffer_AllocateCString(linkSpecification);
    ccnxInterest_SetPayload(interest, payload);
    parcBuffer_Release(&payload);

    athena_EncodeMessage(interest);

    athenaInterestControl(athena, interest, ingressVector);

    ccnxMetaMessage_Release(&interest);

    name = ccnxName_CreateFromCString(CCNxNameAthenaCommand_FIBAddRoute);
    interest = ccnxInterest_CreateSimple(name);
    ccnxName_Release(&name);

    athena_EncodeMessage(interest);

    athenaInterestControl(athena, interest, ingressVector); // missing payload arguments

    ccnxMetaMessage_Release(&interest);

    name = ccnxName_CreateFromCString(CCNxNameAthenaCommand_FIBAddRoute);
    interest = ccnxInterest_CreateSimple(name);
    ccnxName_Release(&name);
    linkSpecification = "/foo/bar TCP_0"; // bad prefix

    payload = parcBuffer_AllocateCString(linkSpecification);
    ccnxInterest_SetPayload(interest, payload);
    parcBuffer_Release(&payload);

    athena_EncodeMessage(interest);

    athenaInterestControl(athena, interest, ingressVector);

    ccnxMetaMessage_Release(&interest);

    name = ccnxName_CreateFromCString(CCNxNameAthenaCommand_FIBAddRoute);
    interest = ccnxInterest_CreateSimple(name);
    ccnxName_Release(&name);
    linkSpecification = "lci:/foo/bar"; // missing link name

    payload = parcBuffer_AllocateCString(linkSpecification);
    ccnxInterest_SetPayload(interest, payload);
    parcBuffer_Release(&payload);

    athena_EncodeMessage(interest);

    athenaInterestControl(athena, interest, ingressVector);

    ccnxMetaMessage_Release(&interest);

    name = ccnxName_CreateFromCString(CCNxNameAthenaCommand_FIBAddRoute);
    interest = ccnxInterest_CreateSimple(name);
    ccnxName_Release(&name);
    linkSpecification = "lci:/foo/bar TCP_0";

    payload = parcBuffer_AllocateCString(linkSpecification);
    ccnxInterest_SetPayload(interest, payload);
    parcBuffer_Release(&payload);

    athena_EncodeMessage(interest);

    athenaInterestControl(athena, interest, ingressVector);

    ccnxMetaMessage_Release(&interest);

    name = ccnxName_CreateFromCString(CCNxNameAthenaCommand_FIBLookup);
    interest = ccnxInterest_CreateSimple(name);
    ccnxName_Release(&name);
    linkSpecification = "lci:/foo/bar";

    payload = parcBuffer_AllocateCString(linkSpecification);
    ccnxInterest_SetPayload(interest, payload);
    parcBuffer_Release(&payload);

    athena_EncodeMessage(interest);

    athenaInterestControl(athena, interest, ingressVector);

    ccnxMetaMessage_Release(&interest);

    name = ccnxName_CreateFromCString(CCNxNameAthenaCommand_FIBList);
    interest = ccnxInterest_CreateSimple(name);
    ccnxName_Release(&name);

    athena_EncodeMessage(interest);

    athenaInterestControl(athena, interest, ingressVector);

    ccnxMetaMessage_Release(&interest);

    name = ccnxName_CreateFromCString(CCNxNameAthenaCommand_FIBRemoveRoute);
    interest = ccnxInterest_CreateSimple(name);
    ccnxName_Release(&name);
    linkSpecification = "TCP_0 lci:/foo/bar";

    payload = parcBuffer_AllocateCString(linkSpecification);
    ccnxInterest_SetPayload(interest, payload);
    parcBuffer_Release(&payload);

    athena_EncodeMessage(interest);

    athenaInterestControl(athena, interest, ingressVector);

    ccnxMetaMessage_Release(&interest);

    parcBitVector_Release(&ingressVector);
    athena_Release(&athena);
}

LONGBOW_TEST_CASE(Global, athenaInterestControl_TransportLinkAdapter)
{
    const char *linkSpecification;
    PARCBuffer *payload;
    Athena *athena = athena_Create(0);
    PARCBitVector *ingressVector = parcBitVector_Create();

    CCNxName *name = ccnxName_CreateFromCString(CCNxNameAthenaCommand_LinkConnect);
    CCNxInterest *interest = ccnxInterest_CreateSimple(name);
    ccnxName_Release(&name);
    linkSpecification = "tcp://localhost:50600/ilster/name=TCPListener";
    printf("link specification %s\n", linkSpecification);

    payload = parcBuffer_AllocateCString(linkSpecification);
    ccnxInterest_SetPayload(interest, payload);
    parcBuffer_Release(&payload);

    athena_EncodeMessage(interest);

    athenaInterestControl(athena, interest, ingressVector);

    ccnxMetaMessage_Release(&interest);

    name = ccnxName_CreateFromCString(CCNxNameAthenaCommand_LinkConnect);
    interest = ccnxInterest_CreateSimple(name);
    ccnxName_Release(&name);

    athena_EncodeMessage(interest);

    athenaInterestControl(athena, interest, ingressVector); // missing payload arguments

    ccnxMetaMessage_Release(&interest);

    name = ccnxName_CreateFromCString(CCNxNameAthenaCommand_LinkConnect);
    interest = ccnxInterest_CreateSimple(name);
    ccnxName_Release(&name);
    linkSpecification = "tcp://localhost:50600/listener/name=TCPListener";

    payload = parcBuffer_AllocateCString(linkSpecification);
    ccnxInterest_SetPayload(interest, payload);
    parcBuffer_Release(&payload);

    athena_EncodeMessage(interest);

    athenaInterestControl(athena, interest, ingressVector);

    ccnxMetaMessage_Release(&interest);

    name = ccnxName_CreateFromCString(CCNxNameAthenaCommand_LinkDisconnect);
    interest = ccnxInterest_CreateSimple(name);
    ccnxName_Release(&name);
    linkSpecification = "TCPListener";

    payload = parcBuffer_AllocateCString(linkSpecification);
    ccnxInterest_SetPayload(interest, payload);
    parcBuffer_Release(&payload);

    athena_EncodeMessage(interest);

    athenaInterestControl(athena, interest, ingressVector);

    ccnxMetaMessage_Release(&interest);

    name = ccnxName_CreateFromCString(CCNxNameAthenaCommand_LinkList);
    interest = ccnxInterest_CreateSimple(name);
    ccnxName_Release(&name);
    linkSpecification = "";

    payload = parcBuffer_AllocateCString(linkSpecification);
    ccnxInterest_SetPayload(interest, payload);
    parcBuffer_Release(&payload);

    athena_EncodeMessage(interest);

    athenaInterestControl(athena, interest, ingressVector);

    ccnxMetaMessage_Release(&interest);

    parcBitVector_Release(&ingressVector);
    athena_Release(&athena);
}

LONGBOW_TEST_CASE(Global, athenaInterestControl_Control)
{
    Athena *athena = athena_Create(0);
    PARCBitVector *ingressVector = parcBitVector_Create();
    CCNxName *name = ccnxName_CreateFromCString(CCNxNameAthena_Control);
    CCNxInterest *interest = ccnxInterest_CreateSimple(name);
    ccnxName_Release(&name);

    athena_EncodeMessage(interest);

    athenaInterestControl(athena, interest, ingressVector);

    ccnxMetaMessage_Release(&interest);

    parcBitVector_Release(&ingressVector);
    athena_Release(&athena);
}

LONGBOW_TEST_CASE(Global, athenaInterestControl_ContentStore)
{
    Athena *athena = athena_Create(0);
    PARCBitVector *ingressVector = parcBitVector_Create();
    CCNxName *name = ccnxName_CreateFromCString(CCNxNameAthena_ContentStore "/unknown");
    CCNxInterest *interest = ccnxInterest_CreateSimple(name);
    ccnxName_Release(&name);

    athena_EncodeMessage(interest);

    athenaInterestControl(athena, interest, ingressVector);

    ccnxMetaMessage_Release(&interest);

    parcBitVector_Release(&ingressVector);
    athena_Release(&athena);
}

LONGBOW_TEST_CASE(Global, athenaInterestControl_PIT)
{
    Athena *athena = athena_Create(0);
    PARCBitVector *ingressVector = parcBitVector_Create();
    CCNxName *name = ccnxName_CreateFromCString(CCNxNameAthena_PIT "/unknown");
    CCNxInterest *interest = ccnxInterest_CreateSimple(name);
    ccnxName_Release(&name);

    athena_EncodeMessage(interest);

    athenaInterestControl(athena, interest, ingressVector);

    ccnxMetaMessage_Release(&interest);

    parcBitVector_Release(&ingressVector);
    athena_Release(&athena);
}

char *_logLevels[] = { "off", "notice", "info", "debug", "error", "all", "unknown", NULL };

LONGBOW_TEST_CASE(Global, athenaInterestControl_Set)
{
    CCNxMetaMessage *response;
    Athena *athena = athena_Create(0);
    char logLevelURI[MAXPATHLEN];

    sprintf(logLevelURI, "%s/invalie", CCNxNameAthenaCommand_Set);
    CCNxName *name = ccnxName_CreateFromCString(logLevelURI);
    CCNxInterest *interest = ccnxInterest_CreateSimple(name);
    ccnxName_Release(&name);

    athena_EncodeMessage(interest);

    response = _Control_Command(athena, interest);
    assertNotNull(response, "Invalid set type not replied to");
    ccnxMetaMessage_Release(&response);

    ccnxMetaMessage_Release(&interest);

    sprintf(logLevelURI, "%s/level", CCNxNameAthenaCommand_Set);
    name = ccnxName_CreateFromCString(logLevelURI);
    interest = ccnxInterest_CreateSimple(name);
    ccnxName_Release(&name);

    athena_EncodeMessage(interest);

    response = _Control_Command(athena, interest);
    assertNotNull(response, "Missing set level not replied to");
    ccnxMetaMessage_Release(&response);

    ccnxMetaMessage_Release(&interest);

    for (int i = 0; _logLevels[i]; i++) {
        sprintf(logLevelURI, "%s/level/%s", CCNxNameAthenaCommand_Set, _logLevels[i]);
        name = ccnxName_CreateFromCString(logLevelURI);
        interest = ccnxInterest_CreateSimple(name);
        ccnxName_Release(&name);

        athena_EncodeMessage(interest);

        response = _Control_Command(athena, interest);
        assertNotNull(response, "Quit command failed");
        ccnxMetaMessage_Release(&response);

        ccnxMetaMessage_Release(&interest);
    }

    athena_Release(&athena);
}

LONGBOW_TEST_CASE(Global, athenaInterestControl_Quit)
{
    Athena *athena = athena_Create(0);

    CCNxName *name = ccnxName_CreateFromCString(CCNxNameAthenaCommand_Quit);
    CCNxInterest *interest = ccnxInterest_CreateSimple(name);
    ccnxName_Release(&name);

    athena_EncodeMessage(interest);

    CCNxMetaMessage *response = _Control_Command(athena, interest);
    assertNotNull(response, "Quit command failed");

    ccnxMetaMessage_Release(&interest);
    ccnxMetaMessage_Release(&response);
    athena_Release(&athena);
}

LONGBOW_TEST_CASE(Global, athenaInterestControl_Stats)
{
    Athena *athena = athena_Create(0);

    CCNxName *name = ccnxName_CreateFromCString(CCNxNameAthenaCommand_Stats);
    CCNxInterest *interest = ccnxInterest_CreateSimple(name);
    ccnxName_Release(&name);

    athena_EncodeMessage(interest);

    CCNxMetaMessage *response = _Control_Command(athena, interest);
    assertNotNull(response, "Stats command failed");

    ccnxMetaMessage_Release(&interest);
    ccnxMetaMessage_Release(&response);
    athena_Release(&athena);
}

LONGBOW_TEST_CASE(Global, athenaInterestControl_Spawn)
{
    Athena *athena = athena_Create(0);

    CCNxName *name = ccnxName_CreateFromCString(CCNxNameAthenaCommand_Run);
    CCNxInterest *interest = ccnxInterest_CreateSimple(name);
    ccnxName_Release(&name);

    // Make sure we detect, and recover from, bad connection specifications.
    PARCBuffer *payload = parcBuffer_AllocateCString("dcp://localhost:50600/listener");
    ccnxInterest_SetPayload(interest, payload);
    parcBuffer_Release(&payload);

    athena_EncodeMessage(interest);

    CCNxMetaMessage *response = _Control_Command(athena, interest);
    assertNotNull(response, "Spawn command failed");
    ccnxMetaMessage_Release(&interest);
    ccnxMetaMessage_Release(&response);

    // Spawn should have failed, so connection to it shouldn't succeed.
    PARCURI *connectionURI = parcURI_Parse("tcp://localhost:50600/name=TCP_1");
    const char *result = athenaTransportLinkAdapter_Open(athena->athenaTransportLinkAdapter, connectionURI);
    assertTrue(result == NULL, "athenaTransportLinkAdapter_Open should have failed");
    parcURI_Release(&connectionURI);

    name = ccnxName_CreateFromCString(CCNxNameAthenaCommand_Run);
    interest = ccnxInterest_CreateSimple(name);
    ccnxName_Release(&name);

    payload = parcBuffer_AllocateCString("tcp://localhost:50600/listener");
    ccnxInterest_SetPayload(interest, payload);
    parcBuffer_Release(&payload);

    athena_EncodeMessage(interest);

    response = _Control_Command(athena, interest);
    assertNotNull(response, "Spawn command failed");
    ccnxMetaMessage_Release(&interest);
    ccnxMetaMessage_Release(&response);

    connectionURI = parcURI_Parse("tcp://localhost:50600/name=TCP_1");
    result = athenaTransportLinkAdapter_Open(athena->athenaTransportLinkAdapter, connectionURI);
    assertTrue(result != NULL, "athenaTransportLinkAdapter_Open failed (%s)", strerror(errno));
    parcURI_Release(&connectionURI);

    PARCBitVector *linkVector = parcBitVector_Create();

    int linkId = athenaTransportLinkAdapter_LinkNameToId(athena->athenaTransportLinkAdapter, "TCP_1");
    parcBitVector_Set(linkVector, linkId);

    name = ccnxName_CreateFromCString(CCNxNameAthenaCommand_Quit);
    interest = ccnxInterest_CreateSimple(name);
    ccnxName_Release(&name);

    athena_EncodeMessage(interest);

    PARCBitVector *resultVector = athenaTransportLinkAdapter_Send(athena->athenaTransportLinkAdapter, interest, linkVector);
    assertNotNull(resultVector, "athenaTransportLinkAdapter_Send failed");
    assertTrue(parcBitVector_NumberOfBitsSet(resultVector) == 1, "Exit message not sent");
    ccnxMetaMessage_Release(&interest);
    parcBitVector_Release(&linkVector);
    parcBitVector_Release(&resultVector);

    //usleep(1000);

    response = athenaTransportLinkAdapter_Receive(athena->athenaTransportLinkAdapter, &resultVector, -1);
    assertNotNull(resultVector, "athenaTransportLinkAdapter_Receive failed");
    assertTrue(parcBitVector_NumberOfBitsSet(resultVector) > 0, "athenaTransportLinkAdapter_Receive failed");
    parcBitVector_Release(&resultVector);
    ccnxMetaMessage_Release(&response);

    athenaTransportLinkAdapter_CloseByName(athena->athenaTransportLinkAdapter, "TCP_1");

    sleep(1); // give spawned thread some time to exit, should use a semaphore here
    athena_Release(&athena);
    //pthread_exit(NULL); // can't wait for spawned threads to exit as this will exit the test
}

LONGBOW_TEST_FIXTURE(Static)
{
    LONGBOW_RUN_TEST_CASE(Static, _create_stats_response);
}

LONGBOW_TEST_FIXTURE_SETUP(Static)
{
    return LONGBOW_STATUS_SUCCEEDED;
}

LONGBOW_TEST_CASE(Static, _create_stats_response)
{
    Athena *athena = athena_Create(0);

    CCNxName *name = ccnxName_CreateFromCString(CCNxNameAthena_Control "/stats");

    CCNxMetaMessage *response = _create_stats_response(athena, name);

    assertTrue(ccnxMetaMessage_IsContentObject(response), "Expected a content object response");
    assertNotNull(ccnxContentObject_GetPayload(response), "Expected a non-NULL payload");
    parcBuffer_Display(ccnxContentObject_GetPayload(response), 0);
    ccnxMetaMessage_Release(&response);
    ccnxName_Release(&name);
    athena_Release(&athena);
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
    LongBowRunner *testRunner = LONGBOW_TEST_RUNNER_CREATE(athena_InterestControl);
    int exitStatus = longBowMain(argc, argv, testRunner, NULL);
    longBowTestRunner_Destroy(&testRunner);
    exit(exitStatus);
}
