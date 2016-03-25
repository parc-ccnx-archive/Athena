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
 * Athena Example Control CLI implementation
 */

#include <config.h>

#include <sys/param.h>
#include <stdio.h>

#include "athenactl.h"
#include "athena_InterestControl.h"

#include <ccnx/common/validation/ccnxValidation_CRC32C.h>
#include <ccnx/common/codec/ccnxCodec_TlvPacket.h>

#define COMMAND_QUIT "quit"
#define COMMAND_RUN "spawn"

#define COMMAND_SET "set"
#define SUBCOMMAND_SET_DEBUG "debug"
#define COMMAND_UNSET "unset"
#define SUBCOMMAND_UNSET_DEBUG "debug"

#define SUBCOMMAND_SET_LEVEL "level"

#define COMMAND_ADD "add"
#define SUBCOMMAND_ADD_LINK "link"
#define SUBCOMMAND_ADD_CONNECTION "connection"
#define SUBCOMMAND_ADD_LISTENER "listener"
#define SUBCOMMAND_ADD_ROUTE "route"

#define COMMAND_LIST "list"
#define SUBCOMMAND_LIST_LINKS "links"
#define SUBCOMMAND_LIST_FIB "fib"
#define SUBCOMMAND_LIST_ROUTES "routes"
#define SUBCOMMAND_LIST_CONNECTIONS "connections"

#define COMMAND_REMOVE "remove"
#define SUBCOMMAND_REMOVE_LINK "link"
#define SUBCOMMAND_REMOVE_CONNECTION "connection"
#define SUBCOMMAND_REMOVE_ROUTE "route"

void
athenactl_EncodeMessage(CCNxMetaMessage *message)
{
    PARCSigner *signer = ccnxValidationCRC32C_CreateSigner();
    CCNxCodecNetworkBufferIoVec *iovec = ccnxCodecTlvPacket_DictionaryEncode(message, signer);
    ccnxWireFormatMessage_PutIoVec(message, iovec);
    ccnxCodecNetworkBufferIoVec_Release(&iovec);
    parcSigner_Release(&signer);
}

const char *
athenactl_SendInterestControl(PARCIdentity *identity, CCNxMetaMessage *message)
{
    const char *result = NULL;
    CCNxPortalFactory *factory = ccnxPortalFactory_Create(identity);

    CCNxPortal *portal = ccnxPortalFactory_CreatePortal(factory, ccnxPortalRTA_Message);

    assertNotNull(portal, "Expected a non-null CCNxPortal pointer.");

    athenactl_EncodeMessage(message);

    if (ccnxPortal_Send(portal, message, CCNxStackTimeout_Never)) {
        while (ccnxPortal_IsError(portal) == false) {
            CCNxMetaMessage *response = ccnxPortal_Receive(portal, CCNxStackTimeout_Never);
            if (response != NULL) {
                if (ccnxMetaMessage_IsContentObject(response)) {
                    CCNxContentObject *contentObject = ccnxMetaMessage_GetContentObject(response);

                    PARCBuffer *payload = ccnxContentObject_GetPayload(contentObject);

                    if (payload) {
                        result = parcBuffer_ToString(payload);
                    }
                }
                ccnxMetaMessage_Release(&response);
                break;
            }
        }
    }

    ccnxPortal_Release(&portal);

    ccnxPortalFactory_Release(&factory);
    return result;
}

static int
_athenactl_AddListener(PARCIdentity *identity, int argc, char **argv)
{
    if (argc < 4) {
        printf("usage: add listener <protocol> <name> <address> <port>\n");
        return 1;
    }

    CCNxName *name = ccnxName_CreateFromCString(CCNxNameAthenaCommand_LinkConnect);
    CCNxInterest *interest = ccnxInterest_CreateSimple(name);
    ccnxName_Release(&name);

    char linkSpecification[MAXPATHLEN];

    // Metis compatibility
    // ex: add listener tcp local0 127.0.0.1 9695
    if (argv[1][0] != '\0') {
        sprintf(linkSpecification, "%s://%s:%s/name=%s/listener", argv[0], argv[2], argv[3], argv[1]);
    } else {
        sprintf(linkSpecification, "%s://%s:%s/listener", argv[0], argv[2], argv[3]);
    }

    PARCBuffer *payload = parcBuffer_AllocateCString(linkSpecification);
    ccnxInterest_SetPayload(interest, payload);
    parcBuffer_Release(&payload);

    const char *result = athenactl_SendInterestControl(identity, interest);
    if (result) {
        printf("Link: %s\n", result);
        parcMemory_Deallocate(&result);
    }

    ccnxMetaMessage_Release(&interest);

    return 0;
}

