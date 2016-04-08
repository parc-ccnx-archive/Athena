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
 * The TransportLinkAdapter is an abstracted interface for managing
 * and interacting with transport links (sometimes referred to as
 * connections).  Management consists of creating links, scanning links
 * for events and polling modules.  The TransportLinkAdapter does not
 * create links itself, but relies on a protocol specific TransportLinkModule
 * to establish new ones.
 *
 * Links are created by passing protocol specific arguments to a
 * protocol specific TransportLinkModule.  The TransportLinkModule
 * creates new TransportLink instances with link specific methods for
 * sending and receiving messages and passes new instances onto the
 * TransportLinkAdapter for link identifier assignment and scheduling.
 *
 * Once a link a link identifier is established by the TransportLinkAdapter
 * messages can then be sent through that link by referencing its link
 * identifier.  When a request is made to receive a message, the
 * TransportLinkAdapter scans its list of registered links for events
 * and passes control back to each link until a message is returned,
 * passing the message along with its ingress link identifier to
 * the caller.
 *
 * When no further events are found, the TransportLinkAdapter polls
 * each TransportLinkModule and then sets back to scans its list of
 * link instances from the beginning.
 *
 * TransportLink instances are not required to pass back a message
 * from the servicing of a receive event.  For example, a listener
 * coule be established for creating new connections.  A read event
 * on that link would involve establishing a new link and passing it to the
 * TransportLinkAdapter to be registered.  No message would need to be passed
 * back unless the module wished to send a message regarding the new connection.
 *
 */

//
//  TransportLinkAdapter -> Open("tcp", "TCP_1", "127.0.0.1:9695")
//      TransportLinkModule -> TCPOpen("TCP_1", "127.0.0.1:9695") -> TransportLink
//                          -> TransportLinkModule_AddLink -> TransportLinkAdapter_AddLink -> (linkId == 1)
//
//  TransportLinkAdapter -> Send(message, 1) -> TCPSend(message)
//  TransportLinkAdapter -> Receive() -> (event on link 1) TCPReceive() -> CCNxMetaMessage
//

#include <config.h>

#include <LongBow/runtime.h>

#include <errno.h>
#include <poll.h>
#include <unistd.h>

#include <ccnx/forwarder/athena/athena.h>
#include <ccnx/forwarder/athena/athena_TransportLinkAdapter.h>

typedef PARCArrayList *(*ModuleInit)(void);

/**
 * @typedef AthenaTransportLinkAdapter
 * @brief Link Adapter Transport private data
 */
struct AthenaTransportLinkAdapter {
    PARCArrayList *moduleList;   // list of available AthenaTransportLinkModule link modules
    PARCArrayList *instanceList; // list of active AthenaTransportLink instances
    PARCArrayList *listenerList; // list of listening AthenaTransportLink instances
    struct pollfd *pollfdReceiveList;
    struct pollfd *pollfdSendList;
    AthenaTransportLink **pollfdTransportLink;
    int pollfdListSize;
    void (*removeLink)(AthenaTransportLinkAdapter_RemoveLinkCallbackContext removeLinkContext, PARCBitVector *parcBitVector);
    AthenaTransportLinkAdapter_RemoveLinkCallbackContext removeLinkContext;
    int nextLinkToRead;
    PARCLog *log;
    struct {
        size_t messageSent;
        size_t messageSend_Attempted;
        size_t messageSend_HopLimitExceeded;
        size_t messageSend_LinkDoesNotExist;
        size_t messageSend_LinkNotAcceptingSendRequests;
        size_t messageSend_LinkSendFailed;
        size_t messageReceived;
        size_t messageReceive_Attempted;
        size_t messageReceive_LinkDoesNotExist;
        size_t messageReceive_NoMessage;
    } stats;
};

void
athenaTransportLinkAdapter_Destroy(AthenaTransportLinkAdapter **athenaTransportLinkAdapter)
{
    // release listener instances
    if ((*athenaTransportLinkAdapter)->listenerList) {
        for (int index = 0; index < parcArrayList_Size((*athenaTransportLinkAdapter)->listenerList); index++) {
            AthenaTransportLink *athenaTransportLink = parcArrayList_Get((*athenaTransportLinkAdapter)->listenerList, index);
            athenaTransportLink_Close(athenaTransportLink);
        }
    }
    // release live instances
    if ((*athenaTransportLinkAdapter)->instanceList) {
        for (int index = 0; index < parcArrayList_Size((*athenaTransportLinkAdapter)->instanceList); index++) {
            AthenaTransportLink *athenaTransportLink = parcArrayList_Get((*athenaTransportLinkAdapter)->instanceList, index);
            if (athenaTransportLink) {
                athenaTransportLink_Close(athenaTransportLink);
            }
        }
    }
    parcArrayList_Destroy(&((*athenaTransportLinkAdapter)->moduleList));
    parcArrayList_Destroy(&((*athenaTransportLinkAdapter)->instanceList));
    parcArrayList_Destroy(&((*athenaTransportLinkAdapter)->listenerList));
    if ((*athenaTransportLinkAdapter)->pollfdReceiveList) {
        parcMemory_Deallocate(&((*athenaTransportLinkAdapter)->pollfdReceiveList));
        parcMemory_Deallocate(&((*athenaTransportLinkAdapter)->pollfdSendList));
        parcMemory_Deallocate(&((*athenaTransportLinkAdapter)->pollfdTransportLink));
    }
    parcLog_Release(&((*athenaTransportLinkAdapter)->log));
    parcMemory_Deallocate(athenaTransportLinkAdapter);
}

PARCLog *
athenaTransportLinkAdapter_GetLogger(AthenaTransportLinkAdapter *athenaTransportLinkAdapter)
{
    return athenaTransportLinkAdapter->log;
}

