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
#include "../athena_Ethernet.c"
#include <LongBow/unit-test.h>

#include <sys/socket.h>

#include <parc/algol/parc_SafeMemory.h>
#include <parc/algol/parc_Network.h>
#include <ccnx/forwarder/athena/athena.h>
#include <ccnx/forwarder/athena/athena_TransportLinkAdapter.h>

LONGBOW_TEST_RUNNER(athena_Ethernet)
{
    parcMemory_SetInterface(&PARCSafeMemoryAsPARCMemory);
    LONGBOW_RUN_TEST_FIXTURE(Global);
    LONGBOW_RUN_TEST_FIXTURE(Local);
}

/*
 * If we cannot open a raw socket, we cannot run any of these tests.
 */
static bool
_checkForDeviceAvailability(void)
{
    bool result = false;
    int fd = _open_bpf_device();
    if (fd > 0) {
        result = true;
        close(fd);
    }

    return result;
}

LONGBOW_TEST_RUNNER_SETUP(athena_Ethernet)
{
    if (_checkForDeviceAvailability() == true) {
        return LONGBOW_STATUS_SUCCEEDED;
    } else {
        exit(77);
        return LONGBOW_STATUS_SETUP_SKIPTESTS;
    }
}

LONGBOW_TEST_RUNNER_TEARDOWN(athena_Ethernet)
{
    return LONGBOW_STATUS_SUCCEEDED;
}

LONGBOW_TEST_FIXTURE(Global)
{
    LONGBOW_RUN_TEST_CASE(Global, athenaEthernet_CreateRelease);
    LONGBOW_RUN_TEST_CASE(Global, athenaEthernet_MacMtu);
    LONGBOW_RUN_TEST_CASE(Global, athenaEthernet_SendReceive);
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
    AthenaTransportLinkAdapter *athenaTransportLinkAdapter = (AthenaTransportLinkAdapter *) context;
}

static char *
_getInterfaceByName(void)
{
    // Lookup the MAC address of an interface that is up, then ask for it.  Don't use loopback.
    struct ifaddrs *ifaddr;
    int failure = getifaddrs(&ifaddr);
    assertFalse(failure, "Error getifaddrs: (%d) %s", errno, strerror(errno));

    char *ifname = NULL;

    struct ifaddrs *next;
    for (next = ifaddr; next != NULL && ifname == NULL; next = next->ifa_next) {
        if ((next->ifa_addr == NULL) || ((next->ifa_flags & IFF_UP) == 0)) {
            continue;
        }

        if (next->ifa_flags & IFF_LOOPBACK) {
            continue;
        }

        if (next->ifa_addr->sa_family == AF_INET) {
            ifname = strdup(next->ifa_name);
        }
    }
    freeifaddrs(ifaddr);
    return ifname;
}

#include <parc/algol/parc_FileOutputStream.h>
#include <parc/logging/parc_Log.h>
#include <parc/logging/parc_LogReporterTextStdout.h>

static PARCLog *
_parc_logger_create(void)
{
    PARCLogReporter *reporter = parcLogReporterTextStdout_Create();
    PARCLog *log = parcLog_Create("localhost", "athenaEthernet", NULL, reporter);
    parcLogReporter_Release(&reporter);

    parcLog_SetLevel(log, PARCLogLevel_Info);
    return log;
}

LONGBOW_TEST_CASE(Global, athenaEthernet_CreateRelease)
{
    const char *interface = _getInterfaceByName();
    assertNotNull(interface, "Could not obtain a test interface");

    PARCLog *log = _parc_logger_create();

    AthenaEthernet *athenaEthernet = athenaEthernet_Create(log, interface, CCNX_ETHERTYPE);
    AthenaEthernet *reference = athenaEthernet_Acquire(athenaEthernet);
    athenaEthernet_Release(&reference);
    parcLog_Release(&log);
    athenaEthernet_Release(&athenaEthernet);
}

