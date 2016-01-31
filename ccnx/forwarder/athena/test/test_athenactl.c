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

#include "../athenactl.c"

#include <LongBow/unit-test.h>

#include <errno.h>
#include <pthread.h>

#include <parc/algol/parc_SafeMemory.h>
#include <parc/security/parc_Security.h>
#include <parc/security/parc_PublicKeySignerPkcs12Store.h>
#include <ccnx/common/ccnx_KeystoreUtilities.h>

LONGBOW_TEST_RUNNER(athenactl)
{
    parcMemory_SetInterface(&PARCSafeMemoryAsPARCMemory);

    LONGBOW_RUN_TEST_FIXTURE(Global);
    LONGBOW_RUN_TEST_FIXTURE(Static);

    LONGBOW_RUN_TEST_FIXTURE(Misc);
}

// The Test Runner calls this function once before any Test Fixtures are run.
LONGBOW_TEST_RUNNER_SETUP(athenactl)
{
    return LONGBOW_STATUS_SUCCEEDED;
}

// The Test Runner calls this function once after all the Test Fixtures are run.
LONGBOW_TEST_RUNNER_TEARDOWN(athenactl)
{
    return LONGBOW_STATUS_SUCCEEDED;
}

LONGBOW_TEST_FIXTURE(Global)
{
    LONGBOW_RUN_TEST_CASE(Global, athenactl_Command);
    LONGBOW_RUN_TEST_CASE(Global, athenactl_Usage);
}

LONGBOW_TEST_FIXTURE_SETUP(Global)
{
    return LONGBOW_STATUS_SUCCEEDED;
}

