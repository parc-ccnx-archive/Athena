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
 * Athena Example Runtime implementation
 */

#include <config.h>
#include <pthread.h>
#include <unistd.h>

#include <ccnx/forwarder/athena/athena.h>
#include <ccnx/forwarder/athena/athena_Control.h>
#include <ccnx/forwarder/athena/athena_InterestControl.h>
#include <ccnx/forwarder/athena/athena_LRUContentStore.h>

#include <ccnx/common/ccnx_Interest.h>
#include <ccnx/common/ccnx_InterestReturn.h>
#include <ccnx/common/ccnx_ContentObject.h>

#include <ccnx/common/validation/ccnxValidation_CRC32C.h>
#include <ccnx/common/codec/ccnxCodec_TlvPacket.h>

static PARCLog *
_athena_logger_create(void)
{
    PARCFileOutputStream *fileOutput = parcFileOutputStream_Create(dup(STDOUT_FILENO));
    PARCOutputStream *output = parcFileOutputStream_AsOutputStream(fileOutput);
    parcFileOutputStream_Release(&fileOutput);

    PARCLogReporter *reporter = parcLogReporterFile_Create(output);
    parcOutputStream_Release(&output);

    PARCLog *log = parcLog_Create("localhost", "athena", NULL, reporter);
    parcLogReporter_Release(&reporter);

    parcLog_SetLevel(log, PARCLogLevel_Info);
    return log;
}

static void
_removeLink(void *context, PARCBitVector *linkVector)
{
    Athena *athena = (Athena *) context;

    const char *linkVectorString = parcBitVector_ToString(linkVector);

    // cleanup specified links from the FIB and PIT, these calls are currently presumed synchronous
    bool result = athenaFIB_RemoveLink(athena->athenaFIB, linkVector);
    assertTrue(result, "Failed to remove link from FIB %s", linkVectorString);

    result = athenaPIT_RemoveLink(athena->athenaPIT, linkVector);
    assertTrue(result, "Failed to remove link from PIT %s", linkVectorString);

    parcMemory_Deallocate(&linkVectorString);
}

static void
_athenaDestroy(Athena **athena)
{
    ccnxName_Release(&((*athena)->athenaName));
    athenaTransportLinkAdapter_Destroy(&((*athena)->athenaTransportLinkAdapter));
    athenaContentStore_Release(&((*athena)->athenaContentStore));
    athenaPIT_Release(&((*athena)->athenaPIT));
    athenaFIB_Release(&((*athena)->athenaFIB));
    parcLog_Release(&((*athena)->log));
}

parcObject_ExtendPARCObject(Athena, _athenaDestroy, NULL, NULL, NULL, NULL, NULL, NULL);

Athena *
athena_Create(size_t contentStoreSizeInMB)
{
    Athena *athena = parcObject_CreateAndClearInstance(Athena);

    athena->athenaName = ccnxName_CreateFromCString(CCNxNameAthena_Forwarder);
    assertNotNull(athena->athenaName, "Failed to create forwarder name (%s)", CCNxNameAthena_Forwarder);

    athena->athenaFIB = athenaFIB_Create();
    assertNotNull(athena->athenaFIB, "Failed to create FIB");

    athena->athenaPIT = athenaPIT_Create();
    assertNotNull(athena->athenaPIT, "Failed to create PIT");

    AthenaLRUContentStoreConfig storeConfig;
    storeConfig.capacityInMB = contentStoreSizeInMB;

    athena->athenaContentStore = athenaContentStore_Create(&AthenaContentStore_LRUImplementation, &storeConfig);
    assertNotNull(athena->athenaContentStore, "Failed to create Content Store");

    athena->athenaTransportLinkAdapter = athenaTransportLinkAdapter_Create(_removeLink, athena);
    assertNotNull(athena->athenaTransportLinkAdapter, "Failed to create Transport Link Adapter");

    athena->log = _athena_logger_create();
    athena->athenaState = Athena_Running;

    return athena;
}

parcObject_ImplementAcquire(athena, Athena);

parcObject_ImplementRelease(athena, Athena);

static void
_processInterestControl(Athena *athena, CCNxInterest *interest, PARCBitVector *ingressVector)
{
    //
    // Management messages
    //
    athenaInterestControl(athena, interest, ingressVector);
}

static void
_processControl(Athena *athena, CCNxControl *control, PARCBitVector *ingressVector)
{
    //
    // Management messages
    //
    athenaControl(athena, control, ingressVector);
}