static PARCLog *
_parc_logger_create(void)
{
    PARCFileOutputStream *fileOutput = parcFileOutputStream_Create(dup(STDOUT_FILENO));
    PARCOutputStream *output = parcFileOutputStream_AsOutputStream(fileOutput);
    parcFileOutputStream_Release(&fileOutput);

    PARCLogReporter *reporter = parcLogReporterFile_Create(output);
    parcOutputStream_Release(&output);

    PARCLog *log = parcLog_Create("localhost", "athenaTransportLinkAdapter", NULL, reporter);
    parcLogReporter_Release(&reporter);

    parcLog_SetLevel(log, PARCLogLevel_Info);
    return log;
}

AthenaTransportLinkAdapter *
athenaTransportLinkAdapter_Create(void (*removeLinkCallback)(void *removeLinkContext, PARCBitVector *parcBitVector), AthenaTransportLinkAdapter_RemoveLinkCallbackContext removeLinkContext)
{
    AthenaTransportLinkAdapter *athenaTransportLinkAdapter = parcMemory_AllocateAndClear(sizeof(AthenaTransportLinkAdapter));
    assertNotNull(athenaTransportLinkAdapter, "parcMemory_AllocateAndClear failed to create a new AthenaTransportLinkAdapter");
    athenaTransportLinkAdapter->moduleList = parcArrayList_Create((void (*)(void **))athenaTransportLinkModule_Destroy);
    athenaTransportLinkAdapter->instanceList = parcArrayList_Create(NULL);
    athenaTransportLinkAdapter->listenerList = parcArrayList_Create(NULL);
    athenaTransportLinkAdapter->nextLinkToRead = 0;
    athenaTransportLinkAdapter->pollfdListSize = 0;
    athenaTransportLinkAdapter->removeLink = removeLinkCallback;
    athenaTransportLinkAdapter->removeLinkContext = removeLinkContext;
    athenaTransportLinkAdapter->log = _parc_logger_create();

    return athenaTransportLinkAdapter;
}

static AthenaTransportLinkModule *
_LookupModule(AthenaTransportLinkAdapter *athenaTransportLinkAdapter, const char *moduleName)
{
    AthenaTransportLinkModule *athenaTransportLinkModule;
    if (athenaTransportLinkAdapter->moduleList) {
        for (int index = 0; index < parcArrayList_Size(athenaTransportLinkAdapter->moduleList); index++) {
            athenaTransportLinkModule = parcArrayList_Get(athenaTransportLinkAdapter->moduleList, index);
            if (strcasecmp(athenaTransportLinkModule_GetName(athenaTransportLinkModule), moduleName) == 0) {
                return athenaTransportLinkModule;
            }
        }
    }
    errno = ENOENT;
    return NULL;
}

static void
_add_to_pollfdList(AthenaTransportLinkAdapter *athenaTransportLinkAdapter, AthenaTransportLink *newTransportLink, int eventFd)
{
    int index;

    // Check for an existing availble slot
    for (index = 0; index < athenaTransportLinkAdapter->pollfdListSize; index++) {
        if (athenaTransportLinkAdapter->pollfdTransportLink[index] == NULL) {
            athenaTransportLinkAdapter->pollfdTransportLink[index] = newTransportLink;
            athenaTransportLinkAdapter->pollfdReceiveList[index].fd = eventFd;
            athenaTransportLinkAdapter->pollfdReceiveList[index].events = POLLIN;
            athenaTransportLinkAdapter->pollfdSendList[index].fd = eventFd;
            athenaTransportLinkAdapter->pollfdSendList[index].events = POLLOUT;
            return;
        }
    }

    // Create a new entry
    if (index == athenaTransportLinkAdapter->pollfdListSize) {
        struct pollfd *newReceiveList;
        newReceiveList = parcMemory_Reallocate(athenaTransportLinkAdapter->pollfdReceiveList, sizeof(struct pollfd) * (index + 1));
        assertNotNull(newReceiveList, "parcMemory_Reallocate failed to resize the pollfdReceiveList");
        athenaTransportLinkAdapter->pollfdReceiveList = newReceiveList;

        struct pollfd *newSendList;
        newSendList = parcMemory_Reallocate(athenaTransportLinkAdapter->pollfdSendList, sizeof(struct pollfd) * (index + 1));
        assertNotNull(newSendList, "parcMemory_Reallocate failed to resize the pollfdSendList");
        athenaTransportLinkAdapter->pollfdSendList = newSendList;

        AthenaTransportLink **newPollFdTransportLink = parcMemory_Reallocate(athenaTransportLinkAdapter->pollfdTransportLink,
                                                                             sizeof(AthenaTransportLink *) * (index + 1));
        assertNotNull(newPollFdTransportLink, "parcMemory_Reallocate failed to resize the pollfdTransportLink list");
        athenaTransportLinkAdapter->pollfdTransportLink = newPollFdTransportLink;

        athenaTransportLinkAdapter->pollfdListSize = index + 1;
        athenaTransportLinkAdapter->pollfdTransportLink[index] = newTransportLink;
        athenaTransportLinkAdapter->pollfdReceiveList[index].fd = eventFd;
        athenaTransportLinkAdapter->pollfdReceiveList[index].events = POLLIN;
        athenaTransportLinkAdapter->pollfdSendList[index].fd = eventFd;
        athenaTransportLinkAdapter->pollfdSendList[index].events = POLLOUT;
    }
}

static void
_remove_from_pollfdList(AthenaTransportLinkAdapter *athenaTransportLinkAdapter, AthenaTransportLink *athenaTransportLink)
{
    for (int index = 0; index < athenaTransportLinkAdapter->pollfdListSize; index++) {
        if (athenaTransportLinkAdapter->pollfdTransportLink[index] == athenaTransportLink) {
            athenaTransportLinkAdapter->pollfdTransportLink[index] = NULL; // disable link callback
            athenaTransportLinkAdapter->pollfdReceiveList[index].fd = -1; // disable polling
            athenaTransportLinkAdapter->pollfdReceiveList[index].events = 0;
            athenaTransportLinkAdapter->pollfdSendList[index].fd = -1; // disable polling
            athenaTransportLinkAdapter->pollfdSendList[index].events = 0;
        }
    }
}

