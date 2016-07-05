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
 * @author Kevin Fox, Palo Alto Research Center (Xerox PARC)
 * @copyright (c) 2015, Xerox Corporation (Xerox) and Palo Alto Research Center, Inc (PARC).  All rights reserved.
 */
#include <config.h>
#include "../athena_TransportLinkModuleUDP.c"
#include <LongBow/unit-test.h>

#include <parc/algol/parc_SafeMemory.h>
#include <parc/algol/parc_Network.h>
#include <ccnx/forwarder/athena/athena.h>
#include <ccnx/forwarder/athena/athena_TransportLinkAdapter.h>

LONGBOW_TEST_RUNNER(athena_TransportLinkModuleUDP)
{
    parcMemory_SetInterface(&PARCSafeMemoryAsPARCMemory);
    LONGBOW_RUN_TEST_FIXTURE(Global);
    LONGBOW_RUN_TEST_FIXTURE(Local);
}

LONGBOW_TEST_RUNNER_SETUP(athena_TransportLinkModuleUDP)
{
    return LONGBOW_STATUS_SUCCEEDED;
}

LONGBOW_TEST_RUNNER_TEARDOWN(athena_TransportLinkModuleUDP)
{
    return LONGBOW_STATUS_SUCCEEDED;
}

LONGBOW_TEST_FIXTURE(Global)
{
    LONGBOW_RUN_TEST_CASE(Global, athenaTransportLinkModuleUDP_OpenClose);
    LONGBOW_RUN_TEST_CASE(Global, athenaTransportLinkModuleUDP6_OpenClose);
    LONGBOW_RUN_TEST_CASE(Global, athenaTransportLinkModuleUDP_SendReceive);
    LONGBOW_RUN_TEST_CASE(Global, athenaTransportLinkModuleUDP6_SendReceive);
    LONGBOW_RUN_TEST_CASE(Global, athenaTransportLinkModuleUDP_SendReceiveFragments);
    LONGBOW_RUN_TEST_CASE(Global, athenaTransportLinkModuleUDP6_SendReceiveFragments);
    LONGBOW_RUN_TEST_CASE(Global, athenaTransportLinkModuleUDP_MTU);
    LONGBOW_RUN_TEST_CASE(Global, athenaTransportLinkModuleUDP_P2P);
    LONGBOW_RUN_TEST_CASE(Global, athenaTransportLinkModuleUDP_Local);
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
    assertNull(context, "_removeLink called with a non null argument");
    //AthenaTransportLinkAdapter *athenaTransportLinkAdapter = (AthenaTransportLinkAdapter *) context;
}

LONGBOW_TEST_CASE(Global, athenaTransportLinkModuleUDP_OpenClose)
{
    PARCURI *connectionURI;
    const char *result;
    AthenaTransportLinkAdapter *athenaTransportLinkAdapter = athenaTransportLinkAdapter_Create(_removeLink, NULL);
    assertNotNull(athenaTransportLinkAdapter, "athenaTransportLinkAdapter_Create returned NULL");

    athenaTransportLinkAdapter_SetLogLevel(athenaTransportLinkAdapter, PARCLogLevel_Debug);

    connectionURI = parcURI_Parse("udp://127.0.0.1:40000/name=");
    result = athenaTransportLinkAdapter_Open(athenaTransportLinkAdapter, connectionURI);
    assertTrue(result == NULL, "athenaTransportLinkAdapter_Open failed to detect bad name argument");
    parcURI_Release(&connectionURI);

    connectionURI = parcURI_Parse("udp://127.0.0.1:40000/local=");
    result = athenaTransportLinkAdapter_Open(athenaTransportLinkAdapter, connectionURI);
    assertTrue(result == NULL, "athenaTransportLinkAdapter_Open failed to detect bad local argument");
    parcURI_Release(&connectionURI);

    connectionURI = parcURI_Parse("udp://127.0.0.1:40000/src=");
    result = athenaTransportLinkAdapter_Open(athenaTransportLinkAdapter, connectionURI);
    assertTrue(result == NULL, "athenaTransportLinkAdapter_Open failed to detect bad source argument");
    parcURI_Release(&connectionURI);

    connectionURI = parcURI_Parse("udp://127.0.0.1/name=UDP_1");
    result = athenaTransportLinkAdapter_Open(athenaTransportLinkAdapter, connectionURI);
    assertTrue(result == NULL, "athenaTransportLinkAdapter_Open failed to detect bad address specification");
    parcURI_Release(&connectionURI);

    connectionURI = parcURI_Parse("udp://127.0.0.1:40000/Listene/name=UDP_1");
    result = athenaTransportLinkAdapter_Open(athenaTransportLinkAdapter, connectionURI);
    assertTrue(result == NULL, "athenaTransportLinkAdapter_Open failed to detect bad argument");
    parcURI_Release(&connectionURI);

    connectionURI = parcURI_Parse("udp://127.0.0.1:40000/Listener/nameo=");
    result = athenaTransportLinkAdapter_Open(athenaTransportLinkAdapter, connectionURI);
    assertTrue(result == NULL, "athenaTransportLinkAdapter_Open failed to detect bad name specification");
    parcURI_Release(&connectionURI);

    connectionURI = parcURI_Parse("udp://127.0.0.1:40000/Listener/name=");
    result = athenaTransportLinkAdapter_Open(athenaTransportLinkAdapter, connectionURI);
    assertTrue(result == NULL, "athenaTransportLinkAdapter_Open failed to detect bad name specification");
    parcURI_Release(&connectionURI);

    connectionURI = parcURI_Parse("udp://127.0.0.1:40000/Listener/name=UDP_1");
    result = athenaTransportLinkAdapter_Open(athenaTransportLinkAdapter, connectionURI);
    assertTrue(result != NULL, "athenaTransportLinkAdapter_Open failed (%s)", strerror(errno));
    parcURI_Release(&connectionURI);

    connectionURI = parcURI_Parse("udp://127.0.0.1:40000/Listener/name=UDP_1");
    result = athenaTransportLinkAdapter_Open(athenaTransportLinkAdapter, connectionURI);
    assertTrue(result == NULL, "athenaTransportLinkAdapter_Open succeeded in opening a duplicate link");
    parcURI_Release(&connectionURI);

    int closeResult = athenaTransportLinkAdapter_CloseByName(athenaTransportLinkAdapter, "UDP_1");
    assertTrue(closeResult == 0, "athenaTransportLinkAdapter_CloseByName failed (%s)", strerror(errno));

    athenaTransportLinkAdapter_Destroy(&athenaTransportLinkAdapter);
}