static void
_processInterest(Athena *athena, CCNxInterest *interest, PARCBitVector *ingressVector)
{
    uint8_t hoplimit;

    //
    // *   (0) Hoplimit check, exclusively on interest messages
    //
    int linkId = parcBitVector_NextBitSet(ingressVector, 0);
    if (athenaTransportLinkAdapter_IsNotLocal(athena->athenaTransportLinkAdapter, linkId)) {
        hoplimit = ccnxInterest_GetHopLimit(interest);
        if (hoplimit == 0) {
            // We should never receive a message with a hoplimit of 0 from a non-local source.
            parcLog_Error(athena->log,
                          "Received a message with a hoplimit of zero from a non-local source (%s).",
                          athenaTransportLinkAdapter_LinkIdToName(athena->athenaTransportLinkAdapter, linkId));
            return;
        }
        ccnxInterest_SetHopLimit(interest, hoplimit - 1);
    }

    //
    // *   (1) if the interest is in the ContentStore, reply and return,
    //     assuming that other PIT entries were satisified when the content arrived.
    //
    CCNxMetaMessage *content = athenaContentStore_GetMatch(athena->athenaContentStore, interest);
    if (content) {
        const char *ingressVectorString = parcBitVector_ToString(ingressVector);
        parcLog_Debug(athena->log, "Forwarding content from store to %s", ingressVectorString);
        parcMemory_Deallocate(&ingressVectorString);
        PARCBitVector *result = athenaTransportLinkAdapter_Send(athena->athenaTransportLinkAdapter, content, ingressVector);
        if (result) { // failed channels - client will resend interest unless we wish to optimize things here
            parcBitVector_Release(&result);
        }
        return;
    }

    //
    // *   (2) add it to the PIT, if it was aggregated or there was an error we're done, otherwise we
    //         forward the interest.  The expectedReturnVector is populated with information we get from
    //         the FIB and used to verify content objects ingress ports when they arrive.
    //
    PARCBitVector *expectedReturnVector;
    AthenaPITResolution result;
    if ((result = athenaPIT_AddInterest(athena->athenaPIT, interest, ingressVector, &expectedReturnVector)) != AthenaPITResolution_Forward) {
        if (result == AthenaPITResolution_Error) {
            parcLog_Error(athena->log, "PIT resolution error");
        }
        return;
    }

    // Divert interests destined to the forwarder, we assume these are control messages
    CCNxName *ccnxName = ccnxInterest_GetName(interest);
    if (ccnxName_StartsWith(ccnxName, athena->athenaName) == true) {
        _processInterestControl(athena, interest, ingressVector);
        return;
    }

    //
    // *   (3) if it's in the FIB, forward, then update the PIT expectedReturnVector so we can verify
    //         when the returned object arrives that it came from an interface it was expected from.
    //         Interest messages with a hoplimit of 0 will never be sent out by the link adapter to a
    //         non-local interface so we need not check that here.
    //
    ccnxName = ccnxInterest_GetName(interest);
    PARCBitVector *egressVector = athenaFIB_Lookup(athena->athenaFIB, ccnxName, ingressVector);

    if (egressVector != NULL) {
        // If no links are in the egress vector the FIB returned, return a no route interest message
        if (parcBitVector_NumberOfBitsSet(egressVector) == 0) {
            CCNxInterestReturn *interestReturn = ccnxInterestReturn_Create(interest, CCNxInterestReturn_ReturnCode_NoRoute);
            PARCBitVector *result = athenaTransportLinkAdapter_Send(athena->athenaTransportLinkAdapter, interestReturn, ingressVector);
            parcBitVector_Release(&result);
            ccnxInterestReturn_Release(&interestReturn);
        } else {
            parcBitVector_SetVector(expectedReturnVector, egressVector);
            PARCBitVector *result = athenaTransportLinkAdapter_Send(athena->athenaTransportLinkAdapter, interest, egressVector);
            if (result) { // remove failed channels - client will resend interest unless we wish to optimize here
                parcBitVector_ClearVector(expectedReturnVector, result);
                parcBitVector_Release(&result);
            }
        }
        parcBitVector_Release(&egressVector);
    } else {
        // No FIB entry found, return a NoRoute interest return and remove the entry from the PIT.
        CCNxInterestReturn *interestReturn = ccnxInterestReturn_Create(interest, CCNxInterestReturn_ReturnCode_NoRoute);
        PARCBitVector *result = athenaTransportLinkAdapter_Send(athena->athenaTransportLinkAdapter, interestReturn, ingressVector);
        parcBitVector_Release(&result);
        ccnxInterestReturn_Release(&interestReturn);
        const char *name = ccnxName_ToString(ccnxName);
        if (athenaPIT_RemoveInterest(athena->athenaPIT, interest, ingressVector) != true) {
            parcLog_Error(athena->log, "Unable to remove interest (%s) from the PIT.", name);
        }
        parcLog_Debug(athena->log, "Name (%s) not found in FIB and no default route. Message dropped.", name);
        parcMemory_Deallocate(&name);
    }
}

