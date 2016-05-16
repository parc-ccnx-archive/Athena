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
 * Athena interest control management implementation
 */

#include <config.h>

#include <errno.h>
#include <pthread.h>
#include <sys/param.h>
#include <stdio.h>

#include "athena_InterestControl.h"

#include "athena_Control.h"
#include "athena_PIT.h"
#include "athena_FIB.h"
#include "athena_ContentStore.h"
#include "athena_TransportLinkAdapter.h"

#include <parc/algol/parc_Memory.h>
#include <parc/algol/parc_JSON.h>

#include <ccnx/common/ccnx_InterestReturn.h>
#include <ccnx/common/ccnx_ContentObject.h>

#include <ccnx/api/control/controlPlaneInterface.h>

#include "athena_About.h"

static char *
_get_arguments(CCNxInterest *interest)
{
    char *arguments = NULL;

    // Get the payload arguments associated with the component command
    PARCBuffer *interestPayload = ccnxInterest_GetPayload(interest);
    if (interestPayload) {
        arguments = parcBuffer_ToString(interestPayload);
    }

    return arguments;
}

static CCNxMetaMessage *
_create_response(Athena *athena, CCNxName *ccnxName, const char *format, ...)
{
    va_list ap;
    va_start(ap, format);

    char responseBuffer[MAXPATHLEN];
    vsprintf(responseBuffer, format, ap);

    PARCBuffer *responsePayload = parcBuffer_AllocateCString(responseBuffer);

    parcLog_Info(athena->log, responseBuffer);
    CCNxContentObject *responseContent = ccnxContentObject_CreateWithNameAndPayload(ccnxName, responsePayload);
    CCNxMetaMessage *responseMessage = ccnxMetaMessage_CreateFromContentObject(responseContent);

    ccnxContentObject_Release(&responseContent);
    parcBuffer_Release(&responsePayload);

    athena_EncodeMessage(responseMessage);
    return responseMessage;
}

static CCNxMetaMessage *
_create_stats_response(Athena *athena, CCNxName *ccnxName)
{
    PARCJSON *json = parcJSON_Create();

    struct timeval tv;
    gettimeofday(&tv, NULL);

    uint64_t nowInMillis = (tv.tv_sec * 1000) + (tv.tv_usec / 1000);

    parcJSON_AddString(json, "moduleName", athenaAbout_Name());
    parcJSON_AddInteger(json, "time", nowInMillis);
    parcJSON_AddInteger(json, "numProcessedInterests",
                        athena->stats.numProcessedInterests);
    parcJSON_AddInteger(json, "numProcessedContentObjects",
                        athena->stats.numProcessedContentObjects);
    parcJSON_AddInteger(json, "numProcessedControlMessages",
                        athena->stats.numProcessedControlMessages);
    parcJSON_AddInteger(json, "numProcessedInterestReturns",
                        athena->stats.numProcessedInterestReturns);

    char *jsonString = parcJSON_ToString(json);

    parcJSON_Release(&json);

    PARCBuffer *payload = parcBuffer_CreateFromArray(jsonString, strlen(jsonString));

    parcMemory_Deallocate(&jsonString);

    CCNxContentObject *contentObject =
        ccnxContentObject_CreateWithNameAndPayload(ccnxName, parcBuffer_Flip(payload));
    ccnxContentObject_SetExpiryTime(contentObject, nowInMillis + 100); // this response is good for 100 millis

    CCNxMetaMessage *result = ccnxMetaMessage_CreateFromContentObject(contentObject);

    ccnxContentObject_Release(&contentObject);
    parcBuffer_Release(&payload);

    return result;
}

