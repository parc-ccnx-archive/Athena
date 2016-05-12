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
#include "../athena_TransportLinkModule.c"
#include <LongBow/unit-test.h>

#include <parc/algol/parc_SafeMemory.h>
#include <parc/algol/parc_Network.h>

#include <stdio.h>

LONGBOW_TEST_RUNNER(athena_TransportLinkModule)
{
    parcMemory_SetInterface(&PARCSafeMemoryAsPARCMemory);
    LONGBOW_RUN_TEST_FIXTURE(Global);
    LONGBOW_RUN_TEST_FIXTURE(Local);
}

LONGBOW_TEST_RUNNER_SETUP(athena_TransportLinkModule)
{
    return LONGBOW_STATUS_SUCCEEDED;
}

LONGBOW_TEST_RUNNER_TEARDOWN(athena_TransportLinkModule)
{
    return LONGBOW_STATUS_SUCCEEDED;
}

LONGBOW_TEST_FIXTURE(Global)
{
    LONGBOW_RUN_TEST_CASE(Global, athenaTransportLinkModule_CreateRelease);
    LONGBOW_RUN_TEST_CASE(Global, athenaTransportLinkModule_GetLogger);
    LONGBOW_RUN_TEST_CASE(Global, athenaTransportLinkModule_Poll);
    LONGBOW_RUN_TEST_CASE(Global, athenaTransportLinkModule_Open);
    LONGBOW_RUN_TEST_CASE(Global, athenaTransportLinkModule_GetName);
    LONGBOW_RUN_TEST_CASE(Global, athenaTransportLinkModule_AddRemoveLink);
    LONGBOW_RUN_TEST_CASE(Global, athenaTransportLinkModule_SetAddRemoveLinkCallback);
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

static int
_send_method(AthenaTransportLink *athenaTransportLink, CCNxMetaMessage *ccnxMetaMessage)
{
    return 0;
}

static CCNxMetaMessage *
_receive_method(AthenaTransportLink *athenaTransportLink)
{
    return NULL;
}

static void
_close_method(AthenaTransportLink *athenaTransportLink)
{
}

static int
_addLink(void *context, struct AthenaTransportLink *athenaTransportLink)
{
    return 0;
}

static int
_addLinkFail(void *context, struct AthenaTransportLink *athenaTransportLink)
{
    return -1;
}

static void
_removeLink(void *context, struct AthenaTransportLink *athenaTransportLink)
{
}

static AthenaTransportLink *
_openMethod(struct AthenaTransportLinkModule *athenaTransportLinkModule, PARCURI *arguments)
{
    AthenaTransportLink *athenaTransportLink = athenaTransportLink_Create("test",
                                                                          _send_method,
                                                                          _receive_method,
                                                                          _close_method);

    return athenaTransportLink;
}

static int
_pollMethod(struct AthenaTransportLink *athenaTransportLink, int timeout)
{
    return 0;
}

LONGBOW_TEST_CASE(Global, athenaTransportLinkModule_CreateRelease)
{
    AthenaTransportLinkModule *athenaTransportLinkModule = athenaTransportLinkModule_Create("test", _openMethod, _pollMethod);
    assertNotNull(athenaTransportLinkModule, "athenaTransportLinkModule_Create failed");
    athenaTransportLinkModule_Destroy(&athenaTransportLinkModule);
}

LONGBOW_TEST_CASE(Global, athenaTransportLinkModule_GetLogger)
{
    AthenaTransportLinkModule *athenaTransportLinkModule = athenaTransportLinkModule_Create("test", _openMethod, _pollMethod);
    assertNotNull(athenaTransportLinkModule, "athenaTransportLinkModule_Create failed");
    PARCLog *logger = athenaTransportLinkModule_GetLogger(athenaTransportLinkModule);
    assertNotNull(logger, "athenaTransportLinkModule_GetLogger failed");
    athenaTransportLinkModule_Destroy(&athenaTransportLinkModule);
}

LONGBOW_TEST_CASE(Global, athenaTransportLinkModule_Open)
{
    AthenaTransportLinkModule *athenaTransportLinkModule = athenaTransportLinkModule_Create("test", _openMethod, _pollMethod);
    assertNotNull(athenaTransportLinkModule, "athenaTransportLinkModule_Create failed");
    athenaTransportLinkModule_SetAddLinkCallback(athenaTransportLinkModule, _addLink, NULL);
    athenaTransportLinkModule_SetRemoveLinkCallback(athenaTransportLinkModule, _removeLink, NULL);

    AthenaTransportLink *athenaTransportLink = athenaTransportLinkModule_Open(athenaTransportLinkModule, NULL);
    assertNotNull(athenaTransportLink, "athenaTransportLinkModule_Open failed");

    athenaTransportLinkModule_SetAddLinkCallback(athenaTransportLinkModule, _addLinkFail, NULL);
    athenaTransportLink = athenaTransportLinkModule_Open(athenaTransportLinkModule, NULL);
    assertNull(athenaTransportLink, "athenaTransportLinkModule_Open should have failed");

    athenaTransportLinkModule_Destroy(&athenaTransportLinkModule);
}

LONGBOW_TEST_CASE(Global, athenaTransportLinkModule_Poll)
{
    AthenaTransportLinkModule *athenaTransportLinkModule = athenaTransportLinkModule_Create("test", _openMethod, _pollMethod);
    assertNotNull(athenaTransportLinkModule, "athenaTransportLinkModule_Create failed");
    athenaTransportLinkModule_Poll(athenaTransportLinkModule, 0);
    athenaTransportLinkModule_Destroy(&athenaTransportLinkModule);
}

LONGBOW_TEST_CASE(Global, athenaTransportLinkModule_GetName)
{
    AthenaTransportLinkModule *athenaTransportLinkModule = athenaTransportLinkModule_Create("test", _openMethod, _pollMethod);
    assertNotNull(athenaTransportLinkModule, "athenaTransportLinkModule_Create failed");
    const char *name = athenaTransportLinkModule_GetName(athenaTransportLinkModule);
    assertTrue(strcmp(name, "test") == 0, "athenaTransportLinkModule_GetName failed (%s != test)", name);
    athenaTransportLinkModule_Destroy(&athenaTransportLinkModule);
}

LONGBOW_TEST_CASE(Global, athenaTransportLinkModule_AddRemoveLink)
{
    AthenaTransportLinkModule *athenaTransportLinkModule = athenaTransportLinkModule_Create("test", _openMethod, _pollMethod);
    assertNotNull(athenaTransportLinkModule, "athenaTransportLinkModule_Create failed");

    AthenaTransportLink *athenaTransportLink = athenaTransportLink_Create("test", _send_method, _receive_method, _close_method);
    assertNotNull(athenaTransportLink, "athenaTransportLink_Create failed");

    athenaTransportLinkModule_SetAddLinkCallback(athenaTransportLinkModule, _addLink, NULL);
    athenaTransportLinkModule_SetRemoveLinkCallback(athenaTransportLinkModule, _removeLink, NULL);

    int result = _athenaTransportLinkModule_AddLink(athenaTransportLinkModule, athenaTransportLink);
    assertTrue(result == 0, "athenaTransportLinkModule_AddLink failed (%d)", result);

    _athenaTransportLinkModule_RemoveLink(athenaTransportLinkModule, athenaTransportLink);

    athenaTransportLink_Release(&athenaTransportLink);

    athenaTransportLinkModule_Destroy(&athenaTransportLinkModule);
}

LONGBOW_TEST_CASE(Global, athenaTransportLinkModule_SetAddRemoveLinkCallback)
{
    AthenaTransportLinkModule *athenaTransportLinkModule = athenaTransportLinkModule_Create("test", _openMethod, _pollMethod);
    assertNotNull(athenaTransportLinkModule, "athenaTransportLinkModule_Create failed");

    athenaTransportLinkModule_SetAddLinkCallback(athenaTransportLinkModule, _addLink, NULL);
    athenaTransportLinkModule_SetRemoveLinkCallback(athenaTransportLinkModule, _removeLink, NULL);
    athenaTransportLinkModule_Destroy(&athenaTransportLinkModule);
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
    LongBowRunner *testRunner = LONGBOW_TEST_RUNNER_CREATE(athena_TransportLinkModule);
    exit(longBowMain(argc, argv, testRunner, NULL));
}