LONGBOW_TEST_CASE(Global, athenaTransportLinkModuleUDP6_OpenClose)
{
    PARCURI *connectionURI;
    const char *result;
    AthenaTransportLinkAdapter *athenaTransportLinkAdapter = athenaTransportLinkAdapter_Create(_removeLink, NULL);
    assertNotNull(athenaTransportLinkAdapter, "athenaTransportLinkAdapter_Create returned NULL");

    athenaTransportLinkAdapter_SetLogLevel(athenaTransportLinkAdapter, PARCLogLevel_Debug);

    connectionURI = parcURI_Parse("udp6://localhost:40000/name=");
    result = athenaTransportLinkAdapter_Open(athenaTransportLinkAdapter, connectionURI);
    assertTrue(result == NULL, "athenaTransportLinkAdapter_Open failed to detect bad name argument");
    parcURI_Release(&connectionURI);

    connectionURI = parcURI_Parse("udp6://localhost:40000/local=");
    result = athenaTransportLinkAdapter_Open(athenaTransportLinkAdapter, connectionURI);
    assertTrue(result == NULL, "athenaTransportLinkAdapter_Open failed to detect bad local argument");
    parcURI_Release(&connectionURI);

    connectionURI = parcURI_Parse("udp6://localhost:40000/src=");
    result = athenaTransportLinkAdapter_Open(athenaTransportLinkAdapter, connectionURI);
    assertTrue(result == NULL, "athenaTransportLinkAdapter_Open failed to detect bad source argument");
    parcURI_Release(&connectionURI);

    connectionURI = parcURI_Parse("udp6://localhost/name=UDP_1");
    result = athenaTransportLinkAdapter_Open(athenaTransportLinkAdapter, connectionURI);
    assertTrue(result == NULL, "athenaTransportLinkAdapter_Open failed to detect bad address specification");
    parcURI_Release(&connectionURI);

    connectionURI = parcURI_Parse("udp6://localhost:40000/Listene/name=UDP_1");
    result = athenaTransportLinkAdapter_Open(athenaTransportLinkAdapter, connectionURI);
    assertTrue(result == NULL, "athenaTransportLinkAdapter_Open failed to detect bad argument");
    parcURI_Release(&connectionURI);

    connectionURI = parcURI_Parse("udp6://localhost:40000/Listener/nameo=");
    result = athenaTransportLinkAdapter_Open(athenaTransportLinkAdapter, connectionURI);
    assertTrue(result == NULL, "athenaTransportLinkAdapter_Open failed to detect bad name specification");
    parcURI_Release(&connectionURI);

    connectionURI = parcURI_Parse("udp6://localhost:40000/Listener/name=");
    result = athenaTransportLinkAdapter_Open(athenaTransportLinkAdapter, connectionURI);
    assertTrue(result == NULL, "athenaTransportLinkAdapter_Open failed to detect bad name specification");
    parcURI_Release(&connectionURI);

    connectionURI = parcURI_Parse("udp6://[::1]:40000/Listener/name=UDP_1");
    result = athenaTransportLinkAdapter_Open(athenaTransportLinkAdapter, connectionURI);
    assertTrue(result != NULL, "athenaTransportLinkAdapter_Open failed (%s)", strerror(errno));
    parcURI_Release(&connectionURI);

    connectionURI = parcURI_Parse("udp6://:40000/Listener/name=UDP_1");
    result = athenaTransportLinkAdapter_Open(athenaTransportLinkAdapter, connectionURI);
    assertTrue(result == NULL, "athenaTransportLinkAdapter_Open succeeded in opening a duplicate link");
    parcURI_Release(&connectionURI);

    int closeResult = athenaTransportLinkAdapter_CloseByName(athenaTransportLinkAdapter, "UDP_1");
    assertTrue(closeResult == 0, "athenaTransportLinkAdapter_CloseByName failed (%s)", strerror(errno));

    connectionURI = parcURI_Parse("udp6://[::0:40000/Listener/name=UDP_1");
    result = athenaTransportLinkAdapter_Open(athenaTransportLinkAdapter, connectionURI);
    assertTrue(result == NULL, "athenaTransportLinkAdapter_Open failed to detect bad IPv6 address");
    parcURI_Release(&connectionURI);

    athenaTransportLinkAdapter_Destroy(&athenaTransportLinkAdapter);
}