static int
_athenactl_AddConnection(PARCIdentity *identity, int argc, char **argv)
{
    if (argc < 4) {
        printf("usage: add connection <protocol> <name> <address> <port>\n");
        return 1;
    }

    CCNxName *name = ccnxName_CreateFromCString(CCNxNameAthenaCommand_LinkConnect);
    CCNxInterest *interest = ccnxInterest_CreateSimple(name);
    ccnxName_Release(&name);

    char linkSpecification[MAXPATHLEN];

    // Metis compatibility
    // ex: add connection udp conn1 ccnx.example.com 9695
    if (argv[1][0] != '\0') {
        sprintf(linkSpecification, "%s://%s:%s/name=%s", argv[0], argv[2], argv[3], argv[1]);
    } else {
        sprintf(linkSpecification, "%s://%s:%s", argv[0], argv[2], argv[3]);
    }

    PARCBuffer *payload = parcBuffer_AllocateCString(linkSpecification);
    ccnxInterest_SetPayload(interest, payload);
    parcBuffer_Release(&payload);

    const char *result = athenactl_SendInterestControl(identity, interest);
    if (result) {
        printf("Link: %s\n", result);
        parcMemory_Deallocate(&result);
    }

    ccnxMetaMessage_Release(&interest);

    return 0;
}

static int
_athenactl_AddLink(PARCIdentity *identity, int argc, char **argv)
{
    if (argc < 1) {
        printf("usage: add link <link specification>\n");
        return 1;
    }

    CCNxName *name = ccnxName_CreateFromCString(CCNxNameAthenaCommand_LinkConnect);
    CCNxInterest *interest = ccnxInterest_CreateSimple(name);
    ccnxName_Release(&name);

    char *linkSpecification = argv[0];

    PARCBuffer *payload = parcBuffer_AllocateCString(linkSpecification);
    ccnxInterest_SetPayload(interest, payload);
    parcBuffer_Release(&payload);

    const char *result = athenactl_SendInterestControl(identity, interest);
    if (result) {
        printf("Link: %s\n", result);
        parcMemory_Deallocate(&result);
    }

    ccnxMetaMessage_Release(&interest);

    return 0;
}

static int
_athenactl_AddRoute(PARCIdentity *identity, int argc, char **argv)
{
    if (argc < 2) {
        printf("usage: add route <linkName> <prefix>\n");
        return 1;
    }

    CCNxName *name = ccnxName_CreateFromCString(CCNxNameAthenaCommand_FIBAddRoute);
    CCNxInterest *interest = ccnxInterest_CreateSimple(name);
    ccnxName_Release(&name);

    char *linkName = argv[0];
    char *prefix = argv[1];

    // passed in as <linkName> <prefix>, passed on as <prefix> <linkname>
    char routeArguments[MAXPATHLEN];
    sprintf(routeArguments, "%s %s", prefix, linkName);
    PARCBuffer *payload = parcBuffer_AllocateCString(routeArguments);
    ccnxInterest_SetPayload(interest, payload);
    parcBuffer_Release(&payload);

    const char *result = athenactl_SendInterestControl(identity, interest);
    if (result) {
        printf("FIB: %s\n", result);
        parcMemory_Deallocate(&result);
    }

    ccnxMetaMessage_Release(&interest);

    return 0;
}

static int
_athenactl_RemoveRoute(PARCIdentity *identity, int argc, char **argv)
{
    if (argc < 2) {
        printf("usage: remove route <linkName> <prefix>\n");
        return 1;
    }

    CCNxName *name = ccnxName_CreateFromCString(CCNxNameAthenaCommand_FIBRemoveRoute);
    CCNxInterest *interest = ccnxInterest_CreateSimple(name);
    ccnxName_Release(&name);

    char *linkName = argv[0];
    char *prefix = argv[1];

    // passed in as <linkName> <prefix>, passed on as <prefix> <linkname>
    char routeArguments[MAXPATHLEN];
    sprintf(routeArguments, "%s %s", prefix, linkName);
    PARCBuffer *payload = parcBuffer_AllocateCString(routeArguments);
    ccnxInterest_SetPayload(interest, payload);
    parcBuffer_Release(&payload);

    const char *result = athenactl_SendInterestControl(identity, interest);
    if (result) {
        printf("FIB: %s\n", result);
        parcMemory_Deallocate(&result);
    }

    ccnxMetaMessage_Release(&interest);

    return 0;
}