/**
 * @abstract add a new link instance to the AthenaTransportLinkAdapter instance list
 * @discussion
 *
 * When athenaTransportLinkAdapter_Open is called on a link specific module, the module instantiates
 * the link and then creates a new AthenaTransportLink to interface with the AthenaTransportLinkAdapter.
 * The AthenaTransportLinkModule passes the new AthenaTransportLink to the AthenaTransportLinkAdapter by
 * calling AthenaTransportLinkAdapter_AddLink with the new link AthenaTransportLink data. The AthenaTransportLinkAdapter
 * then places the new link instance on its internal instance list in a pending state until it has been verified and ensures
 * that the new link doesn't collide (i.e. by name) with any currently registered link before returning success.
 *
 * New routable links are placed into instanceList slots that have previously been vacated before being added
 * to the end of the instanceList.  This is in order to keep bit vectors that are based on the instanceList as
 * small as is necessary.
 *
 * @param [in] athenaTransportLinkAdapter link adapter instance
 * @param [in] linkInstance instance structure created by the link specific module
 * @return 0 on success, -1 with errno set to indicate the error
 *
 * Example:
 * @code
 * {
 * }
 * @endcode
 */
static int
_athenaTransportLinkAdapter_AddLink(AthenaTransportLinkAdapter *athenaTransportLinkAdapter, AthenaTransportLink *newTransportLink)
{
    int linkId = -1;

    // Check for existing linkName in listenerList
    if (athenaTransportLinkAdapter->listenerList) {
        for (int index = 0; index < parcArrayList_Size(athenaTransportLinkAdapter->listenerList); index++) {
            AthenaTransportLink *athenaTransportLink = parcArrayList_Get(athenaTransportLinkAdapter->listenerList, index);
            if (strcmp(athenaTransportLink_GetName(athenaTransportLink), athenaTransportLink_GetName(newTransportLink)) == 0) {
                errno = EADDRINUSE; // name is already in listenerList
                return -1;
            }
        }
    }

    // Check for existing linkName in the instanceList
    if (athenaTransportLinkAdapter->instanceList) {
        for (int index = 0; index < parcArrayList_Size(athenaTransportLinkAdapter->instanceList); index++) {
            AthenaTransportLink *athenaTransportLink = parcArrayList_Get(athenaTransportLinkAdapter->instanceList, index);
            if (athenaTransportLink) {
                if (strcmp(athenaTransportLink_GetName(athenaTransportLink), athenaTransportLink_GetName(newTransportLink)) == 0) {
                    errno = EADDRINUSE; // name is already in instanceList
                    return -1;
                }
            } else {
                // remember the first available index found along the way
                if (linkId == -1) {
                    linkId = index;
                }
            }
        }
    }

    // Add to listenerList or instanceList
    athenaTransportLink_Acquire(newTransportLink);
    if (athenaTransportLink_IsNotRoutable(newTransportLink)) { // listener
        bool result = parcArrayList_Add(athenaTransportLinkAdapter->listenerList, newTransportLink);
        assertTrue(result, "parcArrayList_Add failed to add new listener");
    } else { // routable link, add to instances using the last available id if one was seen
        if (linkId != -1) {
            parcArrayList_Set(athenaTransportLinkAdapter->instanceList, linkId, newTransportLink);
        } else {
            bool result = parcArrayList_Add(athenaTransportLinkAdapter->instanceList, newTransportLink);
            assertTrue(result, "parcArrayList_Add failed to add new link instance");
        }
    }

    // If any transport link has a registered file descriptor add it to the general polling list.
    int eventFd = athenaTransportLink_GetEventFd(newTransportLink);
    if (eventFd != -1) {
        _add_to_pollfdList(athenaTransportLinkAdapter, newTransportLink, eventFd);
    }
    return 0;
}