LONGBOW_TEST_CASE(Global, athenaEthernet_MacMtu)
{
    const char *interface = _getInterfaceByName();
    assertNotNull(interface, "Could not obtain a test interface");

    PARCLog *log = _parc_logger_create();

    AthenaEthernet *athenaEthernet = athenaEthernet_Create(log, interface, CCNX_ETHERTYPE);

    uint32_t mtu = athenaEthernet_GetMTU(athenaEthernet);
    assertTrue(mtu != 0, "athenaEthernet_GetMTU returned 0");

    struct ether_addr address = { -1 };
    athenaEthernet_GetMAC(athenaEthernet, &address);
    assertTrue(address.ether_addr_octet[0] != 0xff, "athenaEthernet_GetMAC failed");

    address.ether_addr_octet[0] = 0xff;
    int result = athenaEthernet_GetInterfaceMAC(interface, &address);
    assertTrue(result != -1, "athenaEthernet_GetInterfaceMAC failed to find device");
    assertTrue(address.ether_addr_octet[0] != 0xff, "athenaEthernet_GetInterfaceMAC failed");

    uint16_t etherType = athenaEthernet_GetEtherType(athenaEthernet);
    assertTrue(etherType == CCNX_ETHERTYPE, "athenaEthernet_GetEtherType failed");

    int descriptor = athenaEthernet_GetDescriptor(athenaEthernet);
    assertTrue(descriptor >= 0, "athenaEthernet_GetDescriptor failed");

    parcLog_Release(&log);
    athenaEthernet_Release(&athenaEthernet);
}

LONGBOW_TEST_CASE(Global, athenaEthernet_SendReceive)
{
    const char *interface = _getInterfaceByName();
    assertNotNull(interface, "Could not obtain a test interface");

    PARCLog *log = _parc_logger_create();

    AthenaEthernet *athenaEthernet = athenaEthernet_Create(log, interface, CCNX_ETHERTYPE);

    struct ether_header header = { 0 };
    memset(header.ether_shost, 0, ETHER_ADDR_LEN * sizeof(uint8_t));
    memset(header.ether_dhost, 0, ETHER_ADDR_LEN * sizeof(uint8_t));
    header.ether_type = htons(athenaEthernet_GetEtherType(athenaEthernet));

    struct iovec iov[2];
    iov[0].iov_base = &header;
    iov[0].iov_len = sizeof(struct ether_header);
    iov[1].iov_base = "this is a test message that should be sufficiently large";
    iov[1].iov_len = strlen(iov[1].iov_base) + 1;
    ssize_t writeCount = athenaEthernet_Send(athenaEthernet, iov, 2);
    assertTrue(writeCount == iov[0].iov_len + iov[1].iov_len, "athenaEthernet_Send write failed");

    writeCount = athenaEthernet_Send(athenaEthernet, iov, 2);
    assertTrue(writeCount == iov[0].iov_len + iov[1].iov_len, "athenaEthernet_Send write failed");

    usleep(10000);

    AthenaTransportLinkEvent events = 0;
    PARCBuffer *buffer = athenaEthernet_Receive(athenaEthernet, 0, &events);
    assertNotNull(buffer, "athenaEthernet_Receive failed");
    assertTrue(events != 0, "athenaEthernet_Receive failed to detect second write");
    parcBuffer_Release(&buffer);

    events = 0;
    buffer = athenaEthernet_Receive(athenaEthernet, 0, &events);
    assertNotNull(buffer, "athenaEthernet_Receive failed");
    assertTrue(events == 0, "athenaEthernet_Receive received more messages than sent");
    parcBuffer_Release(&buffer);

    parcLog_Release(&log);
    athenaEthernet_Release(&athenaEthernet);
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
    LongBowRunner *testRunner = LONGBOW_TEST_RUNNER_CREATE(athena_Ethernet);
    exit(longBowMain(argc, argv, testRunner, NULL));
}