LONGBOW_TEST_CASE(Global, athenaTransportLinkModuleUDP_SendReceive)
{
    PARCURI *connectionURI;
    const char *result;
    AthenaTransportLinkAdapter *athenaTransportLinkAdapter = athenaTransportLinkAdapter_Create(_removeLink, NULL);
    assertNotNull(athenaTransportLinkAdapter, "athenaTransportLinkAdapter_Create returned NULL");

    athenaTransportLinkAdapter_SetLogLevel(athenaTransportLinkAdapter, PARCLogLevel_Debug);

    connectionURI = parcURI_Parse("udp://127.0.0.1:40000/Listener/name=UDPListener");
    result = athenaTransportLinkAdapter_Open(athenaTransportLinkAdapter, connectionURI);
    assertTrue(result != NULL, "athenaTransportLinkAdapter_Open failed (%s)", strerror(errno));
    parcURI_Release(&connectionURI);

    connectionURI = parcURI_Parse("udp://127.0.0.1:40000/name=UDP_1");
    result = athenaTransportLinkAdapter_Open(athenaTransportLinkAdapter, connectionURI);
    assertTrue(result != NULL, "athenaTransportLinkAdapter_Open failed (%s)", strerror(errno));
    parcURI_Release(&connectionURI);

    athenaTransportLinkAdapter_Poll(athenaTransportLinkAdapter, 0);

    CCNxName *name = ccnxName_CreateFromCString("lci:/foo/bar");
    CCNxMetaMessage *ccnxMetaMessage = ccnxInterest_CreateSimple(name);
    ccnxName_Release(&name);

    PARCBitVector *sendVector = parcBitVector_Create();

    int linkId = athenaTransportLinkAdapter_LinkNameToId(athenaTransportLinkAdapter, "UDP_1");
    parcBitVector_Set(sendVector, linkId);

    athena_EncodeMessage(ccnxMetaMessage);
    PARCBitVector *resultVector;
    resultVector = athenaTransportLinkAdapter_Send(athenaTransportLinkAdapter, ccnxMetaMessage, sendVector);
    assertNull(resultVector, "athenaTransportLinkAdapter_Send failed");
    ccnxMetaMessage_Release(&ccnxMetaMessage);

    usleep(1000);

    ccnxMetaMessage = athenaTransportLinkAdapter_Receive(athenaTransportLinkAdapter, &resultVector, 0);
    assertNotNull(resultVector, "athenaTransportLinkAdapter_Receive failed");
    assertTrue(parcBitVector_NumberOfBitsSet(resultVector) == 1, "athenaTransportLinkAdapter_Receive return message with more than one ingress link");
    assertNotNull(ccnxMetaMessage, "athenaTransportLinkAdapter_Receive failed to provide message");
    parcBitVector_Release(&resultVector);
    ccnxMetaMessage_Release(&ccnxMetaMessage);

    // Try to send a large (>mtu) message
    name = ccnxName_CreateFromCString("lci:/foo/bar");
    ccnxMetaMessage = ccnxInterest_CreateSimple(name);
    ccnxName_Release(&name);

    size_t largePayloadSize = (64 * 1024) - 1;
    char largePayload[largePayloadSize];
    PARCBuffer *payload = parcBuffer_Wrap((void *)largePayload, largePayloadSize, 0, largePayloadSize);
    ccnxInterest_SetPayload(ccnxMetaMessage, payload);
    athena_EncodeMessage(ccnxMetaMessage);

    resultVector = athenaTransportLinkAdapter_Send(athenaTransportLinkAdapter, ccnxMetaMessage, sendVector);
    assertTrue(parcBitVector_NumberOfBitsSet(resultVector) > 0, "athenaTransportLinkAdapter_Send should have failed to send a large message");

    parcBuffer_Release(&payload);
    parcBitVector_Release(&sendVector);
    parcBitVector_Release(&resultVector);
    ccnxMetaMessage_Release(&ccnxMetaMessage);

    // Close one end of the connection and send a message from the other.
    int closeResult = athenaTransportLinkAdapter_CloseByName(athenaTransportLinkAdapter, "UDPListener");
    assertTrue(closeResult == 0, "athenaTransportLinkAdapter_CloseByName failed (%s)", strerror(errno));

    ccnxMetaMessage = athenaTransportLinkAdapter_Receive(athenaTransportLinkAdapter, &resultVector, 1);
    assertNull(resultVector, "athenaTransportLinkAdapter_Receive should have failed");

    closeResult = athenaTransportLinkAdapter_CloseByName(athenaTransportLinkAdapter, "UDP_1");
    assertTrue(closeResult == 0, "athenaTransportLinkAdapter_CloseByName failed (%s)", strerror(errno));

    athenaTransportLinkAdapter_Destroy(&athenaTransportLinkAdapter);
}

