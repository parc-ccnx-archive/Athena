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
#include "../athena_TransportLinkModuleTEMPLATE.c"
#include <LongBow/unit-test.h>
#include <stdio.h>

#include <net/if.h>
#ifndef __linux__
#include <net/if_dl.h>
#endif
#include <ifaddrs.h>

#include <parc/algol/parc_SafeMemory.h>
#include <parc/algol/parc_Network.h>
#include <ccnx/forwarder/athena/athena.h>
#include <ccnx/forwarder/athena/athena_TransportLinkAdapter.h>

#define DEVICE "lo"

LONGBOW_TEST_RUNNER(athena_TransportLinkModuleTEMPLATE)
{
    parcMemory_SetInterface(&PARCSafeMemoryAsPARCMemory);
    LONGBOW_RUN_TEST_FIXTURE(Global);
    LONGBOW_RUN_TEST_FIXTURE(Local);
}

LONGBOW_TEST_RUNNER_SETUP(athena_TransportLinkModuleTEMPLATE)
{
    return LONGBOW_STATUS_SUCCEEDED;
}

LONGBOW_TEST_RUNNER_TEARDOWN(athena_TransportLinkModuleTEMPLATE)
{
    return LONGBOW_STATUS_SUCCEEDED;
}

LONGBOW_TEST_FIXTURE(Global)
{
    LONGBOW_RUN_TEST_CASE(Global, athenaTransportLinkModuleTEMPLATE_OpenClose);
    LONGBOW_RUN_TEST_CASE(Global, athenaTransportLinkModuleTEMPLATE_SendReceive);
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

LONGBOW_TEST_CASE(Global, athenaTransportLinkModuleTEMPLATE_OpenClose)
{
    PARCURI *connectionURI;
    const char *result;
    char linkSpecificationURI[MAXPATHLEN];

    AthenaTransportLinkAdapter *athenaTransportLinkAdapter = athenaTransportLinkAdapter_Create(_removeLink, NULL);
    assertNotNull(athenaTransportLinkAdapter, "athenaTransportLinkAdapter_Create returned NULL");

    athenaTransportLinkAdapter_SetLogLevel(athenaTransportLinkAdapter, PARCLogLevel_Debug);

    sprintf(linkSpecificationURI, "template:///name=");
    connectionURI = parcURI_Parse(linkSpecificationURI);
    result = athenaTransportLinkAdapter_Open(athenaTransportLinkAdapter, connectionURI);
    assertTrue(result == NULL, "athenaTransportLinkAdapter_Open failed to detect bad name argument");
    parcURI_Release(&connectionURI);

    sprintf(linkSpecificationURI, "template:///local=");
    connectionURI = parcURI_Parse(linkSpecificationURI);
    result = athenaTransportLinkAdapter_Open(athenaTransportLinkAdapter, connectionURI);
    assertTrue(result == NULL, "athenaTransportLinkAdapter_Open failed to detect bad local argument");
    parcURI_Release(&connectionURI);

    sprintf(linkSpecificationURI, "template:///nameo=");
    connectionURI = parcURI_Parse(linkSpecificationURI);
    result = athenaTransportLinkAdapter_Open(athenaTransportLinkAdapter, connectionURI);
    assertTrue(result == NULL, "athenaTransportLinkAdapter_Open failed to detect bad name specification");
    parcURI_Release(&connectionURI);

    sprintf(linkSpecificationURI, "template:///name=TEMPLATE_1");
    connectionURI = parcURI_Parse(linkSpecificationURI);
    result = athenaTransportLinkAdapter_Open(athenaTransportLinkAdapter, connectionURI);
    assertTrue(result != NULL, "athenaTransportLinkAdapter_Open failed (%s)", strerror(errno));
    parcURI_Release(&connectionURI);

    sprintf(linkSpecificationURI, "template:///name=TEMPLATE_1");
    connectionURI = parcURI_Parse(linkSpecificationURI);
    result = athenaTransportLinkAdapter_Open(athenaTransportLinkAdapter, connectionURI);
    assertTrue(result == NULL, "athenaTransportLinkAdapter_Open succeeded in opening a duplicate link");
    parcURI_Release(&connectionURI);

    int closeResult = athenaTransportLinkAdapter_CloseByName(athenaTransportLinkAdapter, "TEMPLATE_1");
    assertTrue(closeResult == 0, "athenaTransportLinkAdapter_CloseByName failed (%s)", strerror(errno));

    athenaTransportLinkAdapter_Destroy(&athenaTransportLinkAdapter);
}

LONGBOW_TEST_CASE(Global, athenaTransportLinkModuleTEMPLATE_SendReceive)
{
    PARCURI *connectionURI;
    char linkSpecificationURI[MAXPATHLEN];
    const char *result;

    AthenaTransportLinkAdapter *athenaTransportLinkAdapter = athenaTransportLinkAdapter_Create(_removeLink, NULL);
    assertNotNull(athenaTransportLinkAdapter, "athenaTransportLinkAdapter_Create returned NULL");

    // Open a link we can send messages on
    sprintf(linkSpecificationURI, "template:///name=TEMPLATE_1");
    connectionURI = parcURI_Parse(linkSpecificationURI);
    result = athenaTransportLinkAdapter_Open(athenaTransportLinkAdapter, connectionURI);
    assertTrue(result != NULL, "athenaTransportLinkAdapter_Open failed (%s)", strerror(errno));
    parcURI_Release(&connectionURI);

    // Enable debug logging after all instances are open
    athenaTransportLinkAdapter_SetLogLevel(athenaTransportLinkAdapter, PARCLogLevel_Debug);

    // Construct an interest
    CCNxName *name = ccnxName_CreateFromCString("lci:/foo/bar");
    CCNxMetaMessage *ccnxMetaMessage = ccnxInterest_CreateSimple(name);
    ccnxName_Release(&name);

    PARCBitVector *sendVector = parcBitVector_Create();

    // Send the interest out on the link, this message will also be received by ourself
    // since we're sending it to our own MAC destination.
    int linkId = athenaTransportLinkAdapter_LinkNameToId(athenaTransportLinkAdapter, "TEMPLATE_1");
    parcBitVector_Set(sendVector, linkId);

    athena_EncodeMessage(ccnxMetaMessage);
    PARCBitVector *resultVector;
    resultVector = athenaTransportLinkAdapter_Send(athenaTransportLinkAdapter, ccnxMetaMessage, sendVector);
    assertNull(resultVector, "athenaTransportLinkAdapter_Send failed");

    // Send the message a second time
    resultVector = athenaTransportLinkAdapter_Send(athenaTransportLinkAdapter, ccnxMetaMessage, sendVector);
    assertNull(resultVector, "athenaTransportLinkAdapter_Send failed");
    parcBitVector_Release(&sendVector);

    ccnxMetaMessage_Release(&ccnxMetaMessage);

    ccnxMetaMessage = athenaTransportLinkAdapter_Receive(athenaTransportLinkAdapter, &resultVector, 0);
    assertNotNull(resultVector, "athenaTransportLinkAdapter_Receive failed");
    assertTrue(parcBitVector_NumberOfBitsSet(resultVector) == 1, "athenaTransportLinkAdapter_Receive return message with more than one ingress link");
    assertNotNull(ccnxMetaMessage, "athenaTransportLinkAdapter_Receive failed to provide message");
    parcBitVector_Release(&resultVector);
    ccnxMetaMessage_Release(&ccnxMetaMessage);

    // Receive the duplicate
    ccnxMetaMessage = athenaTransportLinkAdapter_Receive(athenaTransportLinkAdapter, &resultVector, 0);
    parcBitVector_Release(&resultVector);
    ccnxMetaMessage_Release(&ccnxMetaMessage);

    int closeResult = athenaTransportLinkAdapter_CloseByName(athenaTransportLinkAdapter, "TEMPLATE_1");
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
    LongBowRunner *testRunner = LONGBOW_TEST_RUNNER_CREATE(athena_TransportLinkModuleTEMPLATE);
    exit(longBowMain(argc, argv, testRunner, NULL));
}
