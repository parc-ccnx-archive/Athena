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
#include "../athena_TransportLinkAdapter.c"
#include <LongBow/unit-test.h>

#include <parc/algol/parc_SafeMemory.h>
#include <parc/algol/parc_Network.h>

#include <stdio.h>


LONGBOW_TEST_RUNNER(athena_TransportLinkAdapter)
{
    parcMemory_SetInterface(&PARCSafeMemoryAsPARCMemory);
    LONGBOW_RUN_TEST_FIXTURE(Global);
    LONGBOW_RUN_TEST_FIXTURE(Local);
}

LONGBOW_TEST_RUNNER_SETUP(athena_TransportLinkAdapter)
{
    return LONGBOW_STATUS_SUCCEEDED;
}

LONGBOW_TEST_RUNNER_TEARDOWN(athena_TransportLinkAdapter)
{
    return LONGBOW_STATUS_SUCCEEDED;
}

LONGBOW_TEST_FIXTURE(Global)
{
    LONGBOW_RUN_TEST_CASE(Global, athenaTransportLinkAdapter_CreateDestroy);
    LONGBOW_RUN_TEST_CASE(Global, athenaTransportLinkAdapter_GetLogger);
    LONGBOW_RUN_TEST_CASE(Global, athenaTransportLinkAdapter_OpenPollClose);
    LONGBOW_RUN_TEST_CASE(Global, athenaTransportLinkAdapter_AddRemoveLink);
    LONGBOW_RUN_TEST_CASE(Global, athenaTransportLinkAdapter_SendReceive);
    LONGBOW_RUN_TEST_CASE(Global, athenaTransportLinkAdapter_LoadLookupRemoveModule);
    LONGBOW_RUN_TEST_CASE(Global, athenaTransportLinkAdapter_NameToIdToName);
    LONGBOW_RUN_TEST_CASE(Global, athenaTransportLinkAdapter_IsNotLocal);
    LONGBOW_RUN_TEST_CASE(Global, athenaTransportLinkAdapter_ListLinks);
    LONGBOW_RUN_TEST_CASE(Global, athenaTransportLinkAdapter_SetLogLevel);
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

void
_removeLink(void *context, PARCBitVector *parcBitVector)
{
    assertNull(context, "_removeLink called with a non null context");
    AthenaTransportLinkAdapter *athenaTransportLinkAdapter = (AthenaTransportLinkAdapter *) context;
}

LONGBOW_TEST_CASE(Global, athenaTransportLinkAdapter_CreateDestroy)
{
    AthenaTransportLinkAdapter *athenaTransportLinkAdapter = athenaTransportLinkAdapter_Create(_removeLink, NULL);
    assertNotNull(athenaTransportLinkAdapter, "athenaTransportLinkAdapter_Create returned NULL");
    athenaTransportLinkAdapter_Destroy(&athenaTransportLinkAdapter);
}

LONGBOW_TEST_CASE(Global, athenaTransportLinkAdapter_GetLogger)
{
    AthenaTransportLinkAdapter *athenaTransportLinkAdapter = athenaTransportLinkAdapter_Create(_removeLink, NULL);
    assertNotNull(athenaTransportLinkAdapter, "athenaTransportLinkAdapter_Create returned NULL");
    PARCLog *logger = athenaTransportLinkAdapter_GetLogger(athenaTransportLinkAdapter);
    assertNotNull(logger, "logger not setup for adapter");
    athenaTransportLinkAdapter_Destroy(&athenaTransportLinkAdapter);
}

LONGBOW_TEST_CASE(Global, athenaTransportLinkAdapter_OpenPollClose)
{
    PARCURI *connectionURI;
    const char *result;
    AthenaTransportLinkAdapter *athenaTransportLinkAdapter = athenaTransportLinkAdapter_Create(_removeLink, NULL);
    assertNotNull(athenaTransportLinkAdapter, "athenaTransportLinkAdapter_Create returned NULL");

    _LoadModule(athenaTransportLinkAdapter, "TCP");

    connectionURI = parcURI_Parse("unknown://127.0.0.1:50200/name=TCP_0");
    result = athenaTransportLinkAdapter_Open(athenaTransportLinkAdapter, connectionURI);
    assertTrue(result == NULL, "athenaTransportLinkAdapter_Open succeeded for Unknown module");
    parcURI_Release(&connectionURI);

    connectionURI = parcURI_Parse("tcp://127.0.0.1:50200/nam=");
    result = athenaTransportLinkAdapter_Open(athenaTransportLinkAdapter, connectionURI);
    assertTrue(result == NULL, "athenaTransportLinkAdapter_Open succeeded for module with bad argument");
    parcURI_Release(&connectionURI);

    connectionURI = parcURI_Parse("tcp://127.0.0.1:50200/listener/name=TCPListener");
    result = athenaTransportLinkAdapter_Open(athenaTransportLinkAdapter, connectionURI);
    assertTrue(result != NULL, "athenaTransportLinkAdapter_Open failed (%s)", strerror(errno));
    parcURI_Release(&connectionURI);

    connectionURI = parcURI_Parse("tcp://127.0.0.1:50200/listener/name=TCPListener");
    result = athenaTransportLinkAdapter_Open(athenaTransportLinkAdapter, connectionURI);
    assertTrue(result == NULL, "athenaTransportLinkAdapter_Open succeeded for duplicate listener name");
    parcURI_Release(&connectionURI);

    connectionURI = parcURI_Parse("tcp://127.0.0.1:50200/name=TCP_0");
    result = athenaTransportLinkAdapter_Open(athenaTransportLinkAdapter, connectionURI);
    assertTrue(result != NULL, "athenaTransportLinkAdapter_Open failed (%s)", strerror(errno));
    parcURI_Release(&connectionURI);

    connectionURI = parcURI_Parse("tcp://127.0.0.1:50200/name=TCP_0");
    result = athenaTransportLinkAdapter_Open(athenaTransportLinkAdapter, connectionURI);
    assertTrue(result == NULL, "athenaTransportLinkAdapter_Open succeeded for duplicate link name");
    parcURI_Release(&connectionURI);

    athenaTransportLinkAdapter_Poll(athenaTransportLinkAdapter, 0);

    int closeResult = athenaTransportLinkAdapter_CloseByName(athenaTransportLinkAdapter, "TCP_0");
    assertTrue(closeResult == 0, "athenaTransportLinkAdapter_CloseByName failed (%s)", strerror(errno));

    closeResult = athenaTransportLinkAdapter_CloseByName(athenaTransportLinkAdapter, "TCP_999");
    assertTrue(closeResult == -1, "athenaTransportLinkAdapter_CloseByName succeeded for unknown link TCP_999");

    athenaTransportLinkAdapter_Destroy(&athenaTransportLinkAdapter);
}

LONGBOW_TEST_CASE(Global, athenaTransportLinkAdapter_NameToIdToName)
{
    PARCURI *connectionURI;
    const char *result;
    AthenaTransportLinkAdapter *athenaTransportLinkAdapter = athenaTransportLinkAdapter_Create(_removeLink, NULL);
    assertNotNull(athenaTransportLinkAdapter, "athenaTransportLinkAdapter_Create returned NULL");

    _LoadModule(athenaTransportLinkAdapter, "TCP");

    connectionURI = parcURI_Parse("tcp://127.0.0.1:50200/Listener/name=TCP_0");
    result = athenaTransportLinkAdapter_Open(athenaTransportLinkAdapter, connectionURI);
    assertTrue(result != NULL, "athenaTransportLinkAdapter_Open failed (%s)", strerror(errno));
    parcURI_Release(&connectionURI);

    connectionURI = parcURI_Parse("tcp://127.0.0.1:50200/name=TCP_1");
    result = athenaTransportLinkAdapter_Open(athenaTransportLinkAdapter, connectionURI);
    assertTrue(result != NULL, "athenaTransportLinkAdapter_Open failed (%s)", strerror(errno));
    parcURI_Release(&connectionURI);

    PARCBitVector *linkVector = parcBitVector_Create();
    int linkId = athenaTransportLinkAdapter_LinkNameToId(athenaTransportLinkAdapter, "TCP_1");
    const char *linkName = athenaTransportLinkAdapter_LinkIdToName(athenaTransportLinkAdapter, linkId);
    assertTrue(strcmp(linkName, "TCP_1") == 0, "athenaTransportLinkAdapter_LinkIdToName failed (%s != TCP_1", linkName);

    linkName = athenaTransportLinkAdapter_LinkIdToName(athenaTransportLinkAdapter, 9999);
    assertTrue(linkName == NULL, "athenaTransportLinkAdapter_LinkIdToName returned name for unknown linkID (9999/%s)", linkName);

    parcBitVector_Set(linkVector, linkId);
    PARCBitVector *resultVector = athenaTransportLinkAdapter_Close(athenaTransportLinkAdapter, linkVector);
    assertNotNull(resultVector, "athenaTransportLinkAdapter_Close failed");
    parcBitVector_Release(&linkVector);
    parcBitVector_Release(&resultVector);

    linkId = athenaTransportLinkAdapter_LinkNameToId(athenaTransportLinkAdapter, "TCP_0");
    assertTrue(linkId == -1, "athenaTransportLinkAdapter_LinkNameToId returned id for non routable link (TCP_0 == %d)", linkId);
    int closeResult = athenaTransportLinkAdapter_CloseByName(athenaTransportLinkAdapter, "TCP_0");
    assertTrue(closeResult == 0, "athenaTransportLinkAdapter_CloseByName failed (%s)", strerror(errno));

    athenaTransportLinkAdapter_Destroy(&athenaTransportLinkAdapter);
}

LONGBOW_TEST_CASE(Global, athenaTransportLinkAdapter_AddRemoveLink)
{
    PARCURI *connectionURI;
    const char *result;
    AthenaTransportLinkAdapter *athenaTransportLinkAdapter = athenaTransportLinkAdapter_Create(_removeLink, NULL);
    assertNotNull(athenaTransportLinkAdapter, "athenaTransportLinkAdapter_Create returned NULL");

    _LoadModule(athenaTransportLinkAdapter, "TCP");

    connectionURI = parcURI_Parse("tcp://127.0.0.1:50200/Listener/name=TCP_0");
    result = athenaTransportLinkAdapter_Open(athenaTransportLinkAdapter, connectionURI);
    assertTrue(result != NULL, "athenaTransportLinkAdapter_Open failed (%s)", strerror(errno));
    parcURI_Release(&connectionURI);

    athenaTransportLinkAdapter_Poll(athenaTransportLinkAdapter, 0);

    int closeResult = athenaTransportLinkAdapter_CloseByName(athenaTransportLinkAdapter, "TCP_0");
    assertTrue(closeResult == 0, "athenaTransportLinkAdapter_CloseByName failed (%s)", strerror(errno));

    athenaTransportLinkAdapter_Destroy(&athenaTransportLinkAdapter);
}

LONGBOW_TEST_CASE(Global, athenaTransportLinkAdapter_SendReceive)
{
    PARCURI *connectionURI;
    const char *result;
    CCNxMetaMessage *receiveMessage;
    PARCBitVector *resultVector;
    AthenaTransportLinkAdapter *athenaTransportLinkAdapter = athenaTransportLinkAdapter_Create(_removeLink, NULL);
    assertNotNull(athenaTransportLinkAdapter, "athenaTransportLinkAdapter_Create returned NULL");

    _LoadModule(athenaTransportLinkAdapter, "TCP");

    connectionURI = parcURI_Parse("tcp://127.0.0.1:50200/Listener/name=TCPListener");
    result = athenaTransportLinkAdapter_Open(athenaTransportLinkAdapter, connectionURI);
    assertTrue(result != NULL, "athenaTransportLinkAdapter_Open failed (%s)", strerror(errno));
    parcURI_Release(&connectionURI);

    receiveMessage = athenaTransportLinkAdapter_Receive(athenaTransportLinkAdapter, &resultVector, 0);
    assertNull(resultVector, "Received message when none sent");

    connectionURI = parcURI_Parse("tcp://127.0.0.1:50200/name=TCP_1");
    result = athenaTransportLinkAdapter_Open(athenaTransportLinkAdapter, connectionURI);
    assertTrue(result != NULL, "athenaTransportLinkAdapter_Open failed (%s)", strerror(errno));
    parcURI_Release(&connectionURI);

    receiveMessage = athenaTransportLinkAdapter_Receive(athenaTransportLinkAdapter, &resultVector, 0);
    assertNull(resultVector, "Received message when none sent");

    connectionURI = parcURI_Parse("tcp://127.0.0.1:50200/name=TCP_2");
    result = athenaTransportLinkAdapter_Open(athenaTransportLinkAdapter, connectionURI);
    assertTrue(result != NULL, "athenaTransportLinkAdapter_Open failed (%s)", strerror(errno));
    parcURI_Release(&connectionURI);

    receiveMessage = athenaTransportLinkAdapter_Receive(athenaTransportLinkAdapter, &resultVector, 0);
    assertNull(resultVector, "Received message when none sent");

    connectionURI = parcURI_Parse("tcp://127.0.0.1:50200/name=TCP_3");
    result = athenaTransportLinkAdapter_Open(athenaTransportLinkAdapter, connectionURI);
    assertTrue(result != NULL, "athenaTransportLinkAdapter_Open failed (%s)", strerror(errno));
    parcURI_Release(&connectionURI);

    receiveMessage = athenaTransportLinkAdapter_Receive(athenaTransportLinkAdapter, &resultVector, 0);
    assertNull(resultVector, "Received message when none sent");

    connectionURI = parcURI_Parse("tcp://127.0.0.1:50200/name=TCP_4");
    result = athenaTransportLinkAdapter_Open(athenaTransportLinkAdapter, connectionURI);
    assertTrue(result != NULL, "athenaTransportLinkAdapter_Open failed (%s)", strerror(errno));
    parcURI_Release(&connectionURI);

    receiveMessage = athenaTransportLinkAdapter_Receive(athenaTransportLinkAdapter, &resultVector, 0);
    assertNull(resultVector, "Received message when none sent");

    connectionURI = parcURI_Parse("tcp://127.0.0.1:50200/name=TCP_5");
    result = athenaTransportLinkAdapter_Open(athenaTransportLinkAdapter, connectionURI);
    assertTrue(result != NULL, "athenaTransportLinkAdapter_Open failed (%s)", strerror(errno));
    parcURI_Release(&connectionURI);

    receiveMessage = athenaTransportLinkAdapter_Receive(athenaTransportLinkAdapter, &resultVector, 0);
    assertNull(resultVector, "Received message when none sent");

    int closeResult = athenaTransportLinkAdapter_CloseByName(athenaTransportLinkAdapter, "TCP_3");
    assertTrue(closeResult == 0, "athenaTransportLinkAdapter_CloseByName failed (%s)", strerror(errno));

    connectionURI = parcURI_Parse("tcp://127.0.0.1:50200/name=TCP_3");
    result = athenaTransportLinkAdapter_Open(athenaTransportLinkAdapter, connectionURI);
    assertTrue(result != NULL, "athenaTransportLinkAdapter_Open failed (%s)", strerror(errno));
    parcURI_Release(&connectionURI);

    receiveMessage = athenaTransportLinkAdapter_Receive(athenaTransportLinkAdapter, &resultVector, 0);
    assertNull(resultVector, "Received message when none sent");

    CCNxName *name = ccnxName_CreateFromURI("lci:/foo/bar");
    CCNxMetaMessage *sendMessage = ccnxInterest_CreateSimple(name);
    ccnxName_Release(&name);

    PARCBitVector *linkVector = parcBitVector_Create();

    int linkId = athenaTransportLinkAdapter_LinkNameToId(athenaTransportLinkAdapter, "TCP_1");
    parcBitVector_Set(linkVector, linkId);
    linkId = athenaTransportLinkAdapter_LinkNameToId(athenaTransportLinkAdapter, "TCP_2");
    parcBitVector_Set(linkVector, linkId);
    linkId = athenaTransportLinkAdapter_LinkNameToId(athenaTransportLinkAdapter, "TCP_3");
    parcBitVector_Set(linkVector, linkId);

    athena_EncodeMessage(sendMessage);

    resultVector = athenaTransportLinkAdapter_Send(athenaTransportLinkAdapter, sendMessage, linkVector);
    assertNotNull(resultVector, "athenaTransportLinkAdapter_Send failed");
    assertTrue(parcBitVector_NumberOfBitsSet(resultVector) == parcBitVector_NumberOfBitsSet(linkVector), "athenaTransportLinkAdapter_Send failed to send to multiple links");
    parcBitVector_Release(&resultVector);

    usleep(1000);

    char *resultString;
    receiveMessage = athenaTransportLinkAdapter_Receive(athenaTransportLinkAdapter, &resultVector, -1);
    assertNotNull(resultVector, "athenaTransportLinkAdapter_Receive failed");
    printf("Receive1 = %s\n", resultString = parcBitVector_ToString(resultVector));
    assertTrue(parcBitVector_NumberOfBitsSet(resultVector) > 0, "athenaTransportLinkAdapter_Receive failed");
    assertTrue(parcBitVector_NumberOfBitsSet(resultVector) == 1, "athenaTransportLinkAdapter_Receive returned message with more than one ingress link (%s)",
               resultString);
    parcMemory_Deallocate(&resultString);
    parcBitVector_Release(&resultVector);
    ccnxMetaMessage_Release(&receiveMessage);

    receiveMessage = athenaTransportLinkAdapter_Receive(athenaTransportLinkAdapter, &resultVector, -1);
    assertNotNull(resultVector, "athenaTransportLinkAdapter_Receive failed");
    printf("Receive2 = %s\n", resultString = parcBitVector_ToString(resultVector));
    assertTrue(parcBitVector_NumberOfBitsSet(resultVector) > 0, "athenaTransportLinkAdapter_Receive failed");
    assertTrue(parcBitVector_NumberOfBitsSet(resultVector) == 1, "athenaTransportLinkAdapter_Receive returned message with more than one ingress link (%s)",
               resultString);
    parcMemory_Deallocate(&resultString);
    parcBitVector_Release(&resultVector);
    ccnxMetaMessage_Release(&receiveMessage);

    receiveMessage = athenaTransportLinkAdapter_Receive(athenaTransportLinkAdapter, &resultVector, -1);
    assertNotNull(resultVector, "athenaTransportLinkAdapter_Receive failed");
    printf("Receive3 = %s\n", resultString = parcBitVector_ToString(resultVector));
    assertTrue(parcBitVector_NumberOfBitsSet(resultVector) > 0, "athenaTransportLinkAdapter_Receive failed");
    assertTrue(parcBitVector_NumberOfBitsSet(resultVector) == 1, "athenaTransportLinkAdapter_Receive returned message with more than one ingress link (%s)",
               resultString);
    parcMemory_Deallocate(&resultString);
    parcBitVector_Release(&resultVector);
    ccnxMetaMessage_Release(&receiveMessage);

    closeResult = athenaTransportLinkAdapter_CloseByName(athenaTransportLinkAdapter, "TCP_1");
    assertTrue(closeResult == 0, "athenaTransportLinkAdapter_CloseByName failed (%s)", strerror(errno));

    closeResult = athenaTransportLinkAdapter_CloseByName(athenaTransportLinkAdapter, "TCP_2");
    assertTrue(closeResult == 0, "athenaTransportLinkAdapter_CloseByName failed (%s)", strerror(errno));

    resultVector = athenaTransportLinkAdapter_Send(athenaTransportLinkAdapter, sendMessage, linkVector);
    assertTrue(parcBitVector_NumberOfBitsSet(resultVector) == 1, "athenaTransportLinkAdapter_Send should have partially failed");
    parcBitVector_Release(&linkVector);
    parcBitVector_Release(&resultVector);

    linkVector = parcBitVector_Create();
    parcBitVector_Set(linkVector, 100);
    resultVector = athenaTransportLinkAdapter_Send(athenaTransportLinkAdapter, sendMessage, linkVector);
    assertTrue(parcBitVector_NumberOfBitsSet(resultVector) == 0, "athenaTransportLinkAdapter_Send should have failed to send to unknown link");
    parcBitVector_Release(&resultVector);
    parcBitVector_Release(&linkVector);
    ccnxMetaMessage_Release(&sendMessage);

    athenaTransportLinkAdapter_Destroy(&athenaTransportLinkAdapter);
}

LONGBOW_TEST_CASE(Global, athenaTransportLinkAdapter_LoadLookupRemoveModule)
{
    int result;
    AthenaTransportLinkAdapter *athenaTransportLinkAdapter = athenaTransportLinkAdapter_Create(_removeLink, NULL);
    assertNotNull(athenaTransportLinkAdapter, "athenaTransportLinkAdapter_Create returned NULL");

    AthenaTransportLinkModule *module = _LoadModule(athenaTransportLinkAdapter, "UDP");
    assertTrue(module, "loading UDPListener module failed");

    module = _LoadModule(athenaTransportLinkAdapter, "UnknownModule");
    assertTrue(module == NULL, "loading UnknownModule module succeeded");

    result = _RemoveModule(athenaTransportLinkAdapter, "UDP");
    assertTrue(result == 0, "_RemoveModule failed %d (%s)", result, strerror(errno));

    module = _LoadModule(athenaTransportLinkAdapter, "TCP");
    assertTrue(module, "loading TCPListener module failed");

    module = _LoadModule(athenaTransportLinkAdapter, "UnknownModule");
    assertTrue(module == NULL, "loading UnknownModule module succeeded");

    result = _RemoveModule(athenaTransportLinkAdapter, "TCP");
    assertTrue(result == 0, "_RemoveModule failed %d (%s)", result, strerror(errno));

    result = _RemoveModule(athenaTransportLinkAdapter, "TCP");
    assertTrue(result == -1, "_RemoveModule succeeded for already removed module TCP");

    athenaTransportLinkAdapter_Destroy(&athenaTransportLinkAdapter);
}

LONGBOW_TEST_CASE(Global, athenaTransportLinkAdapter_IsNotLocal)
{
    PARCURI *connectionURI;
    const char *result;
    AthenaTransportLinkAdapter *athenaTransportLinkAdapter = athenaTransportLinkAdapter_Create(_removeLink, NULL);
    assertNotNull(athenaTransportLinkAdapter, "athenaTransportLinkAdapter_Create returned NULL");

    _LoadModule(athenaTransportLinkAdapter, "TCP");

    connectionURI = parcURI_Parse("tcp://127.0.0.1:50200/Listener/name=TCP_0");
    result = athenaTransportLinkAdapter_Open(athenaTransportLinkAdapter, connectionURI);
    assertTrue(result != NULL, "athenaTransportLinkAdapter_Open failed (%s)", strerror(errno));
    parcURI_Release(&connectionURI);

    connectionURI = parcURI_Parse("tcp://127.0.0.1:50200/name=TCP_1");
    result = athenaTransportLinkAdapter_Open(athenaTransportLinkAdapter, connectionURI);
    assertTrue(result != NULL, "athenaTransportLinkAdapter_Open failed (%s)", strerror(errno));
    parcURI_Release(&connectionURI);

    int linkId = athenaTransportLinkAdapter_LinkNameToId(athenaTransportLinkAdapter, "TCP_1");
    assertFalse(athenaTransportLinkAdapter_IsNotLocal(athenaTransportLinkAdapter, linkId), "Local connection not local");

    assertFalse(athenaTransportLinkAdapter_IsNotLocal(athenaTransportLinkAdapter, -1), "Unknown connection marked local");

    int closeResult = athenaTransportLinkAdapter_CloseByName(athenaTransportLinkAdapter, "TCP_1");
    assertTrue(closeResult == 0, "athenaTransportLinkAdapter_CloseByName failed (%s)", strerror(errno));

    closeResult = athenaTransportLinkAdapter_CloseByName(athenaTransportLinkAdapter, "TCP_0");
    assertTrue(closeResult == 0, "athenaTransportLinkAdapter_CloseByName failed (%s)", strerror(errno));

    athenaTransportLinkAdapter_Destroy(&athenaTransportLinkAdapter);
}

LONGBOW_TEST_CASE(Global, athenaTransportLinkAdapter_ListLinks)
{
    PARCURI *connectionURI;
    const char *result;
    AthenaTransportLinkAdapter *athenaTransportLinkAdapter = athenaTransportLinkAdapter_Create(_removeLink, NULL);
    assertNotNull(athenaTransportLinkAdapter, "athenaTransportLinkAdapter_Create returned NULL");

    _LoadModule(athenaTransportLinkAdapter, "TCP");

    connectionURI = parcURI_Parse("tcp://127.0.0.1:50200/listener/name=TCPListener");
    result = athenaTransportLinkAdapter_Open(athenaTransportLinkAdapter, connectionURI);
    assertTrue(result != NULL, "athenaTransportLinkAdapter_Open failed (%s)", strerror(errno));
    parcURI_Release(&connectionURI);

    connectionURI = parcURI_Parse("tcp://127.0.0.1:50200/name=TCP_0");
    result = athenaTransportLinkAdapter_Open(athenaTransportLinkAdapter, connectionURI);
    assertTrue(result != NULL, "athenaTransportLinkAdapter_Open failed (%s)", strerror(errno));
    parcURI_Release(&connectionURI);

    CCNxName *name = ccnxName_CreateFromURI(CCNxNameAthenaCommand_LinkList);
    CCNxInterest *ccnxMessage = ccnxInterest_CreateSimple(name);
    ccnxName_Release(&name);

    CCNxContentObject *contentObject = athenaTransportLinkAdapter_ProcessMessage(athenaTransportLinkAdapter, ccnxMessage);
    assertNotNull(contentObject, "athenaTransportLinkAdapter_ProcessMessage failed");
    ccnxInterest_Release(&ccnxMessage);
    ccnxContentObject_Release(&contentObject);

    int closeResult = athenaTransportLinkAdapter_CloseByName(athenaTransportLinkAdapter, "TCP_0");
    assertTrue(closeResult == 0, "athenaTransportLinkAdapter_CloseByName failed (%s)", strerror(errno));

    athenaTransportLinkAdapter_Destroy(&athenaTransportLinkAdapter);
}

LONGBOW_TEST_CASE(Global, athenaTransportLinkAdapter_SetLogLevel)
{
    PARCURI *connectionURI;
    const char *result;
    AthenaTransportLinkAdapter *athenaTransportLinkAdapter = athenaTransportLinkAdapter_Create(_removeLink, NULL);
    assertNotNull(athenaTransportLinkAdapter, "athenaTransportLinkAdapter_Create returned NULL");

    _LoadModule(athenaTransportLinkAdapter, "TCP");

    connectionURI = parcURI_Parse("tcp://127.0.0.1:50200/listener/name=TCPListener");
    result = athenaTransportLinkAdapter_Open(athenaTransportLinkAdapter, connectionURI);
    assertTrue(result != NULL, "athenaTransportLinkAdapter_Open failed (%s)", strerror(errno));
    parcURI_Release(&connectionURI);

    connectionURI = parcURI_Parse("tcp://127.0.0.1:50200/name=TCP_0");
    result = athenaTransportLinkAdapter_Open(athenaTransportLinkAdapter, connectionURI);
    assertTrue(result != NULL, "athenaTransportLinkAdapter_Open failed (%s)", strerror(errno));
    parcURI_Release(&connectionURI);

    athenaTransportLinkAdapter_SetLogLevel(athenaTransportLinkAdapter, 0);
    athenaTransportLinkAdapter_SetLogLevel(athenaTransportLinkAdapter, PARCLogLevel_Debug);

    int closeResult = athenaTransportLinkAdapter_CloseByName(athenaTransportLinkAdapter, "TCP_0");
    assertTrue(closeResult == 0, "athenaTransportLinkAdapter_CloseByName failed (%s)", strerror(errno));

    athenaTransportLinkAdapter_Destroy(&athenaTransportLinkAdapter);
}

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

int
main(int argc, char *argv[])
{
    LongBowRunner *testRunner = LONGBOW_TEST_RUNNER_CREATE(athena_TransportLinkAdapter);
    exit(longBowMain(argc, argv, testRunner, NULL));
}