LONGBOW_TEST_CASE(Global, athenaTransportLinkModuleUDP6_SendReceive)
{
    PARCURI *connectionURI;
    const char *result;
    AthenaTransportLinkAdapter *athenaTransportLinkAdapter = athenaTransportLinkAdapter_Create(_removeLink, NULL);
    assertNotNull(athenaTransportLinkAdapter, "athenaTransportLinkAdapter_Create returned NULL");

    athenaTransportLinkAdapter_SetLogLevel(athenaTransportLinkAdapter, PARCLogLevel_Debug);

    connectionURI = parcURI_Parse("udp6://localhost:40000/Listener/name=UDPListener");
    result = athenaTransportLinkAdapter_Open(athenaTransportLinkAdapter, connectionURI);
    assertTrue(result != NULL, "athenaTransportLinkAdapter_Open failed (%s)", strerror(errno));
    parcURI_Release(&connectionURI);

    connectionURI = parcURI_Parse("udp6://localhost:40000/name=UDP_1");
    result = athenaTransportLinkAdapter_Open(athenaTransportLinkAdapter, connectionURI);
    assertTrue(result != NULL, "athenaTransportLinkAdapter_Open failed (%s)", strerror(errno));
    parcURI_Release(&connectionURI);

    athenaTransportLinkAdapter_Poll(athenaTransportLinkAdapter, 0);

    CCNxName *name = ccnxName_CreateFromCString("lci:/foo/bar");
    CCNxMetaMessage *ccnxMetaMessage = ccnxInterest_CreateSimple(name);
    ccnxName_Release(&name);

    PARCBitVector *sendVector = parcBitVector_Create();

    int linkId = athenaTransportLinkAdapter_LinkNameToId(athenaTransportLinkAdapter, "UDP_1");
    parcBitVector_Set(sendVector, linkId);

    athena_EncodeMessage(ccnxMetaMessage);
    PARCBitVector *resultVector;
    resultVector = athenaTransportLinkAdapter_Send(athenaTransportLinkAdapter, ccnxMetaMessage, sendVector);
    assertNull(resultVector, "athenaTransportLinkAdapter_Send failed");
    ccnxMetaMessage_Release(&ccnxMetaMessage);

    usleep(1000);

    ccnxMetaMessage = athenaTransportLinkAdapter_Receive(athenaTransportLinkAdapter, &resultVector, 0);
    assertNotNull(resultVector, "athenaTransportLinkAdapter_Receive failed");
    assertTrue(parcBitVector_NumberOfBitsSet(resultVector) == 1, "athenaTransportLinkAdapter_Receive return message with more than one ingress link");
    assertNotNull(ccnxMetaMessage, "athenaTransportLinkAdapter_Receive failed to provide message");
    parcBitVector_Release(&resultVector);
    ccnxMetaMessage_Release(&ccnxMetaMessage);

    // Try to send a large (>mtu) message
    name = ccnxName_CreateFromCString("lci:/foo/bar");
    ccnxMetaMessage = ccnxInterest_CreateSimple(name);
    ccnxName_Release(&name);

    size_t largePayloadSize = (64 * 1024) - 1;
    char largePayload[largePayloadSize];
    PARCBuffer *payload = parcBuffer_Wrap((void *)largePayload, largePayloadSize, 0, largePayloadSize);
    ccnxInterest_SetPayload(ccnxMetaMessage, payload);
    athena_EncodeMessage(ccnxMetaMessage);

    resultVector = athenaTransportLinkAdapter_Send(athenaTransportLinkAdapter, ccnxMetaMessage, sendVector);
    assertTrue(parcBitVector_NumberOfBitsSet(resultVector) > 0, "athenaTransportLinkAdapter_Send should have failed to send a large message");

    parcBuffer_Release(&payload);
    parcBitVector_Release(&sendVector);
    parcBitVector_Release(&resultVector);
    ccnxMetaMessage_Release(&ccnxMetaMessage);

    // Close one end of the connection and send a message from the other.
    int closeResult = athenaTransportLinkAdapter_CloseByName(athenaTransportLinkAdapter, "UDPListener");
    assertTrue(closeResult == 0, "athenaTransportLinkAdapter_CloseByName failed (%s)", strerror(errno));

    ccnxMetaMessage = athenaTransportLinkAdapter_Receive(athenaTransportLinkAdapter, &resultVector, 1);
    assertNull(resultVector, "athenaTransportLinkAdapter_Receive should have failed");

    closeResult = athenaTransportLinkAdapter_CloseByName(athenaTransportLinkAdapter, "UDP_1");
    assertTrue(closeResult == 0, "athenaTransportLinkAdapter_CloseByName failed (%s)", strerror(errno));

    athenaTransportLinkAdapter_Destroy(&athenaTransportLinkAdapter);
}