void
athenaInterestControl_LogConfigurationChange(Athena *athena, CCNxName *ccnxName, const char *format, ...)
{
    if (athena->configurationLog) {
        const char *name = ccnxName_ToString(ccnxName);
        parcOutputStream_WriteCString(athena->configurationLog, name);
        parcMemory_Deallocate(&name);

        char configurationLogBuffer[MAXPATHLEN] = {0};
        if (format) {
            va_list ap;
            va_start(ap, format);

            parcOutputStream_WriteCString(athena->configurationLog, " ");
            vsprintf(configurationLogBuffer, format, ap);
            parcOutputStream_WriteCString(athena->configurationLog, configurationLogBuffer);
        }
        parcOutputStream_WriteCString(athena->configurationLog, "\n");
    }
}

static CCNxMetaMessage *
_Control_Command_Set(Athena *athena, CCNxName *ccnxName, const char *command)
{
    CCNxMetaMessage *responseMessage = NULL;

    if (ccnxName_GetSegmentCount(ccnxName) <= (AthenaCommandSegment + 2)) {
        responseMessage = _create_response(athena, ccnxName, "Athena set arguments required <name> <value>");
        return responseMessage;
    }
    // Check for required set name argument
    CCNxNameSegment *nameSegment = ccnxName_GetSegment(ccnxName, AthenaCommandSegment + 1);
    char *name = ccnxNameSegment_ToString(nameSegment);
    if (strcasecmp(name, AthenaCommand_LogLevel) == 0) {
        // Check the level to set the log to
        nameSegment = ccnxName_GetSegment(ccnxName, AthenaCommandSegment + 2);
        char *level = ccnxNameSegment_ToString(nameSegment);
        if (strcasecmp(level, AthenaCommand_LogDebug) == 0) {
            athenaTransportLinkAdapter_SetLogLevel(athena->athenaTransportLinkAdapter, PARCLogLevel_Debug);
            parcLog_SetLevel(athena->log, PARCLogLevel_Debug);
            athenaInterestControl_LogConfigurationChange(athena, ccnxName, NULL);
            responseMessage = _create_response(athena, ccnxName, "set athena logging level to %s", AthenaCommand_LogDebug);
        } else if (strcasecmp(level, AthenaCommand_LogInfo) == 0) {
            athenaTransportLinkAdapter_SetLogLevel(athena->athenaTransportLinkAdapter, PARCLogLevel_Info);
            parcLog_SetLevel(athena->log, PARCLogLevel_Info);
            athenaInterestControl_LogConfigurationChange(athena, ccnxName, NULL);
            responseMessage = _create_response(athena, ccnxName, "set athena logging level to %s", AthenaCommand_LogInfo);
        } else if (strcasecmp(level, AthenaCommand_LogOff) == 0) {
            athenaTransportLinkAdapter_SetLogLevel(athena->athenaTransportLinkAdapter, PARCLogLevel_Off);
            parcLog_SetLevel(athena->log, PARCLogLevel_Off);
            athenaInterestControl_LogConfigurationChange(athena, ccnxName, NULL);
            responseMessage = _create_response(athena, ccnxName, "set athena logging level to %s", AthenaCommand_LogOff);
        } else if (strcasecmp(level, AthenaCommand_LogAll) == 0) {
            athenaTransportLinkAdapter_SetLogLevel(athena->athenaTransportLinkAdapter, PARCLogLevel_All);
            parcLog_SetLevel(athena->log, PARCLogLevel_All);
            athenaInterestControl_LogConfigurationChange(athena, ccnxName, NULL);
            responseMessage = _create_response(athena, ccnxName, "set athena logging level to %s", AthenaCommand_LogAll);
        } else if (strcasecmp(level, AthenaCommand_LogError) == 0) {
            athenaTransportLinkAdapter_SetLogLevel(athena->athenaTransportLinkAdapter, PARCLogLevel_Error);
            parcLog_SetLevel(athena->log, PARCLogLevel_Error);
            athenaInterestControl_LogConfigurationChange(athena, ccnxName, NULL);
            responseMessage = _create_response(athena, ccnxName, "set athena logging level to %s", AthenaCommand_LogError);
        } else if (strcasecmp(level, AthenaCommand_LogNotice) == 0) {
            athenaTransportLinkAdapter_SetLogLevel(athena->athenaTransportLinkAdapter, PARCLogLevel_Notice);
            parcLog_SetLevel(athena->log, PARCLogLevel_Notice);
            athenaInterestControl_LogConfigurationChange(athena, ccnxName, NULL);
            responseMessage = _create_response(athena, ccnxName, "set athena logging level to %s", AthenaCommand_LogNotice);
        } else {
            responseMessage = _create_response(athena, ccnxName, "unknown logging level (%s)", level);
        }
        parcMemory_Deallocate(&level);
    } else {
        responseMessage = _create_response(athena, ccnxName, "Athena unknown set name (%s)", name);
    }

    parcMemory_Deallocate(&name);
    return responseMessage;
}

