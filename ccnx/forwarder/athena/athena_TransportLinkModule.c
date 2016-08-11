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
#include <config.h>

#include <LongBow/runtime.h>

#include <errno.h>
#include <sys/param.h>
#include <unistd.h>

#include <ccnx/forwarder/athena/athena.h>
#include <ccnx/forwarder/athena/athena_TransportLinkModule.h>
#include <parc/logging/parc_LogReporterTextStdout.h>


/**
 * @typedef AthenaTransportLinkModule
 * @brief Link Specific Module
 */
struct AthenaTransportLinkModule {
    const char *name;
    PARCArrayList *instanceList; // list of modules active AthenaTransportLink instances
    AthenaTransportLinkModule_Open *openMethod;
    AthenaTransportLinkModule_Poll *pollMethod;
    PARCLog *log;
    AthenaTransportLinkModule_AddLinkCallback *addLink;
    AthenaTransportLinkModule_AddLinkCallbackContext addLinkContext;
    AthenaTransportLinkModule_RemoveLinkCallback *removeLink;
    AthenaTransportLinkModule_RemoveLinkCallbackContext removeLinkContext;
    struct {
        size_t module_Poll;
    } stats;
};

static PARCLog *
_parc_logger_create(const char *name)
{
    PARCLogReporter *reporter = parcLogReporterTextStdout_Create();
    PARCLog *log = parcLog_Create("localhost", "athenaTransportLinkModule", name, reporter);
    parcLogReporter_Release(&reporter);

    parcLog_SetLevel(log, PARCLogLevel_Info);
    return log;
}

PARCLog *
athenaTransportLinkModule_GetLogger(AthenaTransportLinkModule *athenaTransportLinkModule)
{
    return athenaTransportLinkModule->log;
}

AthenaTransportLinkModule *
athenaTransportLinkModule_Create(const char *name,
                                 AthenaTransportLinkModule_Open *openMethod,
                                 AthenaTransportLinkModule_Poll *pollMethod)
{
    AthenaTransportLinkModule *athenaTransportLinkModule = parcMemory_AllocateAndClear(sizeof(AthenaTransportLinkModule));
    assertNotNull(athenaTransportLinkModule, "parcMemory_AllocateAndClear failed to create a new AthenaTransportLinkModule");
    athenaTransportLinkModule->log = _parc_logger_create(name);

    athenaTransportLinkModule->name = parcMemory_StringDuplicate(name, strlen(name));
    assertNotNull(athenaTransportLinkModule->name, "parcMemory_AllocateAndClear failed allocate athenaTransportLinkModule name");

    athenaTransportLinkModule->instanceList = parcArrayList_Create((void (*)(void **))athenaTransportLink_Release);
    assertNotNull(athenaTransportLinkModule->instanceList, "athenaTransportLinkModule_TCP could not allocate instance list");

    athenaTransportLinkModule->openMethod = openMethod;
    athenaTransportLinkModule->pollMethod = pollMethod;

    return athenaTransportLinkModule;
}

void
athenaTransportLinkModule_Destroy(AthenaTransportLinkModule **athenaTransportLinkModule)
{
    int index = (int) parcArrayList_Size((*athenaTransportLinkModule)->instanceList);
    while (index-- > 0) {
        AthenaTransportLink *transportLink;
        transportLink = parcArrayList_Get((*athenaTransportLinkModule)->instanceList, 0);
        athenaTransportLink_Close(transportLink);
    }
    parcArrayList_Destroy(&((*athenaTransportLinkModule)->instanceList));
    parcMemory_Deallocate(&((*athenaTransportLinkModule)->name));
    parcLog_Release(&((*athenaTransportLinkModule)->log));
    parcMemory_Deallocate(athenaTransportLinkModule);
}

int
athenaTransportLinkModule_Poll(AthenaTransportLinkModule *athenaTransportLinkModule, int timeout)
{
    athenaTransportLinkModule->stats.module_Poll++;
    int events = 0;
    for (int index = 0; index < parcArrayList_Size(athenaTransportLinkModule->instanceList); index++) {
        AthenaTransportLink *transportLink = parcArrayList_Get(athenaTransportLinkModule->instanceList, index);
        assertNotNull(transportLink, "Unexpected Null entry in module instance list");
        events += athenaTransportLinkModule->pollMethod(transportLink, timeout);
    }
    return events;
}