/**
 * @abstract called from below the link adapter to coordinate termination of a link instance
 * @discussion
 *
 * This is called exclusively from the Transport Link Module to instigate the removal of
 * an active link.  The link has been flagged closing by the originator.  This method
 * must ensure that all references to the link have been removed and then call the
 * instance close method to finish the operation.
 *
 * @param [in] athenaTransportLinkAdapter link adapter instance
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
_athenaTransportLinkAdapter_RemoveLink(AthenaTransportLinkAdapter *athenaTransportLinkAdapter, AthenaTransportLink *athenaTransportLink)
{
    int linkId = -1;

    // if this is a listener it can simply be removed
    if (athenaTransportLink_IsNotRoutable(athenaTransportLink)) {
        if (athenaTransportLinkAdapter->listenerList) {
            for (int index = 0; index < parcArrayList_Size(athenaTransportLinkAdapter->listenerList); index++) {
                AthenaTransportLink *transportLink = parcArrayList_Get(athenaTransportLinkAdapter->listenerList, index);
                if (athenaTransportLink == transportLink) {
                    parcArrayList_RemoveAtIndex(athenaTransportLinkAdapter->listenerList, index);
                    _remove_from_pollfdList(athenaTransportLinkAdapter, athenaTransportLink);
                    parcLog_Debug(athenaTransportLinkAdapter_GetLogger(athenaTransportLinkAdapter), "listener removed: %s",
                                  athenaTransportLink_GetName(athenaTransportLink));
                    athenaTransportLink_Release(&athenaTransportLink);
                    return;
                }
            }
        }
    }

    // Remove from our internal instance list.
    // The index entry remains to be reused by links that are added in the future.
    if (athenaTransportLinkAdapter->instanceList) {
        for (int index = 0; index < parcArrayList_Size(athenaTransportLinkAdapter->instanceList); index++) {
            AthenaTransportLink *transportLink = parcArrayList_Get(athenaTransportLinkAdapter->instanceList, index);
            if (athenaTransportLink == transportLink) {
                parcArrayList_Set(athenaTransportLinkAdapter->instanceList, index, NULL);
                _remove_from_pollfdList(athenaTransportLinkAdapter, athenaTransportLink);
                linkId = index;
                break;
            }
        }
    }

    assertFalse(linkId == -1, "Attempt to remove link not found in link adapter lists");

    // Callback to notify that the link has been removed and references need to be dropped.
    PARCBitVector *linkVector = parcBitVector_Create();
    parcBitVector_Set(linkVector, linkId);
    athenaTransportLinkAdapter->removeLink(athenaTransportLinkAdapter->removeLinkContext, linkVector);
    parcBitVector_Release(&linkVector);

    // we assume all references to the linkId associated with this instance have been
    // cleared from the PIT and FIB when removeLink returns.

    parcLog_Debug(athenaTransportLinkAdapter_GetLogger(athenaTransportLinkAdapter),
                  "link removed: %s", athenaTransportLink_GetName(athenaTransportLink));

    athenaTransportLink_Release(&athenaTransportLink);
}

static void
_AddModule(AthenaTransportLinkAdapter *athenaTransportLinkAdapter, AthenaTransportLinkModule *newTransportLinkModule)
{
    athenaTransportLinkModule_SetAddLinkCallback(newTransportLinkModule,
                                                 (AthenaTransportLinkModule_AddLinkCallback *) _athenaTransportLinkAdapter_AddLink,
                                                 athenaTransportLinkAdapter);
    athenaTransportLinkModule_SetRemoveLinkCallback(newTransportLinkModule,
                                                    (AthenaTransportLinkModule_RemoveLinkCallback *) _athenaTransportLinkAdapter_RemoveLink,
                                                    athenaTransportLinkAdapter);

    bool result = parcArrayList_Add(athenaTransportLinkAdapter->moduleList, newTransportLinkModule);
    assertTrue(result, "parcArrayList_Add of module failed");
}

// This method is currently unused, but is available and is still diligently tested
#ifdef __GNUC__
__attribute__ ((unused))
#else
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#endif
static int
_RemoveModule(AthenaTransportLinkAdapter *athenaTransportLinkAdapter, const char *moduleName)
{
    AthenaTransportLinkModule *athenaTransportLinkModule;
    if (athenaTransportLinkAdapter->moduleList) {
        for (int index = 0; index < parcArrayList_Size(athenaTransportLinkAdapter->moduleList); index++) {
            athenaTransportLinkModule = parcArrayList_Get(athenaTransportLinkAdapter->moduleList, index);
            if (strcasecmp(athenaTransportLinkModule_GetName(athenaTransportLinkModule), moduleName) == 0) {
                athenaTransportLinkModule_Destroy(&athenaTransportLinkModule);
                parcArrayList_RemoveAtIndex(athenaTransportLinkAdapter->moduleList, index);
                return 0;
            }
        }
    }
    errno = ENOENT;
    return -1;
}
#ifndef __GNUC__
#pragma GCC diagnostic pop
#endif

#include <ctype.h>

static const char *
_strtoupper(const char *string)
{
    char *upperCaseString = parcMemory_StringDuplicate(string, strlen(string));
    for (char *i = upperCaseString; *i; i++) {
        *i = toupper(*i);
    }
    return upperCaseString;
}

#define METHOD_PREFIX "athenaTransportLinkModule"
#define INIT_METHOD_SUFFIX "_Init"

static const char *
_moduleNameToInitMethod(const char *moduleName)
{
    char *result = NULL;

    assertNotNull(moduleName, "module name must not be null");
    const char *module = _strtoupper(moduleName);
    PARCBufferComposer *composer = parcBufferComposer_Create();
    if (composer != NULL) {
        parcBufferComposer_Format(composer, "%s%s%s",
                                  METHOD_PREFIX, module, INIT_METHOD_SUFFIX);
        PARCBuffer *tempBuffer = parcBufferComposer_ProduceBuffer(composer);
        parcBufferComposer_Release(&composer);

        result = parcBuffer_ToString(tempBuffer);
        parcBuffer_Release(&tempBuffer);
    }

    parcMemory_Deallocate(&module);
    return result;
}

#define LIBRARY_MODULE_PREFIX "libathena_"
#ifdef __linux__
#define LIBRARY_MODULE_SUFFIX ".so"
#else // MacOS
#define LIBRARY_MODULE_SUFFIX ".dylib"
#endif

static const char *
_moduleNameToLibrary(const char *moduleName)
{
    char *result = NULL;

    assertNotNull(moduleName, "module name must not be null");
    const char *module = _strtoupper(moduleName);
    PARCBufferComposer *composer = parcBufferComposer_Create();
    if (composer != NULL) {
        parcBufferComposer_Format(composer, "%s%s%s",
                                  LIBRARY_MODULE_PREFIX, module, LIBRARY_MODULE_SUFFIX);
        PARCBuffer *tempBuffer = parcBufferComposer_ProduceBuffer(composer);
        parcBufferComposer_Release(&composer);

        result = parcBuffer_ToString(tempBuffer);
        parcBuffer_Release(&tempBuffer);
    }

    parcMemory_Deallocate(&module);
    return result;
}

#include <dlfcn.h>

static AthenaTransportLinkModule *
_LoadModule(AthenaTransportLinkAdapter *athenaTransportLinkAdapter, const char *moduleName)
{
    assertTrue(_LookupModule(athenaTransportLinkAdapter, moduleName) == NULL,
               "attempt to load an already loaded module");

    // Derive the entry initialization name from the provided module name
    const char *moduleEntry;
    moduleEntry = _moduleNameToInitMethod(moduleName);

    // Check to see if the module was statically linked in.
    void *linkModule = RTLD_DEFAULT;
    ModuleInit _init = dlsym(linkModule, moduleEntry);

    // If not statically linked in, look for a shared library and load it from there
    if (_init == NULL) {
        // Derive the library name from the provided module name
        const char *moduleLibrary;
        moduleLibrary = _moduleNameToLibrary(moduleName);

        void *linkModule = dlopen(moduleLibrary, RTLD_NOW | RTLD_GLOBAL);
        parcMemory_Deallocate(&moduleLibrary);

        // If the shared library wasn't found, look for the symbol in our existing image.  This
        // allows a link module to be linked directly into Athena without modifying the forwarder.
        if (linkModule == NULL) {
            parcLog_Error(athenaTransportLinkAdapter_GetLogger(athenaTransportLinkAdapter),
                          "Unable to dlopen %s: %s", moduleName, dlerror());
            parcMemory_Deallocate(&moduleEntry);
            errno = ENOENT;
            return NULL;
        }

        _init = dlsym(linkModule, moduleEntry);
        if (_init == NULL) {
            parcLog_Error(athenaTransportLinkAdapter_GetLogger(athenaTransportLinkAdapter),
                          "Unable to find %s module _init method: %s", moduleName, dlerror());
            parcMemory_Deallocate(&moduleEntry);
            dlclose(linkModule);
            errno = ENOENT;
            return NULL;
        }
    }
    parcMemory_Deallocate(&moduleEntry);

    // Call the initialization method.
    PARCArrayList *moduleList = _init();
    if (moduleList == NULL) { // if the init method fails, unload the module if it was loaded
        parcLog_Error(athenaTransportLinkAdapter_GetLogger(athenaTransportLinkAdapter),
                      "Empty module list returned from %s module", moduleName);
        if (linkModule != RTLD_DEFAULT) {
            dlclose(linkModule);
        }
        errno = ENOENT;
        return NULL;
    }

    // Process each link module instance (typically only one)
    for (int index = 0; index < parcArrayList_Size(moduleList); index++) {
        AthenaTransportLinkModule *athenaTransportLinkModule = parcArrayList_Get(moduleList, index);
        _AddModule(athenaTransportLinkAdapter, athenaTransportLinkModule);
    }
    parcArrayList_Destroy(&moduleList);

    return _LookupModule(athenaTransportLinkAdapter, moduleName);
}

const char *
athenaTransportLinkAdapter_Open(AthenaTransportLinkAdapter *athenaTransportLinkAdapter, PARCURI *connectionURI)
{
    AthenaTransportLinkModule *athenaTransportLinkModule;
    const char *moduleName = parcURI_GetScheme(connectionURI);

    if (moduleName == NULL) {
        errno = EINVAL;
        return NULL;
    }

    athenaTransportLinkModule = _LookupModule(athenaTransportLinkAdapter, moduleName);
    if (athenaTransportLinkModule == NULL) {
        athenaTransportLinkModule = _LoadModule(athenaTransportLinkAdapter, moduleName);
        if (athenaTransportLinkModule == NULL) {
            errno = ENOENT;
            return NULL;
        }
    }

    AthenaTransportLink *athenaTransportLink = athenaTransportLinkModule_Open(athenaTransportLinkModule, connectionURI);
    if (athenaTransportLink == NULL) {
        return NULL;
    }

    return athenaTransportLink_GetName(athenaTransportLink);
}

int
athenaTransportLinkAdapter_Poll(AthenaTransportLinkAdapter *athenaTransportLinkAdapter, int timeout)
{
    struct pollfd *pollfdReceiveList = athenaTransportLinkAdapter->pollfdReceiveList;
    struct pollfd *pollfdSendList = athenaTransportLinkAdapter->pollfdSendList;
    int pollfdListSize = athenaTransportLinkAdapter->pollfdListSize;
    AthenaTransportLinkModule *athenaTransportLinkModule;
    int events = 0;

    // Allow instances which have not registered an eventfd to mark their events
    if (athenaTransportLinkAdapter->moduleList) {
        for (int index = 0; index < parcArrayList_Size(athenaTransportLinkAdapter->moduleList); index++) {
            athenaTransportLinkModule = parcArrayList_Get(athenaTransportLinkAdapter->moduleList, index);
            events += athenaTransportLinkModule_Poll(athenaTransportLinkModule, timeout);
        }
    }

    if (events) { // if we have existing events, poll doesn't need to block
        timeout = 0;
    }

    int result = poll(pollfdReceiveList, pollfdListSize, timeout);
    if (result < 0) {
        parcLog_Error(athenaTransportLinkAdapter_GetLogger(athenaTransportLinkAdapter),
                      "Receive list poll error: (%d) %s", errno, strerror(errno));
    } else {
        for (int index = 0; index < pollfdListSize; index++) {
            if (pollfdReceiveList[index].revents) {
                AthenaTransportLink *athenaTransportLink = athenaTransportLinkAdapter->pollfdTransportLink[index];
                if (athenaTransportLink) {
                    if (pollfdReceiveList[index].revents & (POLLERR | POLLHUP)) {
                        athenaTransportLink_SetEvent(athenaTransportLink, AthenaTransportLinkEvent_Error);
                        athenaTransportLink_Close(athenaTransportLink);
                    }
                    if (pollfdReceiveList[index].revents & POLLIN) {
                        athenaTransportLink_SetEvent(athenaTransportLink, AthenaTransportLinkEvent_Receive);
                    } else {
                        athenaTransportLink_ClearEvent(athenaTransportLink, AthenaTransportLinkEvent_Receive);
                    }
                }
            }
        }
        events += result;
    }

    result = poll(pollfdSendList, pollfdListSize, 0);
    if (result < 0) {
        parcLog_Error(athenaTransportLinkAdapter_GetLogger(athenaTransportLinkAdapter),
                      "Send list poll error: (%d) %s", errno, strerror(errno));
    } else {
        for (int index = 0; index < pollfdListSize; index++) {
            if (pollfdSendList[index].revents) {
                AthenaTransportLink *athenaTransportLink = athenaTransportLinkAdapter->pollfdTransportLink[index];
                if (athenaTransportLink) {
                    if (pollfdSendList[index].revents & (POLLNVAL | POLLHUP | POLLERR)) {
                        continue;
                    }
                    if (pollfdSendList[index].revents & (POLLERR | POLLHUP)) {
                        athenaTransportLink_SetEvent(athenaTransportLink, AthenaTransportLinkEvent_Error);
                        athenaTransportLink_Close(athenaTransportLink);
                    }
                    if (pollfdSendList[index].revents & POLLOUT) {
                        athenaTransportLink_SetEvent(athenaTransportLink, AthenaTransportLinkEvent_Send);
                    } else {
                        athenaTransportLink_ClearEvent(athenaTransportLink, AthenaTransportLinkEvent_Send);
                    }
                }
            }
        }
        //events += result; // don't register send events
    }
    return events;
}

int
athenaTransportLinkAdapter_CloseByName(AthenaTransportLinkAdapter *athenaTransportLinkAdapter, const char *linkName)
{
    if (athenaTransportLinkAdapter->listenerList) {
        for (int index = 0; index < parcArrayList_Size(athenaTransportLinkAdapter->listenerList); index++) {
            AthenaTransportLink *athenaTransportLink = parcArrayList_Get(athenaTransportLinkAdapter->listenerList, index);
            if (strcmp(athenaTransportLink_GetName(athenaTransportLink), linkName) == 0) {
                athenaTransportLink_Close(athenaTransportLink);
                return 0;
            }
        }
    }
    if (athenaTransportLinkAdapter->instanceList) {
        for (int index = 0; index < parcArrayList_Size(athenaTransportLinkAdapter->instanceList); index++) {
            AthenaTransportLink *athenaTransportLink = parcArrayList_Get(athenaTransportLinkAdapter->instanceList, index);
            if (athenaTransportLink) {
                if (strcmp(athenaTransportLink_GetName(athenaTransportLink), linkName) == 0) {
                    athenaTransportLink_Close(athenaTransportLink);
                    return 0;
                }
            }
        }
    }
    errno = ENOENT;
    return -1;
}

PARCBitVector *
athenaTransportLinkAdapter_Close(AthenaTransportLinkAdapter *athenaTransportLinkAdapter, PARCBitVector *linkVector)
{
    PARCBitVector *resultVector = parcBitVector_Create();
    int nextLinkToClose = 0;
    while ((nextLinkToClose = parcBitVector_NextBitSet(linkVector, nextLinkToClose)) >= 0) {
        AthenaTransportLink *athenaTransportLink = parcArrayList_Get(athenaTransportLinkAdapter->instanceList, nextLinkToClose);
        if (athenaTransportLink) {
            athenaTransportLink_Close(athenaTransportLink);
            parcBitVector_Set(resultVector, nextLinkToClose);
            nextLinkToClose++;
        }
    }
    return resultVector;
}

static CCNxMetaMessage *
_retrieve_next_message(AthenaTransportLinkAdapter *athenaTransportLinkAdapter, PARCArrayList *list, int *linkId)
{
    CCNxMetaMessage *ccnxMetaMessage;
    int index = 0;

    if (linkId) {
        index = *linkId;
    }

    // The passed in linkId is where we start off from in the list traversal.  It's either
    // set and returned as the linkId of the message received, or set to zero and returned
    // if there is no message.
    for (; index < parcArrayList_Size(list); index++) {
        athenaTransportLinkAdapter->stats.messageReceive_Attempted++;
        AthenaTransportLink *athenaTransportLink = parcArrayList_Get(list, index);
        if (athenaTransportLink == NULL) {
            athenaTransportLinkAdapter->stats.messageReceive_LinkDoesNotExist++;
            continue;
        }
        if (athenaTransportLink_GetEvent(athenaTransportLink) & AthenaTransportLinkEvent_Receive) {
            ccnxMetaMessage = athenaTransportLink_Receive(athenaTransportLink);
            if (ccnxMetaMessage) {
                athenaTransportLinkAdapter->stats.messageReceived++;
                if (linkId) {
                    *linkId = index;
                }
                return ccnxMetaMessage;
            } else {
                athenaTransportLinkAdapter->stats.messageReceive_NoMessage++;
            }
        }
    }

    if (linkId) {
        *linkId = 0;
    }
    return NULL;
}

CCNxMetaMessage *
athenaTransportLinkAdapter_Receive(AthenaTransportLinkAdapter *athenaTransportLinkAdapter,
                                   PARCBitVector **resultVector, int timeout)
{
    int linkId;
    *resultVector = parcBitVector_Create();

    // Traverse instance list starting from where we last left off.
    linkId = athenaTransportLinkAdapter->nextLinkToRead;
    CCNxMetaMessage *ccnxMetaMessage = _retrieve_next_message(athenaTransportLinkAdapter, athenaTransportLinkAdapter->instanceList, &linkId);
    if (ccnxMetaMessage) {
        parcBitVector_Set(*resultVector, linkId);
        athenaTransportLinkAdapter->nextLinkToRead = linkId + 1;
        athenaTransportLinkAdapter->stats.messageReceived++;
        return ccnxMetaMessage;
    }

    // Once the instance list is clear, traverse the entire listener list
    ccnxMetaMessage = _retrieve_next_message(athenaTransportLinkAdapter, athenaTransportLinkAdapter->listenerList, NULL);
    assertNull(ccnxMetaMessage, "listener returned an unexpected message");

    // Last instance event has been serviced, poll all modules.
    athenaTransportLinkAdapter_Poll(athenaTransportLinkAdapter, timeout);

    // Traverse the entire listener list (could potentially populate other instances)
    ccnxMetaMessage = _retrieve_next_message(athenaTransportLinkAdapter, athenaTransportLinkAdapter->listenerList, NULL);
    assertNull(ccnxMetaMessage, "listener returned an unexpected message");

    // Search instance list from the beginning.
    athenaTransportLinkAdapter->nextLinkToRead = 0;
    linkId = athenaTransportLinkAdapter->nextLinkToRead;
    ccnxMetaMessage = _retrieve_next_message(athenaTransportLinkAdapter, athenaTransportLinkAdapter->instanceList, &linkId);
    if (ccnxMetaMessage) {
        parcBitVector_Set(*resultVector, linkId);
        athenaTransportLinkAdapter->nextLinkToRead = linkId + 1;
        athenaTransportLinkAdapter->stats.messageReceived++;
        return ccnxMetaMessage;
    }

    parcBitVector_Release(resultVector);
    errno = EAGAIN;
    return NULL;
}

PARCBitVector *
athenaTransportLinkAdapter_Send(AthenaTransportLinkAdapter *athenaTransportLinkAdapter,
                                CCNxMetaMessage *ccnxMetaMessage,
                                PARCBitVector *linkOutputVector)
{
    PARCBitVector *resultVector = parcBitVector_Create();
    int nextLinkToWrite = 0;

    if (athenaTransportLinkAdapter->instanceList == NULL) {
        return resultVector;
    }

    while ((nextLinkToWrite = parcBitVector_NextBitSet(linkOutputVector, nextLinkToWrite)) >= 0) {
        athenaTransportLinkAdapter->stats.messageSend_Attempted++;
        if (nextLinkToWrite >= parcArrayList_Size(athenaTransportLinkAdapter->instanceList)) {
            athenaTransportLinkAdapter->stats.messageSend_LinkDoesNotExist++;
            nextLinkToWrite++;
            continue;
        }
        AthenaTransportLink *athenaTransportLink = parcArrayList_Get(athenaTransportLinkAdapter->instanceList, nextLinkToWrite);
        if (athenaTransportLink == NULL) {
            athenaTransportLinkAdapter->stats.messageSend_LinkDoesNotExist++;
            nextLinkToWrite++;
            continue;
        }
        if (!(athenaTransportLink_GetEvent(athenaTransportLink) & AthenaTransportLinkEvent_Send)) {
            athenaTransportLinkAdapter->stats.messageSend_LinkNotAcceptingSendRequests++;
            nextLinkToWrite++;
            continue;
        }
        // If we're sending an interest to a non-local link,
        // check that it has a sufficient hoplimit.
        if (ccnxMetaMessage_IsInterest(ccnxMetaMessage)) {
            if (athenaTransportLink_IsNotLocal(athenaTransportLink)) {
                if (ccnxInterest_GetHopLimit(ccnxMetaMessage) == 0) {
                    athenaTransportLinkAdapter->stats.messageSend_HopLimitExceeded++;
                    nextLinkToWrite++;
                    continue;
                }
            }
        }
        int result = athenaTransportLink_Send(athenaTransportLink, ccnxMetaMessage);
        if (result == 0) {
            athenaTransportLinkAdapter->stats.messageSent++;
            parcBitVector_Set(resultVector, nextLinkToWrite);
        } else {
            athenaTransportLinkAdapter->stats.messageSend_LinkSendFailed++;
        }
        nextLinkToWrite++;
    }

    return resultVector;
}

const char *
athenaTransportLinkAdapter_LinkIdToName(AthenaTransportLinkAdapter *athenaTransportLinkAdapter, int linkId)
{
    if (athenaTransportLinkAdapter->instanceList == NULL) {
        return NULL;
    }
    if (linkId < parcArrayList_Size(athenaTransportLinkAdapter->instanceList)) {
        AthenaTransportLink *athenaTransportLink = parcArrayList_Get(athenaTransportLinkAdapter->instanceList, linkId);
        if (athenaTransportLink) {
            return athenaTransportLink_GetName(athenaTransportLink);
        }
    }
    return NULL;
}

int
athenaTransportLinkAdapter_LinkNameToId(AthenaTransportLinkAdapter *athenaTransportLinkAdapter, const char *linkName)
{
    if (athenaTransportLinkAdapter->instanceList == NULL) {
        return -1;
    }
    for (int index = 0; index < parcArrayList_Size(athenaTransportLinkAdapter->instanceList); index++) {
        AthenaTransportLink *athenaTransportLink = parcArrayList_Get(athenaTransportLinkAdapter->instanceList, index);
        if (athenaTransportLink) {
            if (strcmp(athenaTransportLink_GetName(athenaTransportLink), linkName) == 0) {
                return index;
            }
        }
    }
    return -1;
}

bool
athenaTransportLinkAdapter_IsNotLocal(AthenaTransportLinkAdapter *athenaTransportLinkAdapter, int linkId)
{
    if (athenaTransportLinkAdapter->instanceList == NULL) {
        return false;
    }
    if ((linkId >= 0) && (linkId < parcArrayList_Size(athenaTransportLinkAdapter->instanceList))) {
        AthenaTransportLink *athenaTransportLink = parcArrayList_Get(athenaTransportLinkAdapter->instanceList, linkId);
        if (athenaTransportLink) {
            return athenaTransportLink_IsNotLocal(athenaTransportLink);
        }
    }
    return false;
}

static CCNxMetaMessage *
_create_linkList_response(AthenaTransportLinkAdapter *athenaTransportLinkAdapter, CCNxName *ccnxName)
{
    PARCJSONArray *jsonLinkList = parcJSONArray_Create();

    for (int index = 0; index < parcArrayList_Size(athenaTransportLinkAdapter->listenerList); index++) {
        AthenaTransportLink *athenaTransportLink = parcArrayList_Get(athenaTransportLinkAdapter->listenerList, index);
        const char *linkName = athenaTransportLink_GetName(athenaTransportLink);
        bool notLocal = athenaTransportLink_IsNotLocal(athenaTransportLink);
        bool localForced = athenaTransportLink_IsForceLocal(athenaTransportLink);
        PARCJSON *jsonItem = parcJSON_Create();
        parcJSON_AddString(jsonItem, "linkName", linkName);
        parcJSON_AddInteger(jsonItem, "index", -1);
        parcJSON_AddBoolean(jsonItem, "notLocal", notLocal);
        parcJSON_AddBoolean(jsonItem, "localForced", localForced);

        PARCJSONValue *jsonItemValue = parcJSONValue_CreateFromJSON(jsonItem);
        parcJSON_Release(&jsonItem);

        parcJSONArray_AddValue(jsonLinkList, jsonItemValue);
        parcJSONValue_Release(&jsonItemValue);

        if (notLocal) {
            parcLog_Debug(athenaTransportLinkAdapter->log, "\n    Link listener%s: %s", localForced ? " (forced remote)" : "", linkName);
        } else {
            parcLog_Debug(athenaTransportLinkAdapter->log, "\n    Link listener%s: %s", localForced ? " (forced local)" : "", linkName);
        }
    }
    for (int index = 0; index < parcArrayList_Size(athenaTransportLinkAdapter->instanceList); index++) {
        AthenaTransportLink *athenaTransportLink = parcArrayList_Get(athenaTransportLinkAdapter->instanceList, index);
        if (athenaTransportLink) {
            const char *linkName = athenaTransportLink_GetName(athenaTransportLink);
            bool notLocal = athenaTransportLink_IsNotLocal(athenaTransportLink);
            bool localForced = athenaTransportLink_IsForceLocal(athenaTransportLink);
            PARCJSON *jsonItem = parcJSON_Create();
            parcJSON_AddString(jsonItem, "linkName", linkName);
            parcJSON_AddInteger(jsonItem, "index", index);
            parcJSON_AddBoolean(jsonItem, "notLocal", notLocal);
            parcJSON_AddBoolean(jsonItem, "localForced", localForced);

            PARCJSONValue *jsonItemValue = parcJSONValue_CreateFromJSON(jsonItem);
            parcJSON_Release(&jsonItem);

            parcJSONArray_AddValue(jsonLinkList, jsonItemValue);
            parcJSONValue_Release(&jsonItemValue);

            if (notLocal) {
                parcLog_Debug(athenaTransportLinkAdapter->log, "\n    Link instance [%d] %s: %s", index, localForced ? "(forced remote)" : "(remote)", linkName);
            } else {
                parcLog_Debug(athenaTransportLinkAdapter->log, "\n    Link instance [%d] %s: %s", index, localForced ? "(forced local)" : "(local)", linkName);
            }
        }
    }

    char *jsonString = parcJSONArray_ToString(jsonLinkList);

    parcJSONArray_Release(&jsonLinkList);

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

CCNxContentObject *
athenaTransportLinkAdapter_ProcessMessage(AthenaTransportLinkAdapter *athenaTransportLinkAdapter, const CCNxInterest *interest)
{
    CCNxMetaMessage *responseMessage = NULL;

    CCNxName *ccnxName = ccnxInterest_GetName(interest);
    if (ccnxName_GetSegmentCount(ccnxName) > AthenaCommandSegment) {
        CCNxNameSegment *nameSegment = ccnxName_GetSegment(ccnxName, AthenaCommandSegment);
        char *command = ccnxNameSegment_ToString(nameSegment);

        if (strcmp(command, AthenaCommand_List) == 0) {
            responseMessage = _create_linkList_response(athenaTransportLinkAdapter, ccnxName);
        }
        parcMemory_Deallocate(&command);
    }
    return responseMessage;
}

void
athenaTransportLinkAdapter_SetLogLevel(AthenaTransportLinkAdapter *athenaTransportLinkAdapter, const PARCLogLevel level)
{
    // set log level on main athena module
    parcLog_SetLevel(athenaTransportLinkAdapter->log, level);

    // set log level on all modules
    if (athenaTransportLinkAdapter->moduleList) {
        for (int index = 0; index < parcArrayList_Size(athenaTransportLinkAdapter->moduleList); index++) {
            AthenaTransportLinkModule *athenaTransportLinkModule = parcArrayList_Get(athenaTransportLinkAdapter->moduleList, index);
            athenaTransportLinkModule_SetLogLevel(athenaTransportLinkModule, level);
        }
    }
    // set log level on all instances
    if (athenaTransportLinkAdapter->listenerList) {
        for (int index = 0; index < parcArrayList_Size(athenaTransportLinkAdapter->listenerList); index++) {
            AthenaTransportLink *athenaTransportLink = parcArrayList_Get(athenaTransportLinkAdapter->listenerList, index);
            athenaTransportLink_SetLogLevel(athenaTransportLink, level);
        }
    }
    // set log level on all listener instances
    if (athenaTransportLinkAdapter->instanceList) {
        for (int index = 0; index < parcArrayList_Size(athenaTransportLinkAdapter->instanceList); index++) {
            AthenaTransportLink *athenaTransportLink = parcArrayList_Get(athenaTransportLinkAdapter->instanceList, index);
            if (athenaTransportLink) {
                athenaTransportLink_SetLogLevel(athenaTransportLink, level);
            }
        }
    }
}
