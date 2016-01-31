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
#include <config.h>

#include <LongBow/runtime.h>

#include <errno.h>
#include <unistd.h>

#include <ccnx/forwarder/athena/athena_TransportLink.h>
#include <parc/algol/parc_Object.h>
#include <ccnx/common/ccnx_Interest.h>

/**
 * @typedef AthenaTransportLink
 * @brief Transport Link instance private data
 */
struct AthenaTransportLink {
    char *linkName;
    PARCLog *log;
    AthenaTransportLink_SendMethod *sendMethod;
    AthenaTransportLink_ReceiveMethod *receiveMethod;
    AthenaTransportLink_CloseMethod *closeMethod;
    int eventFd;
    AthenaTransportLinkEvent linkEvents;
    AthenaTransportLinkFlag linkFlags;
    int forceLocal;
    void *linkData;
    AthenaTransportLink_AddLinkCallback *addLink;
    AthenaTransportLink_AddLinkCallbackContext addLinkContext;
    AthenaTransportLink_RemoveLinkCallback *removeLink;
    AthenaTransportLink_RemoveLinkCallbackContext removeLinkContext;
    struct {
        size_t messageFromLink_Received;
        size_t messageFromLink_Empty;
        size_t messageFromLink_DroppedNoConnection;
        size_t messageToLink_Sent;
        size_t messageToLink_DroppedNoConnection;
        size_t link_Added;
        size_t link_Closed;
        size_t link_Removed;
    } stats;
};

static PARCLog *
_parc_logger_create(const char *name)
{
    PARCFileOutputStream *fileOutput = parcFileOutputStream_Create(dup(STDOUT_FILENO));
    PARCOutputStream *output = parcFileOutputStream_AsOutputStream(fileOutput);
    parcFileOutputStream_Release(&fileOutput);

    PARCLogReporter *reporter = parcLogReporterFile_Create(output);
    parcOutputStream_Release(&output);

    PARCLog *log = parcLog_Create("localhost", "athenaTransport", name, reporter);
    parcLogReporter_Release(&reporter);

    parcLog_SetLevel(log, PARCLogLevel_Info);
    return log;
}

static void
_destroy_link(AthenaTransportLink **athenaTransportLink)
{
    parcMemory_Deallocate(&((*athenaTransportLink)->linkName));
    parcLog_Release(&((*athenaTransportLink)->log));
}

parcObject_ExtendPARCObject(AthenaTransportLink, _destroy_link, NULL, NULL, NULL, NULL, NULL, NULL);

AthenaTransportLink *
athenaTransportLink_Clone(AthenaTransportLink *athenaTransportLink,
                          const char *name,
                          AthenaTransportLink_SendMethod *sendMethod,
                          AthenaTransportLink_ReceiveMethod *receiveMethod,
                          AthenaTransportLink_CloseMethod *closeMethod)
{
    AthenaTransportLink *newTransportLink = athenaTransportLink_Create(name, sendMethod, receiveMethod, closeMethod);

    if (newTransportLink != NULL) {
        newTransportLink->addLink = athenaTransportLink->addLink;
        newTransportLink->addLinkContext = athenaTransportLink->addLinkContext;
        newTransportLink->removeLink = athenaTransportLink->removeLink;
        newTransportLink->removeLinkContext = athenaTransportLink->removeLinkContext;
        newTransportLink->forceLocal = athenaTransportLink->forceLocal;
        newTransportLink->eventFd = -1;
        parcLog_SetLevel(newTransportLink->log, parcLog_GetLevel(athenaTransportLink->log));
    }

    return newTransportLink;
}

AthenaTransportLink *
athenaTransportLink_Create(const char *name,
                           AthenaTransportLink_SendMethod *sendMethod,
                           AthenaTransportLink_ReceiveMethod *receiveMethod,
                           AthenaTransportLink_CloseMethod *closeMethod)
{
    AthenaTransportLink *athenaTransportLink = parcObject_CreateAndClearInstance(AthenaTransportLink);

    if (athenaTransportLink != NULL) {
        athenaTransportLink->linkName = parcMemory_StringDuplicate(name, strlen(name));
        athenaTransportLink->sendMethod = sendMethod;
        athenaTransportLink->receiveMethod = receiveMethod;
        athenaTransportLink->closeMethod = closeMethod;
        athenaTransportLink->log = _parc_logger_create(name);
        athenaTransportLink->linkEvents = AthenaTransportLinkEvent_None;
        athenaTransportLink->linkFlags = AthenaTransportLinkFlag_None;
        athenaTransportLink->eventFd = -1;
    }

    return athenaTransportLink;
}