LONGBOW_TEST_CASE(Global, athenaTransportLinkModuleUDP_SendReceiveFragments)
{
    PARCURI *connectionURI;
    const char *result;
    size_t mtu = 1500; // forced MTU size to make sure we fragment large messages
    char linkSpecificationURI[MAXPATHLEN];

    AthenaTransportLinkAdapter *athenaTransportLinkAdapter = athenaTransportLinkAdapter_Create(_removeLink, NULL);
    assertNotNull(athenaTransportLinkAdapter, "athenaTransportLinkAdapter_Create returned NULL");

    sprintf(linkSpecificationURI, "udp://127.0.0.1:40000/Listener/name=UDPListener/mtu=%zu/fragmenter=BEFS", mtu);
    connectionURI = parcURI_Parse(linkSpecificationURI);
    result = athenaTransportLinkAdapter_Open(athenaTransportLinkAdapter, connectionURI);
    assertTrue(result != NULL, "athenaTransportLinkAdapter_Open failed (%s)", strerror(errno));
    parcURI_Release(&connectionURI);

    sprintf(linkSpecificationURI, "udp://127.0.0.1:40000/name=UDP_1/mtu=%zu/fragmenter=BEFS", mtu);
    connectionURI = parcURI_Parse(linkSpecificationURI);
    result = athenaTransportLinkAdapter_Open(athenaTransportLinkAdapter, connectionURI);
    assertTrue(result != NULL, "athenaTransportLinkAdapter_Open failed (%s)", strerror(errno));
    parcURI_Release(&connectionURI);

    athenaTransportLinkAdapter_SetLogLevel(athenaTransportLinkAdapter, PARCLogLevel_Debug);

    athenaTransportLinkAdapter_Poll(athenaTransportLinkAdapter, 0);

    // Construct an interest
    CCNxName *name = ccnxName_CreateFromCString("lci:/foo/bar");
    CCNxMetaMessage *sendMessage = ccnxInterest_CreateSimple(name);
    CCNxMetaMessage *receivedMessage = NULL;
    ccnxName_Release(&name);

    ssize_t numberOfFragments = (64 * 1024) / mtu;
    size_t largePayloadSize = numberOfFragments * mtu;
    char largePayload[largePayloadSize];

    PARCBuffer *payload = parcBuffer_Wrap((void *)largePayload, largePayloadSize, 0, largePayloadSize);
    ccnxInterest_SetPayload(sendMessage, payload);
    parcBuffer_Release(&payload);
    athena_EncodeMessage(sendMessage);

    PARCBitVector *sendVector = parcBitVector_Create();
    int linkId = athenaTransportLinkAdapter_LinkNameToId(athenaTransportLinkAdapter, "UDP_1");
    parcBitVector_Set(sendVector, linkId);

    PARCBitVector *resultVector;
    resultVector = athenaTransportLinkAdapter_Send(athenaTransportLinkAdapter, sendMessage, sendVector);
    assertNull(resultVector, "athenaTransportLinkAdapter_Send failed");

    usleep(1000);

    size_t iterations = (largePayloadSize / mtu) + 5; // Extra reads to allow some delivery latency
    do {
        usleep(1000);
        receivedMessage = athenaTransportLinkAdapter_Receive(athenaTransportLinkAdapter, &resultVector, 0);
    } while ((receivedMessage == NULL) && (iterations--));

    assertNotNull(resultVector, "athenaTransportLinkAdapter_Receive failed");
    assertTrue(parcBitVector_NumberOfBitsSet(resultVector) == 1, "athenaTransportLinkAdapter_Receive return message with more than one ingress link");
    assertNotNull(receivedMessage, "athenaTransportLinkAdapter_Receive failed to reassemble message");
    ccnxMetaMessage_Release(&receivedMessage);

    // Send the message back on the link it was received on, this link was created by the listener
    // so we don't know its name until we send the first message to it.
    parcBitVector_ClearVector(sendVector, sendVector); // zero out the vector
    parcBitVector_SetVector(sendVector, resultVector); // sendVector == resultVector
    parcBitVector_Release(&resultVector);

    resultVector = athenaTransportLinkAdapter_Send(athenaTransportLinkAdapter, sendMessage, sendVector);
    if (resultVector != NULL) {
        parcBitVector_Release(&resultVector);
    }

    iterations = (largePayloadSize / mtu) + 5; // Extra reads to allow some delivery latency
    do {
        usleep(1000);
        receivedMessage = athenaTransportLinkAdapter_Receive(athenaTransportLinkAdapter, &resultVector, 0);
    } while ((receivedMessage == NULL) && (iterations--));

    assertNotNull(resultVector, "athenaTransportLinkAdapter_Receive failed");
    assertTrue(parcBitVector_NumberOfBitsSet(resultVector) == 1, "athenaTransportLinkAdapter_Receive return message with more than one ingress link");
    assertNotNull(receivedMessage, "athenaTransportLinkAdapter_Receive failed to reassemble message");
    ccnxMetaMessage_Release(&receivedMessage);
    parcBitVector_Release(&resultVector);

    parcBitVector_Release(&sendVector);
    ccnxMetaMessage_Release(&sendMessage);

    // Close one end of the connection and send a message from the other.
    int closeResult = athenaTransportLinkAdapter_CloseByName(athenaTransportLinkAdapter, "UDPListener");
    assertTrue(closeResult == 0, "athenaTransportLinkAdapter_CloseByName failed (%s)", strerror(errno));

    receivedMessage = athenaTransportLinkAdapter_Receive(athenaTransportLinkAdapter, &resultVector, 1);
    assertNull(receivedMessage, "athenaTransportLinkAdapter_Receive should have failed");

    closeResult = athenaTransportLinkAdapter_CloseByName(athenaTransportLinkAdapter, "UDP_1");
    assertTrue(closeResult == 0, "athenaTransportLinkAdapter_CloseByName failed (%s)", strerror(errno));

    athenaTransportLinkAdapter_Destroy(&athenaTransportLinkAdapter);
}

