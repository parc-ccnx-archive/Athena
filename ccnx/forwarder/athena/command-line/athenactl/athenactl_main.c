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

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <errno.h>
#include <ctype.h>
#include <sys/param.h>

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
    printf("    -i | --config  File containing configuration commands\n");
    printf("    <command> The forwarder command to execute\n");
}

static char *keystoreFile = NULL;
static char *keystorePassword = NULL;

/* options descriptor */
static struct option longopts[] = {
    { "address",  required_argument, NULL, 'a' },
    { "identity", required_argument, NULL, 'f' },
    { "config",   required_argument, NULL, 'i' },
    { "password", required_argument, NULL, 'p' },
    { "version",  no_argument,       NULL, 'v' },
    { "help",     no_argument,       NULL, 'h' },
    { NULL,       0,                 NULL, 0   }
};

static int
_parseConfigurationFile(PARCIdentity *identity, const char *configurationFile)
{
    char configLine[MAXPATHLEN];
    char *interestName = NULL;
    char *interestPayload = NULL;
    int lineNumber = 0;
    const char *result = 0;

    FILE *input = fopen(configurationFile, "r");
    if (input == NULL) {
        printf("Could not open %s: %s\n", configurationFile, strerror(errno));
        return -1;
    }

    while (fgets(configLine, MAXPATHLEN, input)) {
        char *commandPtr = configLine;

        // Skip initial white space, if any
        while (isspace(*commandPtr)) {
            commandPtr++;
        }
        interestName = commandPtr;

        // Scan and terminate the interest url
        while (*commandPtr && (!isspace(*commandPtr))) {
            commandPtr++;
        }

        // Scan and terminate the payload, if any
        if (*commandPtr) {
            *commandPtr++ = '\0';
            interestPayload = commandPtr;
        }
        while (*commandPtr && (*commandPtr != '\n')) {
            commandPtr++;
        }
        *commandPtr = '\0';

        // Run the command
        CCNxName *name = ccnxName_CreateFromCString(interestName);
        CCNxInterest *interest = ccnxInterest_CreateSimple(name);
        ccnxName_Release(&name);

        if (interestPayload) {
            PARCBuffer *payload = parcBuffer_AllocateCString(interestPayload);
            ccnxInterest_SetPayload(interest, payload);
            parcBuffer_Release(&payload);
        }
        printf("[%d] %s %s\n", lineNumber++, interestName, interestPayload);
        result = athenactl_SendInterestControl(identity, interest);
        if (result) {
            printf("%s\n", result);
        }
    }
    fclose(input);
    return 0;
}

int
main(int argc, char *argv[])
{
    int result = 0;
    char *configurationFile = NULL;

    int ch;
    while ((ch = getopt_long(argc, argv, "a:f:i:p:hv", longopts, NULL)) != -1) {
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

            case 'i':
                configurationFile = optarg;
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

    if (configurationFile) {
        if (_parseConfigurationFile(identity, configurationFile) != 0) {
             exit(1);
        }
    }

    if (argc > 0) {
        result = athenactl_Command(identity, argc, argv);
    }

    parcIdentity_Release(&identity);
    keystoreParams_Destroy(&keystoreParams);
    parcSecurity_Fini();

    return (result);
}
