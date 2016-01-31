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
#include "../athena_TransportLink.c"
#include <LongBow/unit-test.h>

#include <parc/algol/parc_SafeMemory.h>
#include <parc/algol/parc_Network.h>

#include <stdio.h>


LONGBOW_TEST_RUNNER(athena_TransportLink)
{
    parcMemory_SetInterface(&PARCSafeMemoryAsPARCMemory);
    LONGBOW_RUN_TEST_FIXTURE(Global);
    LONGBOW_RUN_TEST_FIXTURE(Local);
}

LONGBOW_TEST_RUNNER_SETUP(athena_TransportLink)
{
    return LONGBOW_STATUS_SUCCEEDED;
}

LONGBOW_TEST_RUNNER_TEARDOWN(athena_TransportLink)
{
    return LONGBOW_STATUS_SUCCEEDED;
}

LONGBOW_TEST_FIXTURE(Global)
{
    LONGBOW_RUN_TEST_CASE(Global, athenaTransportLink_CreateAcquireRelease);
    LONGBOW_RUN_TEST_CASE(Global, athenaTransportLink_Clone);
    LONGBOW_RUN_TEST_CASE(Global, athenaTransportLink_GetLogger);
    LONGBOW_RUN_TEST_CASE(Global, athenaTransportLink_SendReceiveClose);
    LONGBOW_RUN_TEST_CASE(Global, athenaTransportLink_GetName);
    LONGBOW_RUN_TEST_CASE(Global, athenaTransportLink_GetSetEvent);
    LONGBOW_RUN_TEST_CASE(Global, athenaTransportLink_GetSetPrivateData);
    LONGBOW_RUN_TEST_CASE(Global, athenaTransportLink_AddRemoveCallback);
    LONGBOW_RUN_TEST_CASE(Global, athenaTransportLinkAdapter_SetGetEventFd);
    LONGBOW_RUN_TEST_CASE(Global, athenaTransportLinkAdapter_Routable);
    LONGBOW_RUN_TEST_CASE(Global, athenaTransportLinkAdapter_IsNotLocal);
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

LONGBOW_TEST_CASE(Global, athenaTransportLink_CreateAcquireRelease)
{
    AthenaTransportLink *athenaTransportLink = athenaTransportLink_Create("test", _send_method, _receive_method, _close_method);
    assertNotNull(athenaTransportLink, "athenaTransportLink_Create failed");
    AthenaTransportLink *copy = athenaTransportLink_Acquire(athenaTransportLink);
    athenaTransportLink_Release(&athenaTransportLink);
    athenaTransportLink_Release(&copy);
}

LONGBOW_TEST_CASE(Global, athenaTransportLink_Clone)
{
    AthenaTransportLink *athenaTransportLink = athenaTransportLink_Create("test", _send_method, _receive_method, _close_method);
    assertNotNull(athenaTransportLink, "athenaTransportLink_Create failed");
    AthenaTransportLink *athenaTransportLinkClone = athenaTransportLink_Clone(athenaTransportLink,
                                                                              "Link Clone",
                                                                              _send_method,
                                                                              _receive_method,
                                                                              _close_method);
    assertNotNull(athenaTransportLinkClone, "athenaTransportLink_Clone failed");
    athenaTransportLink_Release(&athenaTransportLinkClone);
    athenaTransportLink_Release(&athenaTransportLink);
}

LONGBOW_TEST_CASE(Global, athenaTransportLink_GetLogger)
{
    AthenaTransportLink *athenaTransportLink = athenaTransportLink_Create("test", _send_method, _receive_method, _close_method);
    assertNotNull(athenaTransportLink, "athenaTransportLink_Create failed");
    assertNotNull(athenaTransportLink_GetLogger(athenaTransportLink), "athenaTransportLink_GetLogger failed");
    athenaTransportLink_Release(&athenaTransportLink);
}

LONGBOW_TEST_CASE(Global, athenaTransportLink_SendReceiveClose)
{
    AthenaTransportLink *athenaTransportLink = athenaTransportLink_Create("test", _send_method, _receive_method, _close_method);
    assertNotNull(athenaTransportLink, "athenaTransportLink_Create failed");

    int result = athenaTransportLink_Send(athenaTransportLink, NULL);
    assertTrue(result == 0, "athenaTransportLink_Send failed (%d)", result);

    CCNxMetaMessage *ccnxMetaMessage = athenaTransportLink_Receive(athenaTransportLink);
    assertTrue(ccnxMetaMessage == NULL, "athenaTransportLink_Receive failed");

    athenaTransportLink->linkEvents |= AthenaTransportLinkEvent_Closing;

    result = athenaTransportLink_Send(athenaTransportLink, NULL);
    assertTrue(result == -1, "athenaTransportLink_Send should have failed (%d)", result);

    ccnxMetaMessage = athenaTransportLink_Receive(athenaTransportLink);
    assertTrue(ccnxMetaMessage == NULL, "athenaTransportLink_Receive failed");

    // Calling close on a closing link should have no effect
    athenaTransportLink_Close(athenaTransportLink);

    athenaTransportLink->linkEvents &= ~AthenaTransportLinkEvent_Closing;

    // Marking the link error will cause receive to close the link.
    athenaTransportLink->linkEvents |= AthenaTransportLinkEvent_Error;

    ccnxMetaMessage = athenaTransportLink_Receive(athenaTransportLink);
    assertTrue(ccnxMetaMessage == NULL, "athenaTransportLink_Receive failed");
}

LONGBOW_TEST_CASE(Global, athenaTransportLink_GetName)
{
    AthenaTransportLink *athenaTransportLink = athenaTransportLink_Create("test", _send_method, _receive_method, _close_method);
    assertNotNull(athenaTransportLink, "athenaTransportLink_Create failed");

    const char *name = athenaTransportLink_GetName(athenaTransportLink);
    assertTrue(strcmp("test", name) == 0, "athenaTransportLink_GetName failed (%s != test)", name);

    athenaTransportLink_Release(&athenaTransportLink);
}

LONGBOW_TEST_CASE(Global, athenaTransportLink_GetSetEvent)
{
    AthenaTransportLink *athenaTransportLink = athenaTransportLink_Create("test", _send_method, _receive_method, _close_method);
    assertNotNull(athenaTransportLink, "athenaTransportLink_Create failed");

    athenaTransportLink_SetEvent(athenaTransportLink, AthenaTransportLinkEvent_Closing);

    AthenaTransportLinkEvent event = athenaTransportLink_GetEvent(athenaTransportLink);
    assertTrue(event == AthenaTransportLinkEvent_Closing, "athenaTransportLink_GetEvent failed (%d != %d)",
               event, AthenaTransportLinkEvent_Closing);

    athenaTransportLink_Release(&athenaTransportLink);
}

LONGBOW_TEST_CASE(Global, athenaTransportLink_GetSetPrivateData)
{
    AthenaTransportLink *athenaTransportLink = athenaTransportLink_Create("test", _send_method, _receive_method, _close_method);
    assertNotNull(athenaTransportLink, "athenaTransportLink_Create failed");

    void *privateData = (void *) 0x1;
    athenaTransportLink_SetPrivateData(athenaTransportLink, privateData);

    void *retrievedPrivateData = athenaTransportLink_GetPrivateData(athenaTransportLink);
    assertTrue(privateData == retrievedPrivateData, "athenaTransportLink_GetPrivateData failed (%p != %p)",
               privateData, retrievedPrivateData);

    athenaTransportLink_Release(&athenaTransportLink);
}

static int
_add_link_callback(void *context, AthenaTransportLink *athenaTransportLink)
{
    assertTrue(context == (void *) 0x1, "add link called with unexpected context");
    return 0;
}

static void
_remove_link_callback(void *context, AthenaTransportLink *athenaTransportLink)
{
    assertTrue(context == (void *) 0x1, "remove link called with unexpected context");
}

LONGBOW_TEST_CASE(Global, athenaTransportLink_AddRemoveCallback)
{
    AthenaTransportLink *athenaTransportLink = athenaTransportLink_Create("test", _send_method, _receive_method, _close_method);
    assertNotNull(athenaTransportLink, "athenaTransportLink_Create failed");

    athenaTransportLink_SetAddLinkCallback(athenaTransportLink, _add_link_callback, (void *) 0x1);
    athenaTransportLink_SetRemoveLinkCallback(athenaTransportLink, _remove_link_callback, (void *) 0x1);

    athenaTransportLink_AddLink(athenaTransportLink, NULL);
    athenaTransportLink_RemoveLink(athenaTransportLink);

    athenaTransportLink_Release(&athenaTransportLink);
}

LONGBOW_TEST_CASE(Global, athenaTransportLinkAdapter_SetGetEventFd)
{
    AthenaTransportLink *athenaTransportLink = athenaTransportLink_Create("test", _send_method, _receive_method, _close_method);
    assertNotNull(athenaTransportLink, "athenaTransportLink_Create failed");
    athenaTransportLink_SetEventFd(athenaTransportLink, 3);
    assertTrue(athenaTransportLink_GetEventFd(athenaTransportLink) == 3, "athenaTransportLink_SetEventFd failed");
    athenaTransportLink_Release(&athenaTransportLink);
}

LONGBOW_TEST_CASE(Global, athenaTransportLinkAdapter_Routable)
{
    AthenaTransportLink *athenaTransportLink = athenaTransportLink_Create("test", _send_method, _receive_method, _close_method);
    assertNotNull(athenaTransportLink, "athenaTransportLink_Create failed");
    athenaTransportLink_SetRoutable(athenaTransportLink, true);
    assertFalse(athenaTransportLink_IsNotRoutable(athenaTransportLink), "athenaTransportLink_SetRoutable failed");
    athenaTransportLink_SetRoutable(athenaTransportLink, false);
    assertTrue(athenaTransportLink_IsNotRoutable(athenaTransportLink), "athenaTransportLink_SetRoutable failed");
    athenaTransportLink_Release(&athenaTransportLink);
}

LONGBOW_TEST_CASE(Global, athenaTransportLinkAdapter_IsNotLocal)
{
    AthenaTransportLink *athenaTransportLink = athenaTransportLink_Create("test", _send_method, _receive_method, _close_method);
    assertNotNull(athenaTransportLink, "athenaTransportLink_Create failed");
    athenaTransportLink_SetLocal(athenaTransportLink, true);
    assertFalse(athenaTransportLink_IsNotLocal(athenaTransportLink), "athenaTransportLink_SetLocal failed");
    athenaTransportLink_SetLocal(athenaTransportLink, false);
    assertTrue(athenaTransportLink_IsNotLocal(athenaTransportLink), "athenaTransportLink_SetLocal failed");
    athenaTransportLink_Release(&athenaTransportLink);
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
    LongBowRunner *testRunner = LONGBOW_TEST_RUNNER_CREATE(athena_TransportLink);
    exit(longBowMain(argc, argv, testRunner, NULL));
}
