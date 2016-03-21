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
/*
 * Athena Example Control CLI
 */

#include <config.h>

#include <stdlib.h>
#include <getopt.h>
#include <stdio.h>

#include <LongBow/runtime.h>

#include <ccnx/forwarder/athena/athenactl.h>
#include <ccnx/forwarder/athena/athenactl_About.h>

#include <parc/security/parc_Security.h>

#include <ccnx/common/ccnx_KeystoreUtilities.h>

static void
_athenactlLogo()
{
    printf("  ____   _    _                               " "\n");
    printf(" / __ \\ | |_ | |      ___   __        __     " "\n");
    printf("| /__\\ || __|| |___  / _ \\ / /___   __> |   " "\n");
    printf("| |  | || |_ | ___ ||  __/ | ___ \\ / __ \\   " "\n");
    printf("|_|  |_| \\__||_| |_| \\___| |_| |_| \\___/_\\" "\n");
    printf("          ___                 _                 _   _              " "\n");
    printf("         / __\\  ___   __     | |_   _     ___  | | | |   ___   _   " "\n");
    printf("         ||    / _ \\ / /___  |  _| | |_  / _ \\ | | | |  / _ \\ | |_ " "\n");
    printf("         ||__  ||_|| | ___ \\ | |_  |  _\\ ||_|| | | | | |  __/ |  _\\" "\n");
    printf("         \\___/ \\___/ |_| |_|  \\__| |_|   \\___/ |_| |_|  \\___| |_|  " "\n");
}

static void
_usage(void)
{
    _athenactlLogo();
    printf("\n");
    printf("usage: athenactl [-h] [-a <address>] [-f <identity file>] [-p <password>] <command>\n");
    printf("    -a | --address  Forwarder connection address (default: tcp://localhost:9695)\n");
    printf("    -f | --identity  The file name containing a PKCS12 keystore\n");
    printf("    -p | --password  The password to unlock the keystore\n");
    printf("    <command> The forwarder command to execute\n");
}

static char *keystoreFile = NULL;
static char *keystorePassword = NULL;

/* options descriptor */
static struct option longopts[] = {
    { "address",  required_argument, NULL, 'a' },
    { "identity", required_argument, NULL, 'f' },
    { "password", required_argument, NULL, 'p' },
    { "version",  no_argument,       NULL, 'v' },
    { "help",     no_argument,       NULL, 'h' },
    { NULL,       0,                 NULL, 0   }
};

int
main(int argc, char *argv[])
{
    int result;

    int ch;
    while ((ch = getopt_long(argc, argv, "a:f:p:hv", longopts, NULL)) != -1) {
        switch (ch) {
            case 'f':
                keystoreFile = optarg;
                break;

            case 'p':
                keystorePassword = optarg;
                break;

            case 'a':
                setenv(FORWARDER_CONNECTION_ENV, optarg, 1);
                break;

            case 'v':
                printf("%s\n", athenactlAbout_Version());
                exit(0);

            case 'h':
                _usage();
                athenactl_Usage();
                exit(0);

            default:
                _usage();
                exit(1);
        }
    }
    argc -= optind;
    argv += optind;

    KeystoreParams *keystoreParams;

    parcSecurity_Init();

    if (keystorePassword == NULL) {
        keystorePassword = ccnxKeystoreUtilities_ReadPassword();
    }
    keystoreParams = ccnxKeystoreUtilities_OpenFile(keystoreFile, keystorePassword);

    if (keystoreParams == NULL) {
        printf("Could not open or authenticate keystore\n");
        exit(1);
    }

    PARCIdentityFile *identityFile = parcIdentityFile_Create(ccnxKeystoreUtilities_GetFileName(keystoreParams), ccnxKeystoreUtilities_GetPassword(keystoreParams));
    if (parcIdentityFile_Exists(identityFile) == false) {
        printf("Inaccessible keystore file '%s'.\n", keystoreFile);
        exit(1);
    }
    PARCIdentity *identity = parcIdentity_Create(identityFile, PARCIdentityFileAsPARCIdentity);
    parcIdentityFile_Release(&identityFile);

    result = athenactl_Command(identity, argc, argv);

    parcIdentity_Release(&identity);
    keystoreParams_Destroy(&keystoreParams);
    parcSecurity_Fini();

    return (result);
}
