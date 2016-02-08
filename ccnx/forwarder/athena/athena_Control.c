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

    switch (cpi_GetMessageOperation(control)) {
        // Add FIB route
        case CPI_REGISTER_PREFIX: {
            PARCBitVector *egressVector = parcBitVector_Create();

            CPIRouteEntry *cpiRouteEntry = cpiForwarding_RouteFromControlMessage(control);
            const CCNxName *prefix = cpiRouteEntry_GetPrefix(cpiRouteEntry);

            int linkId = cpiRouteEntry_GetInterfaceIndex(cpiRouteEntry);
            if (linkId == CPI_CURRENT_INTERFACE) {
                parcBitVector_SetVector(egressVector, ingressVector);
            } else {
                parcBitVector_Set(egressVector, linkId);
            }

            parcLog_Debug(athena->log, "Adding %s route to interface %d",
                          ccnxName_ToString(prefix), parcBitVector_NextBitSet(egressVector, 0));
            athenaFIB_AddRoute(athena->athenaFIB, prefix, egressVector);

            PARCJSON *json = ccnxControl_GetJson(control);
            PARCJSON *jsonAck = cpiAcks_CreateAck(json);

            CCNxControl *response = ccnxControl_CreateCPIRequest(jsonAck);
            parcJSON_Release(&jsonAck);

            athena_EncodeMessage(response);
            PARCBitVector *result = athenaTransportLinkAdapter_Send(athena->athenaTransportLinkAdapter, response, ingressVector);
            if (result) { // failed channels - client will resend interest unless we wish to optimize things here
                parcBitVector_Release(&result);
            }
            ccnxControl_Release(&response);
        } break;
        default:
            parcLog_Error(athena->log, "unknown operation %d", cpi_GetMessageOperation(control));
    }

    return 0;
}

CCNxMetaMessage *
athenaControl_ProcessMessage(Athena *athena, const CCNxMetaMessage *message)
{
    return NULL;
}
