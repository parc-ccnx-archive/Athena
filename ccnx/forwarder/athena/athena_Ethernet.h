/*
 * Copyright (c) 2015-2016, Xerox Corporation (Xerox) and Palo Alto Research Center, Inc (PARC)
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
 * @copyright (c) 2015-2016, Xerox Corporation (Xerox) and Palo Alto Research Center, Inc (PARC).  All rights reserved.
 */
#ifndef libathena_Ethernet_h
#define libathena_Ethernet_h

#define CCNX_ETHERTYPE 0x0801

/**
 * @typedef AthenaEthernet
 * @brief private data for Athena ethernet module
 */
struct AthenaEthernet;
typedef struct AthenaEthernet AthenaEthernet;

/**
 * @abstract create an Athena ethernet instance
 * @discussion
 *
 * @param [in] log
 * @param [in] interface name
 * @param [in] etherType of messages sent through
 * @return athenaEthernet instance
 *
 * Example:
 * @code
 * {
 *     AthenaEthernet *athenaEthernet = athenaEthernet_Create(log, "eth0", CCNX_ETHERTYPE);
 *     ...
 *     athenaEthernet_Release(&athenaEthernet);
 * }
 * @endcode
 */
AthenaEthernet *athenaEthernet_Create(PARCLog *log, const char *interface, uint16_t etherType);

/**
 * @abstract release an Athena ethernet instance
 * @discussion
 *
 * @param [in] athenaEthernet instance
 *
 * Example:
 * @code
 * {
 *     AthenaEthernet *athenaEthernet = athenaEthernet_Create(log, "eth0", CCNX_ETHERTYPE);
 *     ...
 *     athenaEthernet_Release(&athenaEthernet);
 * }
 * @endcode
 */
void athenaEthernet_Release(AthenaEthernet **athenaEthernet);

/**
 * @abstract acquire a reference to an Athena ethernet instance
 * @discussion
 *
 * @param [in] athenaEthernet instance
 * @return new athenaEthernet reference
 *
 * Example:
 * @code
 * {
 *     AthenaEthernet *athenaEthernet = athenaEthernet_Create(log, "eth0", CCNX_ETHERTYPE);
 *     AthenaEthernet *reference = athenaEthernet_Acquire(athenaEthernet);
 *     ...
 *     athenaEthernet_Release(&athenaEthernet);
 *     athenaEthernet_Release(&reference);
 * }
 * @endcode
 */
AthenaEthernet *athenaEthernet_Acquire(const AthenaEthernet *athenaEthernet);

/**
 * @abstract return the current MTU size of an ethernet instance
 * @discussion
 *
 * @param [in] athenaEthernet instance
 * @return MTU size
 *
 * Example:
 * @code
 * {
 *     AthenaEthernet *athenaEthernet = athenaEthernet_Create(log, "eth0", CCNX_ETHERTYPE);
 *     uint32_t mtu = athenaEthernet_GetMtu(athenaEthernet);
 *     athenaEthernet_Release(&athenaEthernet);
 * }
 * @endcode
 */
uint32_t athenaEthernet_GetMTU(AthenaEthernet *athenaEthernet);

/**
 * @abstract obtain the MAC address assigned to an Athena Ethernet instance
 * @discussion
 *
 * @param [in] athenaEthernet instance
 * @param [in] address
 *
 * Example:
 * @code
 * {
 *     AthenaEthernet *athenaEthernet = athenaEthernet_Create(log, "eth0", CCNX_ETHERTYPE);
 *     struct ether_addr address;
 *     uint32_t mtu = athenaEthernet_GetMAC(athenaEthernet, &address);
 *     athenaEthernet_Release(&athenaEthernet);
 * }
 * @endcode
 */
void athenaEthernet_GetMAC(AthenaEthernet *athenaEthernet, struct ether_addr *address);