static int
_athenactl_Add(PARCIdentity *identity, int argc, char **argv)
{
    if (argc < 1) {
        printf("usage: add link/connection/listener/route\n");
        return 1;
    }

    const char *subcommand = argv[0];

    if (strcasecmp(subcommand, SUBCOMMAND_ADD_LINK) == 0) {
        return _athenactl_AddLink(identity, --argc, &argv[1]);
    }
    if (strcasecmp(subcommand, SUBCOMMAND_ADD_CONNECTION) == 0) {
        return _athenactl_AddConnection(identity, --argc, &argv[1]);
    }
    if (strcasecmp(subcommand, SUBCOMMAND_ADD_LISTENER) == 0) {
        return _athenactl_AddListener(identity, --argc, &argv[1]);
    }
    if (strcasecmp(subcommand, SUBCOMMAND_ADD_ROUTE) == 0) {
        return _athenactl_AddRoute(identity, --argc, &argv[1]);
    }
    printf("usage: add link/connection/listener/route\n");
    return 1;
}

static int
_athenactl_ListLinks(PARCIdentity *identity, int argc, char **argv)
{
    CCNxName *name = ccnxName_CreateFromCString(CCNxNameAthenaCommand_LinkList);
    CCNxInterest *interest = ccnxInterest_CreateSimple(name);
    ccnxName_Release(&name);

    const char *result = athenactl_SendInterestControl(identity, interest);
    printf("Link: Interface list");
    if (result) {
        PARCBuffer *buffer = parcBuffer_WrapCString((char *) result);
        PARCJSONParser *parser = parcJSONParser_Create(buffer);
        PARCJSONValue *value = parcJSONValue_Parser(parser);

        if (value == NULL) {
            printf("\n\tCould not parse forwarder list response");
        } else {
            PARCJSONArray *array = parcJSONValue_GetArray(value);

            for (int i = 0; i < parcJSONArray_GetLength(array); i++) {
                PARCJSONValue *getvalue = parcJSONArray_GetValue(array, i);
                PARCJSON *json = parcJSONValue_GetJSON(getvalue);

                PARCJSONValue *pairValue = parcJSON_GetValueByName(json, "linkName");
                PARCBuffer *bufferString = parcJSONValue_GetString(pairValue);
                char *linkName = parcBuffer_ToString(bufferString);

                pairValue = parcJSON_GetValueByName(json, "index");
                int64_t index = parcJSONValue_GetInteger(pairValue);

                pairValue = parcJSON_GetValueByName(json, "notLocal");
                bool notLocal = parcJSONValue_GetBoolean(pairValue);

                pairValue = parcJSON_GetValueByName(json, "localForced");
                bool localForced = parcJSONValue_GetBoolean(pairValue);

                if (index < 0) {
                    if (notLocal) {
                        printf("\n    Link listener%s: %s", localForced ? " (forced remote)" : "", linkName);
                    } else {
                        printf("\n    Link listener%s: %s", localForced ? " (forced local)" : "", linkName);
                    }
                } else {
                    if (notLocal) {
                        printf("\n    Link instance [%" PRId64 "] %s: %s", index, localForced ? "(forced remote)" : "(remote)", linkName);
                    } else {
                        printf("\n    Link instance [%" PRId64 "] %s: %s", index, localForced ? "(forced local)" : "(local)", linkName);
                    }
                }
                parcMemory_Deallocate(&linkName);
            }
            parcJSONValue_Release(&value);
        }

        parcJSONParser_Release(&parser);
        parcBuffer_Release(&buffer);

        parcMemory_Deallocate(&result);
    }
    printf("\nDone.\n");

    ccnxMetaMessage_Release(&interest);

    return 0;
}