static void
_processInterestReturn(Athena *athena, CCNxInterestReturn *interestReturn, PARCBitVector *ingressVector)
{
    // We can ignore interest return messages and allow the PIT entry to timeout, or
    //
    // Verify the return came from the next-hop where the interest was originally sent to
    // if not, ignore
    // otherwise, may try another forwarding path or clear the PIT state and forward the interest return on the reverse path
}

static void
_processContentObject(Athena *athena, CCNxContentObject *contentObject, PARCBitVector *ingressVector)
{
    //
    // *   (1) If it does not match anything in the PIT, drop it
    //
    PARCBitVector *egressVector = athenaPIT_Match(athena->athenaPIT, contentObject, ingressVector);
    if (egressVector) {
        if (parcBitVector_NumberOfBitsSet(egressVector) > 0) {
            //
            // *   (2) Add to the Content Store
            //
            athenaContentStore_PutContentObject(athena->athenaContentStore, contentObject);

            //
            // *   (3) Reverse path forward it via PIT entries
            //
            const char *egressVectorString = parcBitVector_ToString(egressVector);
            parcLog_Debug(athena->log, "Content Object forwarded to %s.", egressVectorString);
            parcMemory_Deallocate(&egressVectorString);
            PARCBitVector *result = athenaTransportLinkAdapter_Send(athena->athenaTransportLinkAdapter, contentObject, egressVector);
            if (result) {
                // if there are failed channels, client will resend interest unless we wish to retry here
                parcBitVector_Release(&result);
            }
        }
        parcBitVector_Release(&egressVector);
    }
}

void
athena_ProcessMessage(Athena *athena, CCNxMetaMessage *ccnxMessage, PARCBitVector *ingressVector)
{
    if (ccnxMetaMessage_IsInterest(ccnxMessage)) {
        const char *name = ccnxName_ToString(ccnxInterest_GetName(ccnxMessage));
        parcLog_Debug(athena->log, "Processing Interest Message: %s", name);
        parcMemory_Deallocate(&name);

        CCNxInterest *interest = ccnxMetaMessage_GetInterest(ccnxMessage);
        _processInterest(athena, interest, ingressVector);
        athena->stats.numProcessedInterests++;
    } else if (ccnxMetaMessage_IsContentObject(ccnxMessage)) {
        const char *name = ccnxName_ToString(ccnxContentObject_GetName(ccnxMessage));
        parcLog_Debug(athena->log, "Processing Content Object Message: %s", name);
        parcMemory_Deallocate(&name);

        CCNxContentObject *contentObject = ccnxMetaMessage_GetContentObject(ccnxMessage);
        _processContentObject(athena, contentObject, ingressVector);
        athena->stats.numProcessedContentObjects++;
    } else if (ccnxMetaMessage_IsControl(ccnxMessage)) {
        parcLog_Debug(athena->log, "Processing Control Message");

        CCNxControl *control = ccnxMetaMessage_GetControl(ccnxMessage);
        _processControl(athena, control, ingressVector);
        athena->stats.numProcessedControlMessages++;
    } else if (ccnxMetaMessage_IsInterestReturn(ccnxMessage)) {
        parcLog_Debug(athena->log, "Processing Interest Return Message");

        CCNxInterestReturn *interestReturn = ccnxMetaMessage_GetInterestReturn(ccnxMessage);
        _processInterestReturn(athena, interestReturn, ingressVector);
        athena->stats.numProcessedInterestReturns++;
    } else {
        trapUnexpectedState("Invalid CCNxMetaMessage type");
    }
}

void
athena_EncodeMessage(CCNxMetaMessage *message)
{
    PARCSigner *signer = ccnxValidationCRC32C_CreateSigner();
    CCNxCodecNetworkBufferIoVec *iovec = ccnxCodecTlvPacket_DictionaryEncode(message, signer);
    bool result = ccnxWireFormatMessage_PutIoVec(message, iovec);
    assertTrue(result, "ccnxWireFormatMessage_PutIoVec failed");
    ccnxCodecNetworkBufferIoVec_Release(&iovec);
    parcSigner_Release(&signer);
}

void *
athena_ForwarderEngine(void *arg)
{
    Athena *athena = (Athena *) arg;

    if (athena) {
        while (athena->athenaState == Athena_Running) {
            CCNxMetaMessage *ccnxMessage;
            PARCBitVector *ingressVector;
            int receiveTimeout = -1; // block until message received
            ccnxMessage = athenaTransportLinkAdapter_Receive(athena->athenaTransportLinkAdapter,
                                                             &ingressVector, receiveTimeout);
            if (ccnxMessage) {
                athena_ProcessMessage(athena, ccnxMessage, ingressVector);

                parcBitVector_Release(&ingressVector);
                ccnxMetaMessage_Release(&ccnxMessage);
            }
        }
        usleep(1000); // workaround for coordinating with test infrastructure
        athena_Release(&athena);
    }
    return NULL;
}