static CCNxMetaMessage *
_Control_Command_Quit(Athena *athena, CCNxName *ccnxName, const char *command)
{
    athena->athenaState = Athena_Exit;
    athenaInterestControl_LogConfigurationChange(athena, ccnxName, NULL);
    return _create_response(athena, ccnxName, "Athena exiting ...");
}

static CCNxMetaMessage *
_Control_Command_Stats(Athena *athena, CCNxName *ccnxName, const char *command)
{
    return _create_stats_response(athena, ccnxName);
}

static void *
_start_forwarder_instance(void *arg)
{
    void *res = athena_ForwarderEngine(arg);
    pthread_detach(pthread_self());
    return res;
}

static CCNxMetaMessage *
_Control_Command_Spawn(Athena *athena, CCNxName *ccnxName, const char *command, const char *connectionSpecification)
{
    CCNxMetaMessage *responseMessage;

    // Create a new athena instance
    Athena *newAthena = athena_Create(AthenaDefaultContentStoreSize);
    if (newAthena == NULL) {
        responseMessage = _create_response(athena, ccnxName, "Could not create a new Athena instance");
        return responseMessage;
    }

    // Add the specified link
    PARCURI *connectionURI = parcURI_Parse(connectionSpecification);
    if (athenaTransportLinkAdapter_Open(newAthena->athenaTransportLinkAdapter, connectionURI) == NULL) {
        parcLog_Error(athena->log, "Unable to configure an interface.  Exiting...");
        responseMessage = _create_response(athena, ccnxName, "Unable to configure an Athena interface for thread");
        parcURI_Release(&connectionURI);
        athena_Release(&newAthena);
        return responseMessage;
    }
    parcURI_Release(&connectionURI);

    pthread_t thread;
    // Passing in a reference that will be released by the new thread as the thread may not
    // have time to acquire a reference itself before we release our reference.
    if (pthread_create(&thread, NULL, _start_forwarder_instance, (void *) athena_Acquire(newAthena)) != 0) {
        responseMessage = _create_response(athena, ccnxName, "Athena process thread creation failed");
        return responseMessage;
    }
    athena_Release(&newAthena);

    athenaInterestControl_LogConfigurationChange(athena, ccnxName, "%s", connectionSpecification);

    responseMessage = _create_response(athena, ccnxName, "Athena process thread started on %s", connectionSpecification);
    return responseMessage;
}