LONGBOW_TEST_CASE(Global, athenaTransportLinkModuleUDP6_SendReceiveFragments)
{
    PARCURI *connectionURI;
    const char *result;
    size_t mtu = 1500; // forced MTU size to make sure we fragment large messages
    char linkSpecificationURI[MAXPATHLEN];

    AthenaTransportLinkAdapter *athenaTransportLinkAdapter = athenaTransportLinkAdapter_Create(_removeLink, NULL);
    assertNotNull(athenaTransportLinkAdapter, "athenaTransportLinkAdapter_Create returned NULL");

    sprintf(linkSpecificationURI, "udp6://localhost:40000/Listener/name=UDPListener/mtu=%zu/fragmenter=BEFS", mtu);
    connectionURI = parcURI_Parse(linkSpecificationURI);
    result = athenaTransportLinkAdapter_Open(athenaTransportLinkAdapter, connectionURI);
    assertTrue(result != NULL, "athenaTransportLinkAdapter_Open failed (%s)", strerror(errno));
    parcURI_Release(&connectionURI);

    sprintf(linkSpecificationURI, "udp6://localhost:40000/name=UDP_1/mtu=%zu/fragmenter=BEFS", mtu);
    connectionURI = parcURI_Parse(linkSpecificationURI);
    result = athenaTransportLinkAdapter_Open(athenaTransportLinkAdapter, connectionURI);
    assertTrue(result != NULL, "athenaTransportLinkAdapter_Open failed (%s)", strerror(errno));
    parcURI_Release(&connectionURI);

    athenaTransportLinkAdapter_SetLogLevel(athenaTransportLinkAdapter, PARCLogLevel_Debug);

    athenaTransportLinkAdapter_Poll(athenaTransportLinkAdapter, 0);

    // Construct an interest
    CCNxName *name = ccnxName_CreateFromCString("lci:/foo/bar");
    CCNxMetaMessage *sendMessage = ccnxInterest_CreateSimple(name);
    CCNxMetaMessage *receivedMessage = NULL;
    ccnxName_Release(&name);

    ssize_t numberOfFragments = (64 * 1024) / mtu;
    size_t largePayloadSize = numberOfFragments * mtu;
    char largePayload[largePayloadSize];

    PARCBuffer *payload = parcBuffer_Wrap((void *)largePayload, largePayloadSize, 0, largePayloadSize);
    ccnxInterest_SetPayload(sendMessage, payload);
    parcBuffer_Release(&payload);
    athena_EncodeMessage(sendMessage);

    PARCBitVector *sendVector = parcBitVector_Create();
    int linkId = athenaTransportLinkAdapter_LinkNameToId(athenaTransportLinkAdapter, "UDP_1");
    parcBitVector_Set(sendVector, linkId);

    PARCBitVector *resultVector;
    resultVector = athenaTransportLinkAdapter_Send(athenaTransportLinkAdapter, sendMessage, sendVector);
    assertNull(resultVector, "athenaTransportLinkAdapter_Send failed");

    usleep(1000);

    size_t iterations = (largePayloadSize / mtu) + 5; // Extra reads to allow some delivery latency
    do {
        usleep(1000);
        receivedMessage = athenaTransportLinkAdapter_Receive(athenaTransportLinkAdapter, &resultVector, 0);
    } while ((receivedMessage == NULL) && (iterations--));

    assertNotNull(resultVector, "athenaTransportLinkAdapter_Receive failed");
    assertTrue(parcBitVector_NumberOfBitsSet(resultVector) == 1, "athenaTransportLinkAdapter_Receive return message with more than one ingress link");
    assertNotNull(receivedMessage, "athenaTransportLinkAdapter_Receive failed to reassemble message");
    ccnxMetaMessage_Release(&receivedMessage);

    // Send the message back on the link it was received on, this link was created by the listener
    // so we don't know its name until we send the first message to it.
    parcBitVector_ClearVector(sendVector, sendVector); // zero out the vector
    parcBitVector_SetVector(sendVector, resultVector); // sendVector == resultVector
    parcBitVector_Release(&resultVector);

    resultVector = athenaTransportLinkAdapter_Send(athenaTransportLinkAdapter, sendMessage, sendVector);
    if (resultVector != NULL) {
        parcBitVector_Release(&resultVector);
    }

    iterations = (largePayloadSize / mtu) + 5; // Extra reads to allow some delivery latency
    do {
        usleep(1000);
        receivedMessage = athenaTransportLinkAdapter_Receive(athenaTransportLinkAdapter, &resultVector, 0);
    } while ((receivedMessage == NULL) && (iterations--));

    assertNotNull(resultVector, "athenaTransportLinkAdapter_Receive failed");
    assertTrue(parcBitVector_NumberOfBitsSet(resultVector) == 1, "athenaTransportLinkAdapter_Receive return message with more than one ingress link");
    assertNotNull(receivedMessage, "athenaTransportLinkAdapter_Receive failed to reassemble message");
    ccnxMetaMessage_Release(&receivedMessage);
    parcBitVector_Release(&resultVector);

    parcBitVector_Release(&sendVector);
    ccnxMetaMessage_Release(&sendMessage);

    // Close one end of the connection and send a message from the other.
    int closeResult = athenaTransportLinkAdapter_CloseByName(athenaTransportLinkAdapter, "UDPListener");
    assertTrue(closeResult == 0, "athenaTransportLinkAdapter_CloseByName failed (%s)", strerror(errno));

    receivedMessage = athenaTransportLinkAdapter_Receive(athenaTransportLinkAdapter, &resultVector, 1);
    assertNull(receivedMessage, "athenaTransportLinkAdapter_Receive should have failed");

    closeResult = athenaTransportLinkAdapter_CloseByName(athenaTransportLinkAdapter, "UDP_1");
    assertTrue(closeResult == 0, "athenaTransportLinkAdapter_CloseByName failed (%s)", strerror(errno));

    athenaTransportLinkAdapter_Destroy(&athenaTransportLinkAdapter);
}

