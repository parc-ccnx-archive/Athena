/*
 * Copyright (c) 2015-2016, Xerox Corporation (Xerox)and Palo Alto Research Center (PARC)
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
 * @copyright 2015-2016, Xerox Corporation (Xerox)and Palo Alto Research Center (PARC).  All rights reserved.
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
#endif // libathenactl_h