PARCIdentity *
_create_identity()
{
    unsigned int keyLength = 1024;
    unsigned int validityDays = 30;
    char *subjectName = "test_athenactl";

    bool success =
        parcPublicKeySignerPkcs12Store_CreateFile("my_keystore", "my_keystore_password", subjectName, keyLength, validityDays);
    assertTrue(success, "parcPublicKeySignerPkcs12Store_CreateFile('my_keystore', 'my_keystore_password') failed.");

    PARCIdentityFile *identityFile = parcIdentityFile_Create("my_keystore", "my_keystore_password");
    PARCIdentity *identity = parcIdentity_Create(identityFile, PARCIdentityFileAsPARCIdentity);
    parcIdentityFile_Release(&identityFile);
    return identity;
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

static struct {
    char *argv[7];
    int argc;
    int expectedReturn;
} _argvs[] = {
    {{COMMAND_ADD,    SUBCOMMAND_ADD_LINK,         "tcp://localhost:50500/name=TCP_0", NULL},   3, 0 },
    {{COMMAND_ADD,    SUBCOMMAND_ADD_LINK,         "tcp://localhost:50500/name=TCP_0", NULL},   2, 1 },
    {{COMMAND_ADD,    SUBCOMMAND_ADD_LISTENER,     "tcp", "TCP_0", "localhost", "50600", NULL}, 6, 0 },
    {{COMMAND_ADD,    SUBCOMMAND_ADD_LISTENER,     "tcp", "TCP_0", "localhost", "50600", NULL}, 5, 1 },
    {{COMMAND_ADD,    SUBCOMMAND_ADD_CONNECTION,   "tcp", "TCP_0", "localhost", "50600", NULL}, 6, 0 },
    {{COMMAND_ADD,    SUBCOMMAND_ADD_CONNECTION,   "tcp", "TCP_0", "localhost", "50600", NULL}, 5, 1 },
    {{COMMAND_LIST,   SUBCOMMAND_LIST_LINKS,       NULL},                                       2, 0 },
    {{COMMAND_LIST,   SUBCOMMAND_LIST_LINKS,       NULL},                                       1, 1 },
    {{COMMAND_LIST,   SUBCOMMAND_LIST_CONNECTIONS, NULL},                                       2, 0 },
    {{COMMAND_LIST,   SUBCOMMAND_LIST_CONNECTIONS, NULL},                                       1, 1 },
    {{COMMAND_ADD,    SUBCOMMAND_ADD_ROUTE,        "TCP_0", "lci:/foo/bar", NULL},              4, 0 },
    {{COMMAND_ADD,    SUBCOMMAND_ADD_ROUTE,        "TCP_0", "lci:/foo/bar", NULL},              3, 1 },
    {{COMMAND_ADD,    SUBCOMMAND_ADD_ROUTE,        "TCP_0", "lci:/foo/bar", NULL},              2, 1 },
    {{COMMAND_LIST,   SUBCOMMAND_LIST_FIB,         NULL},                                       2, 0 },
    {{COMMAND_LIST,   SUBCOMMAND_LIST_FIB,         NULL},                                       1, 1 },
    {{COMMAND_REMOVE, SUBCOMMAND_REMOVE_ROUTE,     "TCP_0", "lci:/foo/bar", NULL},              4, 0 },
    {{COMMAND_REMOVE, SUBCOMMAND_REMOVE_ROUTE,     "TCP_0", "lci:/foo/bar", NULL},              3, 1 },
    {{COMMAND_REMOVE, SUBCOMMAND_REMOVE_ROUTE,     "TCP_0", "lci:/foo/bar", NULL},              2, 1 },
    {{COMMAND_REMOVE, SUBCOMMAND_REMOVE_LINK,      "TCP_0", NULL},                              3, 0 },
    {{COMMAND_REMOVE, SUBCOMMAND_REMOVE_LINK,      "TCP_0", NULL},                              2, 1 },
    {{COMMAND_SET,    SUBCOMMAND_SET_DEBUG,        NULL},                                       2, 0 },
    {{COMMAND_SET,    SUBCOMMAND_SET_DEBUG,        NULL},                                       1, 1 },
    {{COMMAND_SET,    SUBCOMMAND_SET_LEVEL,        "debug", NULL},                              3, 0 },
    {{COMMAND_SET,    SUBCOMMAND_SET_LEVEL,        "debug", NULL},                              2, 1 },
    {{COMMAND_UNSET,  SUBCOMMAND_SET_DEBUG,        NULL},                                       2, 0 },
    {{COMMAND_UNSET,  SUBCOMMAND_SET_DEBUG,        NULL},                                       1, 1 },
    {{""},                                                                                      0, 1 },
    {{COMMAND_RUN,    NULL},                                                                    1, 1 },
    {{COMMAND_QUIT,   NULL},                                                                    1, 0 },
    {{NULL},                                                                                    0, 0 },
};

LONGBOW_TEST_CASE(Global, athenactl_Command)
{
    Athena *athena = athena_Create(AthenaDefaultContentStoreSize);
    PARCIdentity *identity;

    if (athena) {
        PARCURI *connectionURI = parcURI_Parse("tcp://localhost:50500/listener");
        if (athenaTransportLinkAdapter_Open(athena->athenaTransportLinkAdapter, connectionURI) == NULL) {
            parcLog_Error(athena->log, "Unable to configure an interface.  Exiting...");
            parcURI_Release(&connectionURI);
        }
        parcURI_Release(&connectionURI);

        pthread_t thread;
        // pass a new reference that will be released by the new thread
        int result = pthread_create(&thread, NULL, athena_ForwarderEngine, (void *) athena_Acquire(athena));
        assertTrue(result == 0, "pthread_create failed to create athena_ForwarderEngine");

        result = setenv(FORWARDER_CONNECTION_ENV, "tcp://localhost:50500", 1);
        assertTrue(result == 0, "setenv of %s failed", FORWARDER_CONNECTION_ENV);

        int index = 0;
        while (_argvs[index].argv[0]) {
            parcSecurity_Init();
            identity = _create_identity();
            result = athenactl_Command(identity, _argvs[index].argc, _argvs[index].argv);
            parcIdentity_Release(&identity);
            assertTrue(result == _argvs[index].expectedReturn, "athenactl_Command failed");
            parcSecurity_Fini();
            index++;
        }

        athena_Release(&athena);
        pthread_join(thread, NULL);
    }
}

LONGBOW_TEST_CASE(Global, athenactl_Usage)
{
    athenactl_Usage();
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
    LongBowRunner *testRunner = LONGBOW_TEST_RUNNER_CREATE(athenactl);
    int exitStatus = longBowMain(argc, argv, testRunner, NULL);
    longBowTestRunner_Destroy(&testRunner);
    exit(exitStatus);
}