LONGBOW_TEST_CASE(Global, athenaTransportLinkModuleUDP_MTU)
{
    PARCURI *connectionURI;
    const char *result;
    AthenaTransportLinkAdapter *athenaTransportLinkAdapter = athenaTransportLinkAdapter_Create(_removeLink, NULL);
    assertNotNull(athenaTransportLinkAdapter, "athenaTransportLinkAdapter_Create returned NULL");

    athenaTransportLinkAdapter_SetLogLevel(athenaTransportLinkAdapter, PARCLogLevel_Debug);

    connectionURI = parcURI_Parse("udp://127.0.0.1:40000/Listener/name=UDPListener/mtu=");
    result = athenaTransportLinkAdapter_Open(athenaTransportLinkAdapter, connectionURI);
    assertTrue(result == NULL, "athenaTransportLinkAdapter_Open failed to detect improper MTU (%s)", strerror(errno));
    parcURI_Release(&connectionURI);

    connectionURI = parcURI_Parse("udp://127.0.0.1:40000/Listener/name=UDPListener/mtu=20");
    result = athenaTransportLinkAdapter_Open(athenaTransportLinkAdapter, connectionURI);
    assertTrue(result != NULL, "athenaTransportLinkAdapter_Open failed (%s)", strerror(errno));
    parcURI_Release(&connectionURI);

    connectionURI = parcURI_Parse("udp://127.0.0.1:40000/name=UDP_1/mtu=20");
    result = athenaTransportLinkAdapter_Open(athenaTransportLinkAdapter, connectionURI);
    assertTrue(result != NULL, "athenaTransportLinkAdapter_Open failed (%s)", strerror(errno));
    parcURI_Release(&connectionURI);

    athenaTransportLinkAdapter_Poll(athenaTransportLinkAdapter, 0);

    CCNxName *name = ccnxName_CreateFromCString("lci:/foo/bar");
    CCNxMetaMessage *ccnxMetaMessage = ccnxInterest_CreateSimple(name);
    ccnxName_Release(&name);

    PARCBitVector *sendVector = parcBitVector_Create();

    int linkId = athenaTransportLinkAdapter_LinkNameToId(athenaTransportLinkAdapter, "UDP_1");
    parcBitVector_Set(sendVector, linkId);

    athena_EncodeMessage(ccnxMetaMessage);
    PARCBitVector *resultVector;
    resultVector = athenaTransportLinkAdapter_Send(athenaTransportLinkAdapter, ccnxMetaMessage, sendVector);
    assertTrue(parcBitVector_NumberOfBitsSet(resultVector) > 0, "athenaTransportLinkAdapter_Send should have failed");
    assertTrue(errno == EMSGSIZE, "athenaTransportLinkAdapter_Send should have failed with EMSGSIZE (%d): %s", errno, strerror(errno));
    parcBitVector_Release(&resultVector);
    ccnxMetaMessage_Release(&ccnxMetaMessage);
    parcBitVector_Release(&sendVector);

    usleep(1000);

    ccnxMetaMessage = athenaTransportLinkAdapter_Receive(athenaTransportLinkAdapter, &resultVector, 0);
    assertNull(ccnxMetaMessage, "athenaTransportLinkAdapter_Receive should have failed");

    int closeResult = athenaTransportLinkAdapter_CloseByName(athenaTransportLinkAdapter, "UDPListener");
    assertTrue(closeResult == 0, "athenaTransportLinkAdapter_CloseByName failed (%s)", strerror(errno));

    closeResult = athenaTransportLinkAdapter_CloseByName(athenaTransportLinkAdapter, "UDP_1");
    assertTrue(closeResult == 0, "athenaTransportLinkAdapter_CloseByName failed (%s)", strerror(errno));

    athenaTransportLinkAdapter_Destroy(&athenaTransportLinkAdapter);
}