static int
_athenactl_ListFIB(PARCIdentity *identity, int argc, char **argv)
{
    CCNxName *name = ccnxName_CreateFromCString(CCNxNameAthenaCommand_FIBList);
    CCNxInterest *interest = ccnxInterest_CreateSimple(name);
    ccnxName_Release(&name);

    const char *result = athenactl_SendInterestControl(identity, interest);
    if (result) {
        PARCJSON *jsonContent = parcJSON_ParseString(result);
        if (jsonContent != NULL) {
            PARCJSONValue *resultValue = parcJSON_GetValueByName(jsonContent, JSON_KEY_RESULT);
            PARCJSONArray *fibEntryList = parcJSONValue_GetArray(resultValue);
            size_t fibEntryListLength = parcJSONArray_GetLength(fibEntryList);
            printf("Routes (%d):\n", (int) fibEntryListLength);
            if (fibEntryListLength == 0) {
                printf("    No Entries\n");
            }
            for (size_t i = 0; i < fibEntryListLength; ++i) {
                PARCJSONValue *elementValue = parcJSONArray_GetValue(fibEntryList, i);
                PARCJSON *valueObj = parcJSONValue_GetJSON(elementValue);
                PARCJSONValue *value = parcJSON_GetValueByName(valueObj, JSON_KEY_NAME);
                char *prefixString = parcBuffer_ToString(parcJSONValue_GetString(value));

                value = parcJSON_GetValueByName(valueObj, JSON_KEY_LINK);
                char *linkString = parcBuffer_ToString(parcJSONValue_GetString(value));
                printf("    %s -> %s\n", prefixString, linkString);
                parcMemory_Deallocate(&prefixString);
                parcMemory_Deallocate(&linkString);
            }
            parcJSON_Release(&jsonContent);
        } else {
            printf("Returned value is not JSON: %s\n", result);
        }
        parcMemory_Deallocate(&result);
    } else {
        printf("NULL result recieved from route List request\n");
    }

    ccnxMetaMessage_Release(&interest);

    return 0;
}

static int
_athenactl_List(PARCIdentity *identity, int argc, char **argv)
{
    if (argc < 1) {
        printf("usage: list links/connections/routes\n");
        return 1;
    }

    const char *subcommand = argv[0];

    if (strcasecmp(subcommand, SUBCOMMAND_LIST_LINKS) == 0) {
        return _athenactl_ListLinks(identity, --argc, &argv[1]);
    } else if (strcasecmp(subcommand, SUBCOMMAND_LIST_CONNECTIONS) == 0) {
        return _athenactl_ListLinks(identity, --argc, &argv[1]);
    } else if (strcasecmp(subcommand, SUBCOMMAND_LIST_ROUTES) == 0) {
        return _athenactl_ListFIB(identity, --argc, &argv[1]);
    } else if (strcasecmp(subcommand, SUBCOMMAND_LIST_FIB) == 0) {
        return _athenactl_ListFIB(identity, --argc, &argv[1]);
    }
    printf("usage: list links/connections/routes\n");
    return 1;
}

static int
_athenactl_RemoveLink(PARCIdentity *identity, int argc, char **argv)
{
    if (argc < 1) {
        printf("usage: remove link <linkName>\n");
        return 1;
    }

    CCNxName *name = ccnxName_CreateFromCString(CCNxNameAthenaCommand_LinkDisconnect);
    CCNxInterest *interest = ccnxInterest_CreateSimple(name);
    ccnxName_Release(&name);

    PARCBuffer *payload = parcBuffer_AllocateCString(argv[0]);
    ccnxInterest_SetPayload(interest, payload);
    parcBuffer_Release(&payload);

    const char *result = athenactl_SendInterestControl(identity, interest);
    if (result) {
        printf("Link: %s\n", result);
        parcMemory_Deallocate(&result);
    }

    ccnxMetaMessage_Release(&interest);

    return 0;
}

static int
_athenactl_Remove(PARCIdentity *identity, int argc, char **argv)
{
    if (argc < 1) {
        printf("usage: remove link/route\n");
        return 1;
    }

    const char *subcommand = argv[0];

    if (strcasecmp(subcommand, SUBCOMMAND_REMOVE_LINK) == 0) {
        return _athenactl_RemoveLink(identity, --argc, &argv[1]);
    }
    if (strcasecmp(subcommand, SUBCOMMAND_REMOVE_ROUTE) == 0) {
        return _athenactl_RemoveRoute(identity, --argc, &argv[1]);
    }
    printf("usage: remove link/route\n");
    return 1;
}