/**
 * @abstract called from the link specific adapter to coordinate the addition of a link instance
 * @discussion
 *
 * @param [in] athenaTransportLink link adapter instance
 * @param [in] athenaTransportLink link instance to add
 * @return 0 if successful, -1 on failure with errno set to indicate error
 *
 * Example:
 * @code
 * {
 *
 * }
 * @endcode
 */
static int
_athenaTransportLinkModule_AddLink(AthenaTransportLinkModule *athenaTransportLinkModule, AthenaTransportLink *newTransportLink)
{
    // up call to add link to transport adapter
    int result = athenaTransportLinkModule->addLink(athenaTransportLinkModule->addLinkContext, newTransportLink);
    if (result == 0) {
        athenaTransportLink_Acquire(newTransportLink);
        parcArrayList_Add(athenaTransportLinkModule->instanceList, newTransportLink);
    }
    return result;
}

/**
 * @abstract called from the link specific adapter to coordinate termination of a link instance
 * @discussion
 *
 * This is called exclusively from the Transport Link specific Module to instigate the
 * removal of an active link.  The link is flagged as closing and the link module is called
 * to continue the operation.  Eventually this should result in the link specific close
 * method being called, which should finish cleaning state, and deallocate the instance.
 *
 * @param [in] athenaTransportLink link adapter instance
 * @param [in] athenaTransportLink link instance to remove
 *
 * Example:
 * @code
 * {
 *
 * }
 * @endcode
 */
static void
_athenaTransportLinkModule_RemoveLink(AthenaTransportLinkModule *athenaTransportLinkModule, AthenaTransportLink *athenaTransportLink)
{
    // remove from our list
    for (int index = 0; index < parcArrayList_Size(athenaTransportLinkModule->instanceList); index++) {
        AthenaTransportLink *transportLink = parcArrayList_Get(athenaTransportLinkModule->instanceList, index);
        if (athenaTransportLink == transportLink) {
            AthenaTransportLink *removedInstance = parcArrayList_RemoveAtIndex(athenaTransportLinkModule->instanceList, index);
            assertTrue(removedInstance == athenaTransportLink, "Wrong link removed");
            break;
        }
    }
    athenaTransportLinkModule->removeLink(athenaTransportLinkModule->removeLinkContext, athenaTransportLink);
    athenaTransportLink_Release(&athenaTransportLink);
}

AthenaTransportLink *
athenaTransportLinkModule_Open(AthenaTransportLinkModule *athenaTransportLinkModule, PARCURI *connectionURI)
{
    AthenaTransportLink *athenaTransportLink = athenaTransportLinkModule->openMethod(athenaTransportLinkModule, connectionURI);
    if (athenaTransportLink) {
        athenaTransportLink_SetAddLinkCallback(athenaTransportLink,
                                               (AthenaTransportLink_AddLinkCallback *) _athenaTransportLinkModule_AddLink,
                                               athenaTransportLinkModule);

        int result = _athenaTransportLinkModule_AddLink(athenaTransportLinkModule, athenaTransportLink);
        if (result == -1) {
            int addLinkError = errno;
            parcLog_Error(athenaTransportLinkModule_GetLogger(athenaTransportLinkModule),
                          "Adding link %s failed: %s", athenaTransportLink_GetName(athenaTransportLink), strerror(errno));
            athenaTransportLink_Close(athenaTransportLink);
            errno = addLinkError;
            return NULL;
        }

        athenaTransportLink_SetRemoveLinkCallback(athenaTransportLink,
                                                  (AthenaTransportLink_RemoveLinkCallback *) _athenaTransportLinkModule_RemoveLink,
                                                  athenaTransportLinkModule);
    }
    return athenaTransportLink;
}

const char *
athenaTransportLinkModule_GetName(AthenaTransportLinkModule *athenaTransportLinkModule)
{
    return athenaTransportLinkModule->name;
}