LONGBOW_TEST_CASE(Global, athenaTransportLinkModuleUDP_P2P)
{
    PARCURI *connectionURI;
    const char *result;
    AthenaTransportLinkAdapter *athenaTransportLinkAdapter = athenaTransportLinkAdapter_Create(_removeLink, NULL);
    assertNotNull(athenaTransportLinkAdapter, "athenaTransportLinkAdapter_Create returned NULL");

    athenaTransportLinkAdapter_SetLogLevel(athenaTransportLinkAdapter, PARCLogLevel_Debug);

    connectionURI = parcURI_Parse("udp://localhost:40001/src=localhost:40002/name=UDP_0");
    result = athenaTransportLinkAdapter_Open(athenaTransportLinkAdapter, connectionURI);
    assertTrue(result != NULL, "athenaTransportLinkAdapter_Open failed (%s)", strerror(errno));
    parcURI_Release(&connectionURI);

    connectionURI = parcURI_Parse("udp://localhost:40002/src=localhost:40001/name=UDP_1");
    result = athenaTransportLinkAdapter_Open(athenaTransportLinkAdapter, connectionURI);
    assertTrue(result != NULL, "athenaTransportLinkAdapter_Open failed (%s)", strerror(errno));
    parcURI_Release(&connectionURI);

    CCNxName *name = ccnxName_CreateFromCString("lci:/foo/bar");
    CCNxMetaMessage *ccnxMetaMessage = ccnxInterest_CreateSimple(name);
    ccnxName_Release(&name);

    PARCBitVector *sendVector = parcBitVector_Create();

    int linkId = athenaTransportLinkAdapter_LinkNameToId(athenaTransportLinkAdapter, "UDP_1");
    parcBitVector_Set(sendVector, linkId);

    athena_EncodeMessage(ccnxMetaMessage);
    PARCBitVector *resultVector;
    resultVector = athenaTransportLinkAdapter_Send(athenaTransportLinkAdapter, ccnxMetaMessage, sendVector);
    assertNull(resultVector, "athenaTransportLinkAdapter_Send failed");
    ccnxMetaMessage_Release(&ccnxMetaMessage);
    parcBitVector_Release(&sendVector);

    usleep(1000);

    ccnxMetaMessage = athenaTransportLinkAdapter_Receive(athenaTransportLinkAdapter, &resultVector, 0);
    assertNotNull(resultVector, "athenaTransportLinkAdapter_Receive failed");
    assertTrue(parcBitVector_NumberOfBitsSet(resultVector) == 1, "athenaTransportLinkAdapter_Receive return message with more than one ingress link");
    assertNotNull(ccnxMetaMessage, "athenaTransportLinkAdapter_Receive failed to provide message");
    parcBitVector_Release(&resultVector);
    ccnxMetaMessage_Release(&ccnxMetaMessage);

    athenaTransportLinkAdapter_Destroy(&athenaTransportLinkAdapter);
}

LONGBOW_TEST_CASE(Global, athenaTransportLinkModuleUDP_Local)
{
    PARCURI *connectionURI;
    const char *result;
    AthenaTransportLinkAdapter *athenaTransportLinkAdapter = athenaTransportLinkAdapter_Create(_removeLink, NULL);
    assertNotNull(athenaTransportLinkAdapter, "athenaTransportLinkAdapter_Create returned NULL");

    athenaTransportLinkAdapter_SetLogLevel(athenaTransportLinkAdapter, PARCLogLevel_Debug);

    connectionURI = parcURI_Parse("udp://127.0.0.1:40000/Listener/name=UDP_0");
    result = athenaTransportLinkAdapter_Open(athenaTransportLinkAdapter, connectionURI);
    assertTrue(result != NULL, "athenaTransportLinkAdapter_Open failed (%s)", strerror(errno));
    parcURI_Release(&connectionURI);

    connectionURI = parcURI_Parse("udp://127.0.0.1:40000/name=UDP_1/local=boo");
    result = athenaTransportLinkAdapter_Open(athenaTransportLinkAdapter, connectionURI);
    assertTrue(result == NULL, "athenaTransportLinkAdapter_Open failed to detect bad local directive");
    parcURI_Release(&connectionURI);

    connectionURI = parcURI_Parse("udp://127.0.0.1:40000/name=UDP_1/local=true");
    result = athenaTransportLinkAdapter_Open(athenaTransportLinkAdapter, connectionURI);
    assertTrue(result != NULL, "athenaTransportLinkAdapter_Open failed (%s)", strerror(errno));
    parcURI_Release(&connectionURI);

    connectionURI = parcURI_Parse("udp://127.0.0.1:40000/name=UDP_2/local=false");
    result = athenaTransportLinkAdapter_Open(athenaTransportLinkAdapter, connectionURI);
    assertTrue(result != NULL, "athenaTransportLinkAdapter_Open failed (%s)", strerror(errno));
    parcURI_Release(&connectionURI);

    int linkId = athenaTransportLinkAdapter_LinkNameToId(athenaTransportLinkAdapter, "UDP_1");
    assertFalse(athenaTransportLinkAdapter_IsNotLocal(athenaTransportLinkAdapter, linkId), "Local connection not local");

    linkId = athenaTransportLinkAdapter_LinkNameToId(athenaTransportLinkAdapter, "UDP_2");
    assertTrue(athenaTransportLinkAdapter_IsNotLocal(athenaTransportLinkAdapter, linkId), "NonLocal connection is local");

    int closeResult = athenaTransportLinkAdapter_CloseByName(athenaTransportLinkAdapter, "UDP_2");
    assertTrue(closeResult == 0, "athenaTransportLinkAdapter_CloseByName failed (%s)", strerror(errno));

    closeResult = athenaTransportLinkAdapter_CloseByName(athenaTransportLinkAdapter, "UDP_1");
    assertTrue(closeResult == 0, "athenaTransportLinkAdapter_CloseByName failed (%s)", strerror(errno));

    closeResult = athenaTransportLinkAdapter_CloseByName(athenaTransportLinkAdapter, "UDP_0");
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
    LongBowRunner *testRunner = LONGBOW_TEST_RUNNER_CREATE(athena_TransportLinkModuleUDP);
    exit(longBowMain(argc, argv, testRunner, NULL));
}