static int
_athenactl_SetDebug(PARCIdentity *identity, int argc, char **argv)
{
    CCNxName *name = ccnxName_CreateFromCString(CCNxNameAthenaCommand_Set "/" AthenaCommand_LogLevel "/" AthenaCommand_LogDebug);
    CCNxInterest *interest = ccnxInterest_CreateSimple(name);
    ccnxName_Release(&name);

    const char *result = athenactl_SendInterestControl(identity, interest);
    if (result) {
        printf("%s\n", result);
        parcMemory_Deallocate(&result);
    }

    ccnxMetaMessage_Release(&interest);

    return 0;
}

static int
_athenactl_SetLogLevel(PARCIdentity *identity, int argc, char **argv)
{
    if (argc < 1) {
        printf("usage: set level off/notice/info/debug/error/all\n");
        return 1;
    }

    char logLevelURI[MAXPATHLEN];
    sprintf(logLevelURI, "%s/level/%s", CCNxNameAthenaCommand_Set, argv[0]);
    CCNxName *name = ccnxName_CreateFromCString(logLevelURI);
    CCNxInterest *interest = ccnxInterest_CreateSimple(name);
    ccnxName_Release(&name);

    const char *result = athenactl_SendInterestControl(identity, interest);
    if (result) {
        printf("%s\n", result);
        parcMemory_Deallocate(&result);
    }

    ccnxMetaMessage_Release(&interest);

    return 0;
}

static int
_athenactl_UnSetDebug(PARCIdentity *identity, int argc, char **argv)
{
    CCNxName *name = ccnxName_CreateFromCString(CCNxNameAthenaCommand_Set "/" AthenaCommand_LogLevel "/" AthenaCommand_LogInfo);
    CCNxInterest *interest = ccnxInterest_CreateSimple(name);
    ccnxName_Release(&name);

    const char *result = athenactl_SendInterestControl(identity, interest);
    if (result) {
        printf("%s\n", result);
        parcMemory_Deallocate(&result);
    }

    ccnxMetaMessage_Release(&interest);

    return 0;
}

static int
_athenactl_Set(PARCIdentity *identity, int argc, char **argv)
{
    if (argc < 1) {
        printf("usage: set level/debug\n");
        return 1;
    }

    const char *subcommand = argv[0];

    if (strcasecmp(subcommand, SUBCOMMAND_SET_DEBUG) == 0) {
        return _athenactl_SetDebug(identity, --argc, &argv[1]);
    }
    if (strcasecmp(subcommand, SUBCOMMAND_SET_LEVEL) == 0) {
        return _athenactl_SetLogLevel(identity, --argc, &argv[1]);
    }
    printf("usage: set level/debug\n");
    return 1;
}

static int
_athenactl_UnSet(PARCIdentity *identity, int argc, char **argv)
{
    if (argc < 1) {
        printf("usage: unset debug\n");
        return 1;
    }

    const char *subcommand = argv[0];

    if (strcasecmp(subcommand, SUBCOMMAND_UNSET_DEBUG) == 0) {
        return _athenactl_UnSetDebug(identity, --argc, &argv[1]);
    }
    printf("usage: unset debug\n");
    return 1;
}

static int
_athenactl_Quit(PARCIdentity *identity, int argc, char **argv)
{
    CCNxName *name = ccnxName_CreateFromCString(CCNxNameAthenaCommand_Quit);
    CCNxInterest *interest = ccnxInterest_CreateSimple(name);
    ccnxName_Release(&name);

    PARCBuffer *payload = parcBuffer_AllocateCString("exit");
    ccnxInterest_SetPayload(interest, payload);
    parcBuffer_Release(&payload);

    const char *result = athenactl_SendInterestControl(identity, interest);
    if (result) {
        printf("%s\n", result);
        parcMemory_Deallocate(&result);
    }

    ccnxMetaMessage_Release(&interest);

    return 0;
}

