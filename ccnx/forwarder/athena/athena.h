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
 * @file athena.h
 * @brief Athena forwarder
 *
 * @author Kevin Fox, Palo Alto Research Center (Xerox PARC)
 * @copyright (c) 2015-2016, Xerox Corporation (Xerox) and Palo Alto Research Center, Inc (PARC).  All rights reserved.
 */

#ifndef libathena_h
#define libathena_h

#include <ccnx/transport/common/transport_MetaMessage.h>

#include <ccnx/forwarder/athena/athena_TransportLinkAdapter.h>
#include <ccnx/forwarder/athena/athena_ContentStore.h>
#include <ccnx/forwarder/athena/athena_PIT.h>
#include <ccnx/forwarder/athena/athena_FIB.h>

#define AthenaDefaultConnectionURI "tcp://localhost:9695/Listener"
#define AthenaDefaultContentStoreSize 0
#define AthenaDefaultListenerPort 9695

/**
 * @typedef AthenaTransportLinkFlag
 * @brief An enumeration of link instance flags
 */
typedef enum {
    Athena_Exit = 0x00,
    Athena_Running = 0x01
} AthenaState;

/**
 * @typedef Athena∫
 * @brief private data for Athena daemon
 */
typedef struct Athena {
    CCNxName *athenaName;
    AthenaState athenaState;
    AthenaTransportLinkAdapter *athenaTransportLinkAdapter;
    AthenaPIT *athenaPIT;
    AthenaFIB *athenaFIB;
    AthenaContentStore *athenaContentStore;
    PARCLog *log;
    PARCOutputStream *configurationLog;

    struct {
        uint64_t numProcessedInterests;
        uint64_t numProcessedContentObjects;
        uint64_t numProcessedInterestReturns;
        uint64_t numProcessedControlMessages;
        uint64_t numProcessedManifests;
    } stats;
} Athena;

#define AthenaModule_Control              "Control"
#define AthenaModule_FIB                  "FIB"
#define AthenaModule_PIT                  "PIT"
#define AthenaModule_ContentStore         "ContentStore"
#define AthenaModule_TransportLinkAdapter "TransportLinkAdapter"

#define CCNxNameAthena_Forwarder    "ccnx:/local/forwarder"
#define CCNxNameAthena_Control      CCNxNameAthena_Forwarder "/" AthenaModule_Control
#define CCNxNameAthena_FIB          CCNxNameAthena_Forwarder "/" AthenaModule_FIB
#define CCNxNameAthena_PIT          CCNxNameAthena_Forwarder "/" AthenaModule_PIT
#define CCNxNameAthena_ContentStore CCNxNameAthena_Forwarder "/" AthenaModule_ContentStore
#define CCNxNameAthena_Link         CCNxNameAthena_Forwarder "/" AthenaModule_TransportLinkAdapter

// General Commands
#define AthenaCommandSegment 3
#define AthenaCommand_Lookup "lookup"
#define AthenaCommand_Add    "add"
#define AthenaCommand_List   "list"
#define AthenaCommand_Remove "remove"
#define AthenaCommand_Resize "resize"
#define AthenaCommand_Set    "set"
#define AthenaCommand_Quit   "quit"
#define AthenaCommand_Run    "spawn"
#define AthenaCommand_Stats  "stats"

#define AthenaCommand_LogLevel  "level"
#define AthenaCommand_LogDebug  "debug"
#define AthenaCommand_LogInfo   "info"
#define AthenaCommand_LogError  "error"
#define AthenaCommand_LogAll    "all"
#define AthenaCommand_LogOff    "off"
#define AthenaCommand_LogNotice "notice"

