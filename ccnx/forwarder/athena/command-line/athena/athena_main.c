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
/*
 * Athena Example Runtime
 */

#include <config.h>

#include <stdio.h>
#include <pthread.h>
#include <getopt.h>
#include <netdb.h>
#include <errno.h>
#include <sys/param.h>
#include <sys/utsname.h>
#include <fcntl.h>
#include <ctype.h>

#include <ccnx/forwarder/athena/athena.h>
#include <ccnx/forwarder/athena/athena_About.h>
#include <ccnx/forwarder/athena/athena_InterestControl.h>

static char *_athenaDefaultConnectionURI = AthenaDefaultConnectionURI;
static size_t _contentStoreSizeInMB = AthenaDefaultContentStoreSize;

static void
_athenaLogo()
{
    printf("            ____   _    _                           " "\n");
    printf("           / __ \\ | |_ | |      ___   __        __  " "\n");
    printf("          | /__\\ || __|| |___  / _ \\ / /___   __> | " "\n");
    printf("          | |  | || |_ | ___ ||  __/ | ___ \\ / __ \\ " "\n");
    printf("          |_|  |_| \\__||_| |_| \\___| |_| |_| \\___/_\\" "\n");
}

static void
_usage()
{
    printf("usage: athena [-c <protocol>://<address>:<port>[/listener][/name=<name>][/local=<bool>]] [-s storeSize] [-o stateFile] [-d]\n");
    printf("    -c | --connect    Transport link specification to create\n");
    printf("    -s | --store      Size of the content store in mega bytes\n");
    printf("    -o | --statefile  File to contain configuration state changes\n");
    printf("    -i | --config     File containing configuration commands\n");
    printf("    -d | --debug      Turn on debug logging\n");
}

static struct option options[] = {
    { .name = "store",     .has_arg = required_argument, .flag = NULL, .val = 's' },
    { .name = "connect",   .has_arg = required_argument, .flag = NULL, .val = 'c' },
    { .name = "config",    .has_arg = required_argument, .flag = NULL, .val = 'i' },
    { .name = "statefile", .has_arg = required_argument, .flag = NULL, .val = 'o' },
    { .name = "help",      .has_arg = no_argument,       .flag = NULL, .val = 'h' },
    { .name = "version",   .has_arg = no_argument,       .flag = NULL, .val = 'v' },
    { .name = "debug",     .has_arg = no_argument,       .flag = NULL, .val = 'd' },
    { .name = NULL,        .has_arg = 0,                 .flag = NULL, .val = 0   },
};

static int
_parseConfigurationFile(Athena *athena, const char *configurationFile)
{
    char configLine[MAXPATHLEN];
    char *interestName = NULL;
    char *interestPayload = NULL;
    int lineNumber = 0;

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
        if ((*commandPtr == '#') || (*commandPtr == '\0') ) { // Commented line
            continue;
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
        if (name == NULL) {
            printf("Could not parse %s\n", interestName);
            continue;
        }
        CCNxInterest *interest = ccnxInterest_CreateSimple(name);
        ccnxName_Release(&name);

        if (interestPayload) {
            PARCBuffer *payload = parcBuffer_AllocateCString(interestPayload);
            ccnxInterest_SetPayload(interest, payload);
            parcBuffer_Release(&payload);
        }
        printf("[%d] %s %s\n", lineNumber++, interestName, interestPayload);
        PARCBitVector *ingress = parcBitVector_Create();
        athena_ProcessMessage(athena, interest, ingress);
        parcBitVector_Release(&ingress);
    }
    fclose(input);
    return 0;
}