parcObject_ImplementAcquire(athenaTransportLink, AthenaTransportLink);

parcObject_ImplementRelease(athenaTransportLink, AthenaTransportLink);

PARCLog *
athenaTransportLink_GetLogger(AthenaTransportLink *athenaTransportLink)
{
    return athenaTransportLink->log;
}

int
athenaTransportLink_Send(AthenaTransportLink *athenaTransportLink, CCNxMetaMessage *ccnxMetaMessage)
{
    if (athenaTransportLink->sendMethod) {
        if (athenaTransportLink_GetEvent(athenaTransportLink) & AthenaTransportLinkEvent_Closing) {
            athenaTransportLink->stats.messageToLink_DroppedNoConnection++;
            errno = ENOTCONN;
            return -1;
        }
        athenaTransportLink->stats.messageToLink_Sent++;
        return athenaTransportLink->sendMethod(athenaTransportLink, ccnxMetaMessage);
    }
    return 0;
}

AthenaTransportLinkEvent
athenaTransportLink_GetEvent(AthenaTransportLink *athenaTransportLink)
{
    return athenaTransportLink->linkEvents;
}

void
athenaTransportLink_SetEvent(AthenaTransportLink *athenaTransportLink, AthenaTransportLinkEvent linkEvents)
{
    athenaTransportLink->linkEvents |= linkEvents;
}

void
athenaTransportLink_ClearEvent(AthenaTransportLink *athenaTransportLink, AthenaTransportLinkEvent linkEvents)
{
    athenaTransportLink->linkEvents &= ~linkEvents;
}

CCNxMetaMessage *
athenaTransportLink_Receive(AthenaTransportLink *athenaTransportLink)
{
    CCNxMetaMessage *ccnxMetaMessage = NULL;
    if (athenaTransportLink->receiveMethod) {
        if (athenaTransportLink_GetEvent(athenaTransportLink) & AthenaTransportLinkEvent_Closing) {
            athenaTransportLink->stats.messageFromLink_DroppedNoConnection++;
            errno = ENOTCONN;
            return NULL;
        }
        if (athenaTransportLink_GetEvent(athenaTransportLink) & AthenaTransportLinkEvent_Error) {
            athenaTransportLink_Close(athenaTransportLink);
            athenaTransportLink->stats.messageFromLink_DroppedNoConnection++;
            errno = ENOTCONN;
            return NULL;
        }
        // Turn off the Receive event flag since we're servicing it
        athenaTransportLink_ClearEvent(athenaTransportLink, AthenaTransportLinkEvent_Receive);
        ccnxMetaMessage = athenaTransportLink->receiveMethod(athenaTransportLink);
        if (ccnxMetaMessage == NULL) {
            athenaTransportLink->stats.messageFromLink_Empty++;
            errno = ENOMSG;
        } else {
            athenaTransportLink->stats.messageFromLink_Received++;
        }
    }
    return ccnxMetaMessage;
}

void
athenaTransportLink_RemoveLink(AthenaTransportLink *athenaTransportLink)
{
    if (athenaTransportLink->removeLink) {
        athenaTransportLink->stats.link_Removed++;
        athenaTransportLink->removeLink(athenaTransportLink->removeLinkContext, athenaTransportLink);
    }
}

void
athenaTransportLink_Close(AthenaTransportLink *athenaTransportLink)
{
    // XXX test and set
    if (athenaTransportLink_GetEvent(athenaTransportLink) & AthenaTransportLinkEvent_Closing) {
        return;
    }
    athenaTransportLink_SetEvent(athenaTransportLink, AthenaTransportLinkEvent_Closing);
    if (athenaTransportLink->closeMethod) {
        athenaTransportLink->stats.link_Closed++;
        athenaTransportLink->closeMethod(athenaTransportLink);
    }
    athenaTransportLink_RemoveLink(athenaTransportLink);
    athenaTransportLink_Release(&athenaTransportLink);
}