// Module Specific Commands
#define CCNxNameAthenaCommand_LinkConnect        CCNxNameAthena_Link "/" AthenaCommand_Add    // create a connection to interface specified in payload, returns name
#define CCNxNameAthenaCommand_LinkDisconnect     CCNxNameAthena_Link "/" AthenaCommand_Remove // remove a connection to interface specified in payload, by name
#define CCNxNameAthenaCommand_LinkList           CCNxNameAthena_Link "/" AthenaCommand_List   // list interfaces
#define CCNxNameAthenaCommand_FIBLookup          CCNxNameAthena_FIB "/" AthenaCommand_Lookup                  // return current FIB contents for name in payload
#define CCNxNameAthenaCommand_FIBList            CCNxNameAthena_FIB "/" AthenaCommand_List                    // list current FIB contents
#define CCNxNameAthenaCommand_FIBAddRoute        CCNxNameAthena_FIB "/" AthenaCommand_Add                     // add route for arguments in payload
#define CCNxNameAthenaCommand_FIBRemoveRoute     CCNxNameAthena_FIB "/" AthenaCommand_Remove                  // remove route for arguments in payload
#define CCNxNameAthenaCommand_PITLookup          CCNxNameAthena_PIT "/" AthenaCommand_Lookup                  // return current PIT contents for name in payload
#define CCNxNameAthenaCommand_PITList            CCNxNameAthena_PIT "/" AthenaCommand_List                    // list current PIT contents
#define CCNxNameAthenaCommand_ContentStoreResize CCNxNameAthena_ContentStore "/" AthenaCommand_Resize         // resize current content store to size in MB in payload
#define CCNxNameAthenaCommand_Quit               CCNxNameAthena_Control "/" AthenaCommand_Quit                // ask the forwarder to exit
#define CCNxNameAthenaCommand_Run                CCNxNameAthena_Control "/" AthenaCommand_Run                 // start a new forwarder instance
#define CCNxNameAthenaCommand_Set                CCNxNameAthena_Control "/" AthenaCommand_Set                 // set a forwarder variable
#define CCNxNameAthenaCommand_Stats              CCNxNameAthena_Control "/" AthenaCommand_Stats               // get forwarder stats

/**
 * @abstract create an Athena forwarder instance
 * @discussion
 *
 * @param [in] contentStoreSizeInMB size of content store in MB
 * @return athena instance
 *
 * Example:
 * @code
 * {
 *     Athena *athena = athena_Create(10);
 *     ...
 *     athena_Release(&athena);
 * }
 * @endcode
 */
Athena *athena_Create(size_t contentStoreSizeInMB);

/**
 * @abstract acquire a reference to an Athena forwarder instance
 * @discussion
 *
 * @param [in] athena instance
 *
 * Example:
 * @code
 * {
 *     Athena *athena = athena_Create(10);
 *     ...
 *     athena_Acquire(athena);
 *     ...
 *     athena_Release(&athena);
 *     ...
 *     athena_Release(&athena);
 * }
 * @endcode
 */
Athena *athena_Acquire(const Athena *athena);

/**
 * @abstract release an Athena forwarder instance
 * @discussion
 *
 * @param [in] athena instance
 *
 * Example:
 * @code
 * {
 *     Athena *athena = athena_Create(10);
 *     ...
 *     athena_Release(&athena);
 * }
 * @endcode
 */
void athena_Release(Athena **athena);

/**
 * @abstract process a CCNx message
 * @discussion
 *
 * @param [in] athena forwarder context
 * @param [in] ccnxMessage pointer to message to process
 * @param [in] ingressVector link message was received from
 *
 * Example:
 * @code
 * {
 *     Athena *athena = athena_Create(10);
 *     PARCBitVector *ingressVector;
 *     CCNxMetaMessage *ccnxMessage = athenaTransportLinkAdapter_Receive(athena->athenaTransportLinkAdapter, &ingressVector, -1)
 *
 *     athena_ProcessMessage(athena, ccnxMessage, ingressVector);
 *
 *     ccnxMetaMessage_Release(&ccnxMessage);
 *     parcBitVector_Release(&ingressVector);
 *     athena_Release(&athena);
 * }
 * @endcode
 */
void athena_ProcessMessage(Athena *athena, CCNxMetaMessage *ccnxMessage, PARCBitVector *ingressVector);

/**
 * @abstract encode message into wire format
 * @discussion
 *
 * @param [in] message
 *
 * Example:
 * @code
 * {
 *     athena_EncodeMessage(message);
 * }
 * @endcode
 */
void athena_EncodeMessage(CCNxMetaMessage *message);

/**
 * @abstract start an athena forwarder loop
 * @discussion
 *
 * @param [in] athena instance pointer
 * @return exit status
 *
 * Example:
 * @code
 * {
 * }
 * @endcode
 */
void *athena_ForwarderEngine(void *athena);
#endif // libathena_h