static void
_parseCommandLine(Athena *athena, int argc, char **argv)
{
    int c;
    bool interfaceConfigured = false;
    char *configurationFile = NULL;

    while ((c = getopt_long(argc, argv, "hs:i:c:o:vd", options, NULL)) != -1) {
        switch (c) {
            case 's': {
                int sizeInMB = atoi(optarg);
                if (athenaContentStore_SetCapacity(athena->athenaContentStore, sizeInMB) != true) {
                    parcLog_Error(athena->log, "Unable to resize content store to %d (MB)", sizeInMB);
                }
                _contentStoreSizeInMB = sizeInMB;
                break;
            }
            case 'c': {
                PARCURI *connectionURI = parcURI_Parse(optarg);
                const char *result = athenaTransportLinkAdapter_Open(athena->athenaTransportLinkAdapter, connectionURI);
                if (result == NULL) {
                    parcLog_Error(athena->log, "Unable to configure %s: %s", optarg, strerror(errno));
                    parcURI_Release(&connectionURI);
                    exit(EXIT_FAILURE);
                }
                CCNxName *addLink = ccnxName_CreateFromCString(CCNxNameAthenaCommand_LinkConnect);
                athenaInterestControl_LogConfigurationChange(athena, addLink, "%s", optarg);
                ccnxName_Release(&addLink);
                parcURI_Release(&connectionURI);
                interfaceConfigured = true;
                break;
            }
            case 'o': {
                const char *stateFile = optarg;
                int fd = open(stateFile, O_CREAT|O_WRONLY|O_TRUNC, 0600);
                if (fd < 0) {
                    parcLog_Error(athena->log, "Unable to open %s: %s", stateFile, strerror(errno));
                    exit(EXIT_FAILURE);
                }
                PARCFileOutputStream *fileOutputStream = parcFileOutputStream_Create(fd);
                assertNotNull(fileOutputStream, "File output stream failed (%s)", stateFile);
                athena->configurationLog = parcFileOutputStream_AsOutputStream(fileOutputStream);
                parcFileOutputStream_Release(&fileOutputStream);
                break;
            }
            case 'v':
                printf("%s\n", athenaAbout_Version());
                exit(0);
            case 'd':
                athenaTransportLinkAdapter_SetLogLevel(athena->athenaTransportLinkAdapter, PARCLogLevel_Debug);
                parcLog_SetLevel(athena->log, PARCLogLevel_Debug);
                break;
            case 'i':
                configurationFile = optarg;
                break;
            case 'h':
            default:
                _usage();
                exit(EXIT_FAILURE);
                break;
        }
    }

    if (argc - optind) {
        parcLog_Error(athena->log, "Bad arguments");
        _usage();
        exit(EXIT_FAILURE);
    }

    if (configurationFile) {
        if (_parseConfigurationFile(athena, configurationFile) != 0) {
             exit(1);
        }
        return;
    }

    if (interfaceConfigured != true) {
        PARCURI *connectionURI = parcURI_Parse(_athenaDefaultConnectionURI);
        if (athenaTransportLinkAdapter_Open(athena->athenaTransportLinkAdapter, connectionURI) == NULL) {
            parcLog_Error(athena->log, "Unable to configure an interface.  Exiting...");
            parcURI_Release(&connectionURI);
            exit(EXIT_FAILURE);
        }
        CCNxName *addLink = ccnxName_CreateFromCString(CCNxNameAthenaCommand_LinkConnect);
        athenaInterestControl_LogConfigurationChange(athena, addLink, "%s", _athenaDefaultConnectionURI);
        ccnxName_Release(&addLink);
        parcURI_Release(&connectionURI);
        struct utsname name;
        if (uname(&name) == 0) {
            char nodeURIspecification[MAXPATHLEN];
            sprintf(nodeURIspecification, "tcp://%s:%d/listener",
                    name.nodename, AthenaDefaultListenerPort);
            PARCURI *nodeURI = parcURI_Parse(nodeURIspecification);
            if (athenaTransportLinkAdapter_Open(athena->athenaTransportLinkAdapter, nodeURI) == NULL) {
                parcURI_Release(&nodeURI);
            } else {
                CCNxName *addLink = ccnxName_CreateFromCString(CCNxNameAthenaCommand_LinkConnect);
                athenaInterestControl_LogConfigurationChange(athena, addLink, "%s", nodeURIspecification);
                ccnxName_Release(&addLink);
            }
        }
    }
}

int
main(int argc, char *argv[])
{
    _athenaLogo();
    printf("\n");

    Athena *athena = athena_Create(AthenaDefaultContentStoreSize);

    // Passing in a reference that will be released by athena_Process.  athena_Process is used
    // in athena_InterestControl.c:_Control_Command as the entry point for spawned instances.
    // Spawned instances may not have have time to acquire a reference before our reference is
    // released so the reference is acquired for them.
    if (athena) {
        _parseCommandLine(athena, argc, argv);
        (void) athena_ForwarderEngine(athena_Acquire(athena));
    }
    athena_Release(&athena);
    pthread_exit(NULL); // wait for any residual threads to exit

    return 0;
}