static CCNxMetaMessage *
_Control_Command(Athena *athena, CCNxInterest *interest)
{
    CCNxMetaMessage *responseMessage;
    responseMessage = athenaControl_ProcessMessage(athena, interest);
    if (responseMessage) {
        return responseMessage;
    }

    CCNxName *ccnxName = ccnxInterest_GetName(interest);
    if (ccnxName_GetSegmentCount(ccnxName) <= AthenaCommandSegment) {
        responseMessage = _create_response(athena, ccnxName, "No command specified");
        return responseMessage;
    }

    CCNxNameSegment *nameSegment = ccnxName_GetSegment(ccnxName, AthenaCommandSegment);
    char *command = ccnxNameSegment_ToString(nameSegment);

    // Set <level> <debug,info>
    if (strncasecmp(command, AthenaCommand_Set, strlen(AthenaCommand_Set)) == 0) {
        responseMessage = _Control_Command_Set(athena, ccnxName, command);
        parcMemory_Deallocate(&command);
        return responseMessage;
    }

    // Quit
    if (strncasecmp(command, AthenaCommand_Quit, strlen(AthenaCommand_Quit)) == 0) {
        responseMessage = _Control_Command_Quit(athena, ccnxName, command);
        parcMemory_Deallocate(&command);
        return responseMessage;
    }

    // Stats
    if (strncasecmp(command, AthenaCommand_Stats, strlen(AthenaCommand_Stats)) == 0) {
        responseMessage = _Control_Command_Stats(athena, ccnxName, command);
        parcMemory_Deallocate(&command);
        return responseMessage;
    }

    // Spawn
    if (strncasecmp(command, AthenaCommand_Run, strlen(AthenaCommand_Run)) == 0) {
        const char *connectionSpecification = _get_arguments(interest);
        responseMessage = _Control_Command_Spawn(athena, ccnxName, command, connectionSpecification);
        parcMemory_Deallocate(&connectionSpecification);
        parcMemory_Deallocate(&command);
        return responseMessage;
    }

    responseMessage = _create_response(athena, ccnxName, "Unknown command \"%s\"", command);
    parcMemory_Deallocate(&command);
    return responseMessage;
}

static CCNxMetaMessage *
_PIT_Command(Athena *athena, CCNxInterest *interest)
{
    CCNxMetaMessage *responseMessage;
    responseMessage = athenaPIT_ProcessMessage(athena->athenaPIT, interest);
    if (responseMessage) {
        return responseMessage;
    }

    CCNxName *ccnxName = ccnxInterest_GetName(interest);
    if (ccnxName_GetSegmentCount(ccnxName) > AthenaCommandSegment) {
        CCNxNameSegment *nameSegment = ccnxName_GetSegment(ccnxName, AthenaCommandSegment);
        char *command = ccnxNameSegment_ToString(nameSegment);

        if (strcasecmp(command, AthenaCommand_List) == 0) {
            parcLog_Debug(athena->log, "PIT List command invoked");
            PARCList *pitEntries = athenaPIT_CreateEntryList(athena->athenaPIT);
            printf("\n");
            for (size_t i = 0; i < parcList_Size(pitEntries); ++i) {
                PARCBuffer *strbuf = parcList_GetAtIndex(pitEntries, i);
                char *toprint = parcBuffer_ToString(strbuf);
                parcLog_Info(athena->log, "%s\n", toprint);
                parcMemory_Deallocate(&toprint);
            }
            parcList_Release(&pitEntries);
            responseMessage = _create_response(athena, ccnxName, "PIT listed on forwarder output log.");
        } else {
            responseMessage = _create_response(athena, ccnxName, "Unknown command: %s", command);
        }

        parcMemory_Deallocate(&command);
    }
    return responseMessage;
}

static CCNxMetaMessage *
_ContentStore_Command(Athena *athena, CCNxInterest *interest)
{
    CCNxMetaMessage *responseMessage;
    responseMessage = athenaContentStore_ProcessMessage(athena->athenaContentStore, interest);
    if (responseMessage) {
        return responseMessage;
    }

    CCNxName *ccnxName = ccnxInterest_GetName(interest);
    if (ccnxName_GetSegmentCount(ccnxName) > AthenaCommandSegment) {
        CCNxNameSegment *nameSegment = ccnxName_GetSegment(ccnxName, AthenaCommandSegment);
        char *command = ccnxNameSegment_ToString(nameSegment);

        //char *arguments = _get_arguments(interest);

        //responseMessage = _create_response(athena, ccnxName, ...

        //parcMemory_Deallocate(&command);
        //if (arguments) {
        //    parcMemory_Deallocate(&arguments);
        //}

        parcMemory_Deallocate(&command);
    }
    return responseMessage;
}