static int
_athenactl_Run(PARCIdentity *identity, int argc, char **argv)
{
    if (argc < 1) {
        printf("usage: spawn <port | link specification>\n");
        return 1;
    }

    char *linkSpecification = argv[0];
    char constructedLinkSpecification[MAXPATHLEN] = { 0 };

    // Short-cut, user can specify just a port and we will construct a default tcp listener specification
    if (atoi(argv[0]) != 0) {
        sprintf(constructedLinkSpecification, "tcp://localhost:%d/listener", atoi(argv[0]));
        linkSpecification = constructedLinkSpecification;
    }

    CCNxName *name = ccnxName_CreateFromCString(CCNxNameAthenaCommand_Run);
    CCNxInterest *interest = ccnxInterest_CreateSimple(name);
    ccnxName_Release(&name);

    PARCBuffer *payload = parcBuffer_AllocateCString(linkSpecification);
    ccnxInterest_SetPayload(interest, payload);
    parcBuffer_Release(&payload);

    const char *result = athenactl_SendInterestControl(identity, interest);
    if (result) {
        printf("%s\n", result);
        parcMemory_Deallocate(&result);
    }

    ccnxMetaMessage_Release(&interest);

    return 0;
}

static int
_athenactl_InputCommand(PARCIdentity *identity, int argc, char **argv)
{
    CCNxName *name = ccnxName_CreateFromCString(argv[0]);
    CCNxInterest *interest = ccnxInterest_CreateSimple(name);
    ccnxName_Release(&name);
    if (argc > 1) {
        PARCBufferComposer *payloadComposer = parcBufferComposer_Create();
        parcBufferComposer_Format(payloadComposer, "%s", argv[1]);
        argc--; argv++;
        while (argc > 1) {
            parcBufferComposer_Format(payloadComposer, " %s", argv[1]);
            argc--; argv++;
        }
        PARCBuffer *payload = parcBufferComposer_GetBuffer(payloadComposer);
        parcBuffer_Flip(payload);
        ccnxInterest_SetPayload(interest, payload);
        parcBufferComposer_Release(&payloadComposer);
    }

    const char *result = athenactl_SendInterestControl(identity, interest);
    if (result) {
        printf("%s\n", result);
        parcMemory_Deallocate(&result);
    }

    ccnxMetaMessage_Release(&interest);

    return 0;
}

int
athenactl_Command(PARCIdentity *identity, int argc, char **argv)
{
    if (argc < 1) {
        printf("commands: add/list/remove/set/unset/spawn/quit\n");
        return 1;
    }

    const char *command = argv[0];

    if (strcasecmp(command, COMMAND_ADD) == 0) {
        return _athenactl_Add(identity, --argc, &argv[1]);
    }
    if (strcasecmp(command, COMMAND_LIST) == 0) {
        return _athenactl_List(identity, --argc, &argv[1]);
    }
    if (strcasecmp(command, COMMAND_REMOVE) == 0) {
        return _athenactl_Remove(identity, --argc, &argv[1]);
    }
    if (strcasecmp(command, COMMAND_SET) == 0) {
        return _athenactl_Set(identity, --argc, &argv[1]);
    }
    if (strcasecmp(command, COMMAND_UNSET) == 0) {
        return _athenactl_UnSet(identity, --argc, &argv[1]);
    }
    if (strcasecmp(command, COMMAND_RUN) == 0) {
        return _athenactl_Run(identity, --argc, &argv[1]);
    }
    if (strcasecmp(command, COMMAND_QUIT) == 0) {
        return _athenactl_Quit(identity, --argc, &argv[1]);
    }
    if (strncasecmp(command, CCNxNameAthena_Forwarder, strlen(CCNxNameAthena_Forwarder)) == 0) {
        return _athenactl_InputCommand(identity, argc, &argv[0]);
    }
    printf("athenactl: unknown command\n");
    printf("commands: add/list/remove/set/unset/spawn/quit\n");
    return 1;
}

void
athenactl_Usage(void)
{
    printf("        add link <schema>://<authority>[/listener][/<options>][/name=<linkname>]\n");
    printf("            <schema> == tcp/...\n");
    printf("            <authority> == <protocol specific address/port>\n");
    printf("            <options> == local=<true/false>\n");
    printf("        remove link <linkname>\n");
    printf("        list <links/routes>\n");
    printf("        add route <linkname> lci:/<path>\n");
    printf("        remove route <linkname> lci:/<path>\n");
    printf("        set level <off/notice/info/debug/error/all>\n");
    printf("        spawn <port>\n");
    printf("        quit\n");
}