const char *
athenaTransportLink_GetName(AthenaTransportLink *athenaTransportLink)
{
    return athenaTransportLink->linkName;
}

void
athenaTransportLink_SetPrivateData(AthenaTransportLink *athenaTransportLink, void *linkData)
{
    athenaTransportLink->linkData = linkData;
}

void *
athenaTransportLink_GetPrivateData(AthenaTransportLink *athenaTransportLink)
{
    return athenaTransportLink->linkData;
}

int
athenaTransportLink_AddLink(AthenaTransportLink *athenaTransportLink, AthenaTransportLink *newTransportLink)
{
    if (athenaTransportLink->addLink) {
        athenaTransportLink->stats.link_Added++;
        return athenaTransportLink->addLink(athenaTransportLink->addLinkContext, newTransportLink);
    }
    return 0;
}

void
athenaTransportLink_SetAddLinkCallback(AthenaTransportLink *athenaTransportLink,
                                       AthenaTransportLink_AddLinkCallback *addLink,
                                       AthenaTransportLink_AddLinkCallbackContext addLinkContext)
{
    athenaTransportLink->addLink = addLink;
    athenaTransportLink->addLinkContext = addLinkContext;
}

void
athenaTransportLink_SetRemoveLinkCallback(AthenaTransportLink *athenaTransportLink,
                                          AthenaTransportLink_RemoveLinkCallback *removeLink,
                                          AthenaTransportLink_RemoveLinkCallbackContext removeLinkContext)
{
    athenaTransportLink->removeLink = removeLink;
    athenaTransportLink->removeLinkContext = removeLinkContext;
}

void
athenaTransportLink_SetEventFd(AthenaTransportLink *athenaTransportLink, int eventFd)
{
    athenaTransportLink->eventFd = eventFd;
}

int
athenaTransportLink_GetEventFd(AthenaTransportLink *athenaTransportLink)
{
    return athenaTransportLink->eventFd;
}

void
athenaTransportLink_SetRoutable(AthenaTransportLink *athenaTransportLink, bool isRoutable)
{
    if (isRoutable == true) {
        athenaTransportLink->linkFlags &= ~AthenaTransportLinkFlag_IsNotRoutable;
    } else {
        athenaTransportLink->linkFlags |= AthenaTransportLinkFlag_IsNotRoutable;
    }
}

bool
athenaTransportLink_IsNotRoutable(AthenaTransportLink *athenaTransportLink)
{
    return athenaTransportLink->linkFlags & AthenaTransportLinkFlag_IsNotRoutable;
}

bool
athenaTransportLink_IsForceLocal(AthenaTransportLink *athenaTransportLink)
{
    return athenaTransportLink->forceLocal;
}

void
athenaTransportLink_ForceLocal(AthenaTransportLink *athenaTransportLink, int forceLocal)
{
    athenaTransportLink->forceLocal = forceLocal;
}

void
athenaTransportLink_SetLocal(AthenaTransportLink *athenaTransportLink, bool isLocal)
{
    if (isLocal == true) {
        athenaTransportLink->linkFlags |= AthenaTransportLinkFlag_IsLocal;
    } else {
        athenaTransportLink->linkFlags &= ~AthenaTransportLinkFlag_IsLocal;
    }
}

bool
athenaTransportLink_IsNotLocal(AthenaTransportLink *athenaTransportLink)
{
    if (athenaTransportLink->forceLocal) {
        if (athenaTransportLink->forceLocal == AthenaTransportLink_ForcedLocal) {
            return false;
        } else {
            return true;
        }
    }
    if (athenaTransportLink->linkFlags & AthenaTransportLinkFlag_IsLocal) {
        return false;
    }
    return true;
}

void
athenaTransportLink_SetLogLevel(AthenaTransportLink *athenaTransportLink, const PARCLogLevel level)
{
    parcLog_SetLevel(athenaTransportLink->log, level);
}