static CCNxMetaMessage *
_create_FIBList_response(Athena *athena, CCNxName *ccnxName, PARCList *fibEntryList)
{
    PARCJSON *jsonPayload = parcJSON_Create();
    PARCJSONArray *jsonEntryList = parcJSONArray_Create();
    parcJSON_AddArray(jsonPayload, JSON_KEY_RESULT, jsonEntryList);

    for (size_t i = 0; i < parcList_Size(fibEntryList); ++i) {
        AthenaFIBListEntry *entry = parcList_GetAtIndex(fibEntryList, i);
        if (entry != NULL) {
            CCNxName *prefixName = athenaFIBListEntry_GetName(entry);
            char *prefix = ccnxName_ToString(prefixName);
            int linkId = athenaFIBListEntry_GetLinkId(entry);
            const char *linkName = athenaTransportLinkAdapter_LinkIdToName(athena->athenaTransportLinkAdapter, linkId);
            parcLog_Debug(athena->log, "  Route: %s->%s", prefix, linkName);

            PARCJSON *jsonItem = parcJSON_Create();
            parcJSON_AddString(jsonItem, JSON_KEY_NAME, prefix);
            parcJSON_AddString(jsonItem, JSON_KEY_LINK, linkName);

            PARCJSONValue *jsonItemValue = parcJSONValue_CreateFromJSON(jsonItem);
            parcJSON_Release(&jsonItem);

            parcJSONArray_AddValue(jsonEntryList, jsonItemValue);
            parcJSONValue_Release(&jsonItemValue);

            parcMemory_Deallocate(&prefix);
        }
    }

    char *jsonString = parcJSON_ToString(jsonPayload);

    parcJSONArray_Release(&jsonEntryList);
    parcJSON_Release(&jsonPayload);

    PARCBuffer *payload = parcBuffer_CreateFromArray(jsonString, strlen(jsonString));

    CCNxContentObject *contentObject =
        ccnxContentObject_CreateWithNameAndPayload(ccnxName, parcBuffer_Flip(payload));

    struct timeval tv;
    gettimeofday(&tv, NULL);
    uint64_t nowInMillis = (tv.tv_sec * 1000) + (tv.tv_usec / 1000);
    ccnxContentObject_SetExpiryTime(contentObject, nowInMillis + 100); // this response is good for 100 millis

    CCNxMetaMessage *result = ccnxMetaMessage_CreateFromContentObject(contentObject);

    ccnxContentObject_Release(&contentObject);
    parcBuffer_Release(&payload);
    parcMemory_Deallocate(&jsonString);

    athena_EncodeMessage(result);
    return result;
}

