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
#ifndef libathenactl_h
#define libathenactl_h

#include <ccnx/forwarder/athena/athena.h>

#include <ccnx/api/ccnx_Portal/ccnx_Portal.h>
#include <ccnx/api/ccnx_Portal/ccnx_PortalRTA.h>
#include <parc/security/parc_IdentityFile.h>

// included to provide FORWARDER_CONNECTION_ENV
#include <ccnx/transport/transport_rta/config/config_Forwarder_Metis.h>

/**
 * @abstract process a CCNx command line argument
 * @discussion
 *
 * @param [in] identity user identity
 * @param [in] argc number of command arguments
 * @param [in] argv command and arguments
 * @return 0 on success
 *
 * Example:
 * @code
 * {
 *    parcSecurity_Init();
 *
 *    if (keystorePassword == NULL) {
 *        keystorePassword = ccnxKeystoreUtilities_ReadPassword();
 *    }
 *    keystoreParams = ccnxKeystoreUtilities_OpenFile(keystoreFile, keystorePassword);
 *
 *    if (keystoreParams == NULL) {
 *        printf("Could not open or authenticate keystore\n");
 *        exit(1);
 *    }
 *
 *    PARCIdentityFile *identityFile = parcIdentityFile_Create(ccnxKeystoreUtilities_GetFileName(keystoreParams), ccnxKeystoreUtilities_GetPassword(keystoreParams));
 *    if (parcIdentityFile_Exists(identityFile) == false) {
 *        printf("Inaccessible keystore file '%s'.\n", keystoreFile);
 *        exit(1);
 *    }
 *    PARCIdentity *identity = parcIdentity_Create(identityFile, PARCIdentityFileAsPARCIdentity);
 *    parcIdentityFile_Release(&identityFile);
 *
 *    result = athenactl_Command(identity, argc, argv);
 *
 *    keystoreParams_Destroy(&keystoreParams);
 *    parcSecurity_Fini();
 * }
 * @endcode
 */
int athenactl_Command(PARCIdentity *identity, int argc, char **argv);

/**
 * @abstract print athenactl command usage
 * @discussion
 *
 * Example:
 * @code
 * {
 *     athenactl_Usage();
 * }
 * @endcode
 */
void athenactl_Usage(void);

/**
 * @abstract encode message into wire format
 * @discussion
 *
 * @param [in] message
 *
 * Example:
 * @code
 * {
 *     athenactl_EncodeMessage(message);
 * }
 * @endcode
 */
void athenactl_EncodeMessage(CCNxMetaMessage *message);

/**
 * @abstract send a constructed interest control message
 * @discussion
 *
 * @param [in] identity
 * @param [in] message
 * @return result message from forwarder
 *
 * Example:
 * @code
 * {
 * }
 * @endcode
 */
const char *athenactl_SendInterestControl(PARCIdentity *identity, CCNxMetaMessage *message);
#endif // libathenactl_h