/**
 * @abstract obtain the MAC address associated with an interface
 * @discussion
 *
 * @param [in] device name
 * @param [in] address
 * @return 0 on success
 *
 * Example:
 * @code
 * {
 *     struct ether_addr address;
 *     int result = athenaEthernet_GetInterfaceMAC("eth0", &address);
 * }
 * @endcode
 */
int athenaEthernet_GetInterfaceMAC(const char *device, struct ether_addr *ether_addr);

/**
 * @abstract return the ether type associated with a Athena Ethernet interface
 * @discussion
 *
 * @param [in] athenaEthernet instance
 * @return ether type
 *
 * Example:
 * @code
 * {
 *     AthenaEthernet *athenaEthernet = athenaEthernet_Create(log, "eth0", CCNX_ETHERTYPE);
 *     uint16_t etherType = athenaEthernet_GetEtherType(athenaEthernet);
 *     assertTrue(etherType == CCNX_ETHERTYPE, "ether type was changed");
 *     athenaEthernet_Release(&athenaEthernet);
 * }
 * @endcode
 */
uint16_t athenaEthernet_GetEtherType(AthenaEthernet *athenaEthernet);

/**
 * @abstract receive a packet on an Athena Ethernet instance
 * @discussion
 *
 * @param [in] athenaEthernet instance
 * @param [in] timeout value in ms
 * @param [in] events still outstanding
 * @return PARCBuffer containing message
 *
 * Example:
 * @code
 * {
 *     AthenaTransportLinkEvent events = 0;
 *     AthenaEthernet *athenaEthernet = athenaEthernet_Create(log, "eth0", CCNX_ETHERTYPE);
 *     PARCBuffer *buffer = athenaEthernet_Receive(athenaEthernet, -1, &events);
 *     athenaEthernet_Release(&athenaEthernet);
 * }
 * @endcode
 */
PARCBuffer *athenaEthernet_Receive(AthenaEthernet *athenaEthernet, int timeout, AthenaTransportLinkEvent *events);

/**
 * @abstract send an io vector on an Athena Ethernet instance
 * @discussion
 *
 * @param [in] athenaEthernet instance
 * @param [in] iov vector to write
 * @param [in] iovcnt vector count
 * @return number of bytes sent, -1 on error with errno set
 *
 * Example:
 * @code
 * {
 *     AthenaEthernet *athenaEthernet = athenaEthernet_Create(log, "eth0", CCNX_ETHERTYPE);
 *     struct iovec iov;
 *     iov.iov_base = "foo";
 *     iov.iov_len = strlen(foo) + 1;
 *
 *     int result = athenaEthernet_Send(athenaEthernet, &iov, 1);
 *
 *     athenaEthernet_Release(&athenaEthernet);
 * }
 * @endcode
 */
ssize_t athenaEthernet_Send(AthenaEthernet *athenaEthernet, struct iovec *iov, int iovcnt);

/**
 * @abstract return the file descriptor associated with an Athena Ethernet instance
 * @discussion
 *
 * @param [in] athenaEthernet instance
 * @return file descriptor, -1 on error or no descriptor
 *
 * Example:
 * @code
 * {
 *     AthenaEthernet *athenaEthernet = athenaEthernet_Create(log, "eth0", CCNX_ETHERTYPE);
 *     int fd = athenaEthernet_GetDescriptor(athenaEthernet);
 *     athenaEthernet_Release(&athenaEthernet);
 * }
 * @endcode
 */
int athenaEthernet_GetDescriptor(AthenaEthernet *athenaEthernet);

/**
 * @abstract return the platform device name for the specified instance
 * @discussion
 *
 * @param [in] athenaEthernet instance
 * @return string containing name of interface
 *
 * Example:
 * @code
 * {
 *     const char *interfaceName = athenaEthernet_GetName(athenaEthernet);
 * }
 * @endcode
 */
const char *athenaEthernet_GetName(AthenaEthernet *athenaEthernet);

#endif // libathena_Ethernet_h