static CCNxMetaMessage *
_FIB_Command(Athena *athena, CCNxInterest *interest, PARCBitVector *ingress)
{
    CCNxMetaMessage *responseMessage;
    responseMessage = athenaFIB_ProcessMessage(athena->athenaFIB, interest);
    if (responseMessage) {
        return responseMessage;
    }

    CCNxName *ccnxName = ccnxInterest_GetName(interest);
    if (ccnxName_GetSegmentCount(ccnxName) > AthenaCommandSegment) {
        CCNxNameSegment *nameSegment = ccnxName_GetSegment(ccnxName, AthenaCommandSegment);
        char *command = ccnxNameSegment_ToString(nameSegment);

        if ((strcasecmp(command, AthenaCommand_Add) == 0) || (strcasecmp(command, AthenaCommand_Remove) == 0)) {
            char *arguments = _get_arguments(interest);

            if (arguments == NULL) {
                responseMessage = _create_response(athena, ccnxName, "No link or prefix arguments given to %s route command", command);
                parcMemory_Deallocate(&command);
                return responseMessage;
            }

            char linkName[MAXPATHLEN];
            char prefix[MAXPATHLEN];
            PARCBitVector *linkVector;

            // {Add,Remove} Route arguments "<prefix> [<linkName>]", if linkName not specified, use the incoming link id ([de-]registration)
            int numberOfArguments = sscanf(arguments, "%s %s", prefix, linkName);
            if (numberOfArguments == 2) {
                int linkId = athenaTransportLinkAdapter_LinkNameToId(athena->athenaTransportLinkAdapter, linkName);
                if (linkId == -1) {
                    responseMessage = _create_response(athena, ccnxName, "Unknown linkName %s", linkName);
                    parcMemory_Deallocate(&command);
                    parcMemory_Deallocate(&arguments);
                    return responseMessage;
                }
                linkVector = parcBitVector_Create();
                parcBitVector_Set(linkVector, linkId);
            } else if (numberOfArguments == 1) { // use ingress link
                linkVector = parcBitVector_Acquire(ingress);
            } else {
                responseMessage = _create_response(athena, ccnxName, "No prefix specified or too many arguments");
                parcMemory_Deallocate(&command);
                parcMemory_Deallocate(&arguments);
                return responseMessage;
            }

            CCNxName *prefixName = ccnxName_CreateFromCString(prefix);
            if (prefixName == NULL) {
                responseMessage = _create_response(athena, ccnxName, "Unable to parse prefix %s", prefix);
                parcMemory_Deallocate(&command);
                parcMemory_Deallocate(&arguments);
                parcBitVector_Release(&linkVector);
                return responseMessage;
            }

            int result = false;
            if (strcasecmp(command, AthenaCommand_Add) == 0) {
                result = athenaFIB_AddRoute(athena->athenaFIB, prefixName, linkVector);
            } else if (strcasecmp(command, AthenaCommand_Remove) == 0) {
                result = athenaFIB_DeleteRoute(athena->athenaFIB, prefixName, linkVector);
            }

            if (result == true) {
                char *routePrefix = ccnxName_ToString(prefixName);
                const char *linkIdName = athenaTransportLinkAdapter_LinkIdToName(athena->athenaTransportLinkAdapter, parcBitVector_NextBitSet(linkVector, 0));
                responseMessage = _create_response(athena, ccnxName, "%s route %s -> %s", command, routePrefix, linkIdName);
                athenaInterestControl_LogConfigurationChange(athena, ccnxName, "%s %s", routePrefix, linkIdName);
                parcMemory_Deallocate(&routePrefix);
            } else {
                responseMessage = _create_response(athena, ccnxName, "%s failed", command);
            }
            parcBitVector_Release(&linkVector);
            ccnxName_Release(&prefixName);

            parcMemory_Deallocate(&arguments);
        } else if (strcasecmp(command, AthenaCommand_List) == 0) {
            // Need to create the response here because as the FIB doesn't know the linkName
            parcLog_Debug(athena->log, "FIB List command invoked");
            PARCList *fibEntries = athenaFIB_CreateEntryList(athena->athenaFIB);
            responseMessage = _create_FIBList_response(athena, ccnxName, fibEntries);
            parcList_Release(&fibEntries);
        } else {
            responseMessage = _create_response(athena, ccnxName, "Unknown command: %s", command);
        }

        parcMemory_Deallocate(&command);
    }
    return responseMessage;
}

