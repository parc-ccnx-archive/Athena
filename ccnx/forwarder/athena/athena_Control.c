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
/*
 * Athena control management implementation
 */

#include <config.h>

#include <ccnx/forwarder/athena/athena_Control.h>
#include <parc/algol/parc_Memory.h>

#include <ccnx/common/ccnx_InterestReturn.h>
#include <ccnx/common/ccnx_ContentObject.h>

#include <ccnx/api/control/controlPlaneInterface.h>

int
athenaControl(Athena *athena, CCNxControl *control, PARCBitVector *ingressVector)
{
    /*
     * Management messages
     *
     *  Following is the start of a list of configuration messages that we need to handle.
     */
    //
    // ccnxMessage -> linkModule(parameters) // setup an interface
    //   -> TransportLinkAdapter -> listInterfaces, removeInterface
    //   -> ForwardingEngine -> addRoute, deleteRoute, contentStoreSize?
    //   -> Advertisement
    //   -> ListPIT
    //   -> Unsolicited message on channel XXX
    //
    bool commandResult = false;

    CpiOperation operation = cpi_GetMessageOperation(control);
    switch (operation) {
        case CPI_REGISTER_PREFIX:         // Add FIB route
            // Fall through
        case CPI_UNREGISTER_PREFIX: {     // Remove FIB route

            PARCBitVector *egressVector = parcBitVector_Create();

            CPIRouteEntry *cpiRouteEntry = cpiForwarding_RouteFromControlMessage(control);
            const CCNxName *prefix = cpiRouteEntry_GetPrefix(cpiRouteEntry);

            int linkId = cpiRouteEntry_GetInterfaceIndex(cpiRouteEntry);
            if (linkId == CPI_CURRENT_INTERFACE) {
                parcBitVector_SetVector(egressVector, ingressVector);
            } else {
                parcBitVector_Set(egressVector, linkId);
            }

            char *prefixString = ccnxName_ToString(prefix);
            unsigned interface = parcBitVector_NextBitSet(egressVector, 0);
            if (operation == CPI_REGISTER_PREFIX) {
                parcLog_Debug(athena->log, "Adding %s route to interface %d",
                              prefixString, interface);
                commandResult = athenaFIB_AddRoute(athena->athenaFIB, prefix, egressVector);
                if (!commandResult) {
                    parcLog_Warning(athena->log, "Unable to add route %s to interface %d",
                                    prefixString, interface);
                }

            } else { // Must be CPI_UNREGISTER_PREFIX
                parcLog_Debug(athena->log, "Removing %s route from interface %d",
                              prefixString, interface);
                commandResult = athenaFIB_DeleteRoute(athena->athenaFIB, prefix, egressVector);
                if (!commandResult) {
                    parcLog_Warning(athena->log, "Unable to remove route %s from interface %d",
                                    prefixString, interface);
                }
            }
            parcMemory_Deallocate(&prefixString);
            parcBitVector_Release(&egressVector);

            // Now send an ACK for the control message back to the sender.

            PARCJSON *json = ccnxControl_GetJson(control);
            parcJSON_AddBoolean(json, "RESULT", commandResult);

            PARCJSON *jsonAck = cpiAcks_CreateAck(json);

            CCNxControl *response = ccnxControl_CreateCPIRequest(jsonAck);
            parcJSON_Release(&jsonAck);

            athena_EncodeMessage(response);

            PARCBitVector *result =
                athenaTransportLinkAdapter_Send(athena->athenaTransportLinkAdapter, response, ingressVector);

            if (result) { // failed channels - client will resend interest unless we wish to optimize things here
                parcBitVector_Release(&result);
            }
            ccnxControl_Release(&response);
            cpiRouteEntry_Destroy(&cpiRouteEntry);

        } break;

        default:
            parcLog_Error(athena->log, "unknown operation %d", cpi_GetMessageOperation(control));
    }

    return commandResult == true? 0 : -1;
}

CCNxMetaMessage *
athenaControl_ProcessMessage(Athena *athena, const CCNxMetaMessage *message)
{
    return NULL;
}