void
athenaTransportLinkModule_SetAddLinkCallback(AthenaTransportLinkModule *athenaTransportLinkModule,
                                             AthenaTransportLinkModule_AddLinkCallback *addLink,
                                             AthenaTransportLinkModule_AddLinkCallbackContext addLinkContext)
{
    athenaTransportLinkModule->addLink = addLink;
    athenaTransportLinkModule->addLinkContext = addLinkContext;
}

void
athenaTransportLinkModule_SetRemoveLinkCallback(AthenaTransportLinkModule *athenaTransportLinkModule,
                                                AthenaTransportLinkModule_RemoveLinkCallback *removeLink,
                                                AthenaTransportLinkModule_RemoveLinkCallbackContext removeLinkContext)
{
    athenaTransportLinkModule->removeLink = removeLink;
    athenaTransportLinkModule->removeLinkContext = removeLinkContext;
}

void
athenaTransportLinkModule_SetLogLevel(AthenaTransportLinkModule *athenaTransportLinkModule, const PARCLogLevel level)
{
    parcLog_SetLevel(athenaTransportLinkModule->log, level);
}

PARCBuffer *
athenaTransportLinkModule_CreateMessageBuffer(CCNxMetaMessage *message)
{
    PARCBuffer *buffer = ccnxWireFormatMessage_GetWireFormatBuffer(message);

    // If there is no PARCBuffer present, check for an IO vector and convert that into a contiguous buffer.
    if (buffer == NULL) {
        CCNxCodecNetworkBufferIoVec *iovec = ccnxWireFormatMessage_GetIoVec(message);
        if (iovec == NULL) { // if there's no iovec or buffer, encode the message and return the iovec
            athena_EncodeMessage(message);
            iovec = ccnxWireFormatMessage_GetIoVec(message);
        }
        assertNotNull(iovec, "Null io vector");
        size_t iovcnt = ccnxCodecNetworkBufferIoVec_GetCount((CCNxCodecNetworkBufferIoVec *) iovec);
        const struct iovec *array = ccnxCodecNetworkBufferIoVec_GetArray((CCNxCodecNetworkBufferIoVec *) iovec);

        // If it's a single vector wrap it in a buffer to avoid a copy
        if (iovcnt == 1) {
            buffer = parcBuffer_Wrap(array[0].iov_base, array[0].iov_len, 0, array[0].iov_len);
        } else {
            size_t totalbytes = 0;
            for (int i = 0; i < iovcnt; i++) {
                totalbytes += array[i].iov_len;
            }
            buffer = parcBuffer_Allocate(totalbytes);
            for (int i = 0; i < iovcnt; i++) {
                parcBuffer_PutArray(buffer, array[i].iov_len, array[i].iov_base);
            }
            parcBuffer_Flip(buffer);
        }
    } else {
        buffer = parcBuffer_Acquire(buffer);
    }

    return buffer;
}

CCNxCodecNetworkBufferIoVec *
athenaTransportLinkModule_GetMessageIoVector(CCNxMetaMessage *message)
{
    CCNxCodecNetworkBufferIoVec *iovec = ccnxWireFormatMessage_GetIoVec(message);

    // If there was no io vector present, check for a buffer and convert that into an iovec
    if (iovec == NULL) {
        PARCBuffer *buffer = ccnxWireFormatMessage_GetWireFormatBuffer(message);
        if (buffer == NULL) { // if there's no iovec or buffer, encode the message and return the iovec
            athena_EncodeMessage(message);
            iovec = ccnxWireFormatMessage_GetIoVec(message);
            iovec = ccnxCodecNetworkBufferIoVec_Acquire(iovec);
        } else {
            CCNxCodecNetworkBuffer *netbuff = ccnxCodecNetworkBuffer_Create(&ParcMemoryMemoryBlock, NULL);
            assertNotNull(netbuff, "Null network buffer allocation");
            parcBuffer_SetPosition(buffer, 0);
            ccnxCodecNetworkBuffer_PutBuffer(netbuff, buffer);
            iovec = ccnxCodecNetworkBuffer_CreateIoVec(netbuff);
            ccnxCodecNetworkBuffer_Release(&netbuff);
        }
    } else {
        iovec = ccnxCodecNetworkBufferIoVec_Acquire(iovec);
    }

    return iovec;
}