static CCNxMetaMessage *
_TransportLinkAdapter_Command(Athena *athena, CCNxInterest *interest)
{
    CCNxMetaMessage *responseMessage;
    responseMessage = athenaTransportLinkAdapter_ProcessMessage(athena->athenaTransportLinkAdapter, interest);
    if (responseMessage) {
        return responseMessage;
    }

    CCNxName *ccnxName = ccnxInterest_GetName(interest);
    if (ccnxName_GetSegmentCount(ccnxName) > AthenaCommandSegment) {
        CCNxNameSegment *nameSegment = ccnxName_GetSegment(ccnxName, AthenaCommandSegment);
        char *command = ccnxNameSegment_ToString(nameSegment);

        char *arguments = _get_arguments(interest);
        if (arguments == NULL) {
            responseMessage = _create_response(athena, ccnxName, "No link arguments given to %s command", command);
            parcMemory_Deallocate(&command);
            return responseMessage;
        }

        if (strcasecmp(command, AthenaCommand_Add) == 0) {
            if (arguments) {
                PARCURI *connectionURI = parcURI_Parse(arguments);
                if (connectionURI == NULL) {
                    responseMessage = _create_response(athena, ccnxName, "Could not parse URI:  %s", arguments);
                    return responseMessage;
                }
                const char *linkName = athenaTransportLinkAdapter_Open(athena->athenaTransportLinkAdapter, connectionURI);
                parcURI_Release(&connectionURI);
                if (linkName) {
                    responseMessage = _create_response(athena, ccnxName, "%s", linkName);
                    athenaInterestControl_LogConfigurationChange(athena, ccnxName, "%s", arguments);
                } else {
                    responseMessage = _create_response(athena, ccnxName, "New %s link failed: %s", arguments, strerror(errno));
                }
            }
        } else if (strcasecmp(command, AthenaCommand_Remove) == 0) {
            if (arguments) {
                int result = athenaTransportLinkAdapter_CloseByName(athena->athenaTransportLinkAdapter, arguments);
                if (result) {
                    responseMessage = _create_response(athena, ccnxName, "removal of %s failed", arguments);
                } else {
                    responseMessage = _create_response(athena, ccnxName, "%s removed", arguments);
                    athenaInterestControl_LogConfigurationChange(athena, ccnxName, "%s", arguments);
                }
            }
        } else {
            responseMessage = _create_response(athena, ccnxName, "Unknown TransportLinkAdapter command %s", command);
        }

        parcMemory_Deallocate(&command);
        parcMemory_Deallocate(&arguments);
    }
    return responseMessage;
}

int
athenaInterestControl(Athena *athena, CCNxInterest *interest, PARCBitVector *ingressVector)
{
    CCNxMetaMessage *responseMessage = NULL;
    CCNxName *ccnxName = ccnxInterest_GetName(interest);

    CCNxName *ccnxComponentName = ccnxName_CreateFromCString(CCNxNameAthena_Control);
    if (ccnxName_StartsWith(ccnxName, ccnxComponentName) == true) {
        responseMessage = _Control_Command(athena, interest);
    }
    ccnxName_Release(&ccnxComponentName);

    ccnxComponentName = ccnxName_CreateFromCString(CCNxNameAthena_Link);
    if (ccnxName_StartsWith(ccnxName, ccnxComponentName) == true) {
        responseMessage = _TransportLinkAdapter_Command(athena, interest);
    }
    ccnxName_Release(&ccnxComponentName);

    ccnxComponentName = ccnxName_CreateFromCString(CCNxNameAthena_FIB);
    if (ccnxName_StartsWith(ccnxName, ccnxComponentName) == true) {
        responseMessage = _FIB_Command(athena, interest, ingressVector);
    }
    ccnxName_Release(&ccnxComponentName);

    ccnxComponentName = ccnxName_CreateFromCString(CCNxNameAthena_PIT);
    if (ccnxName_StartsWith(ccnxName, ccnxComponentName) == true) {
        responseMessage = _PIT_Command(athena, interest);
    }
    ccnxName_Release(&ccnxComponentName);

    ccnxComponentName = ccnxName_CreateFromCString(CCNxNameAthena_ContentStore);
    if (ccnxName_StartsWith(ccnxName, ccnxComponentName) == true) {
        responseMessage = _ContentStore_Command(athena, interest);
    }
    ccnxName_Release(&ccnxComponentName);

    if (responseMessage) {
        athena_ProcessMessage(athena, responseMessage, ingressVector);
        ccnxContentObject_Release(&responseMessage);
    }

    return 0;
}
