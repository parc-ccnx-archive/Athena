/*
 * Copyright (c) 2016, Xerox Corporation (Xerox)and Palo Alto Research Center (PARC)
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
 * @copyright 2016, Xerox Corporation (Xerox)and Palo Alto Research Center (PARC).  All rights reserved.
 */

/*
 * Provide support for loadable fragmentation modules.  The fragmenter library must be named
 * libathena_ETHFragmenter_<name>, and must contain an initialization routine that is named
 * athenaFragmenter_<name>_Init.  The init routine is provided an AthenaFragmenter
 * object instance that is used to maintain private instance state for the fragmentation module.
 */
#include <config.h>

#include <LongBow/runtime.h>

#include <dlfcn.h>
#include <errno.h>

#include <parc/algol/parc_Object.h>
#include <ccnx/forwarder/athena/athena_TransportLinkModule.h>
#include <ccnx/forwarder/athena/athena_Fragmenter.h>

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

#define LIBRARY_MODULE_PREFIX "libathena_Fragmenter_"
#ifdef __linux__
#define LIBRARY_MODULE_SUFFIX ".so"
#else // MacOS
#define LIBRARY_MODULE_SUFFIX ".dylib"
#endif

#define METHOD_PREFIX "athenaFragmenter_"
#define INIT_METHOD_SUFFIX "_Init"

static const char *
_nameToInitMethod(const char *name)
{
    char *result = NULL;

    assertNotNull(name, "name must not be null");
    const char *module = _strtoupper(name);
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

static const char *
_nameToLibrary(const char *name)
{
    char *result = NULL;

    assertNotNull(name, "module name must not be null");
    const char *libraryName = _strtoupper(name);
    PARCBufferComposer *composer = parcBufferComposer_Create();
    if (composer != NULL) {
        parcBufferComposer_Format(composer, "%s%s%s",
                                  LIBRARY_MODULE_PREFIX, libraryName, LIBRARY_MODULE_SUFFIX);
        PARCBuffer *tempBuffer = parcBufferComposer_ProduceBuffer(composer);
        parcBufferComposer_Release(&composer);

        result = parcBuffer_ToString(tempBuffer);
        parcBuffer_Release(&tempBuffer);
    }

    parcMemory_Deallocate(&libraryName);
    return result;
}

static void
_destroy(AthenaFragmenter **athenaFragmenter)
{
    if ((*athenaFragmenter)->fini) {
        (*athenaFragmenter)->fini(*athenaFragmenter);
    }
    if ((*athenaFragmenter)->module) {
        dlclose((*athenaFragmenter)->module);
    }
    parcMemory_Deallocate(&((*athenaFragmenter)->moduleName));
    athenaTransportLink_Release(&((*athenaFragmenter)->athenaTransportLink));
}

parcObject_ExtendPARCObject(AthenaFragmenter, _destroy, NULL, NULL, NULL, NULL, NULL, NULL);

AthenaFragmenter *
athenaFragmenter_Create(AthenaTransportLink *athenaTransportLink, const char *fragmenterName)
{
    AthenaFragmenter *athenaFragmenter = parcObject_CreateAndClearInstance(AthenaFragmenter);
    assertNotNull(athenaFragmenter, "Could not create a new fragmenter instance.");
    athenaFragmenter->moduleName = parcMemory_StringDuplicate(fragmenterName, strlen(fragmenterName));

    athenaFragmenter->athenaTransportLink = athenaTransportLink_Acquire(athenaTransportLink);
    const char *moduleLibrary = _nameToLibrary(fragmenterName);
    athenaFragmenter->module = dlopen(moduleLibrary, RTLD_NOW | RTLD_GLOBAL);
    parcMemory_Deallocate(&moduleLibrary);

    if (athenaFragmenter->module == NULL) {
        athenaFragmenter_Release(&athenaFragmenter);
        errno = ENOENT;
        return NULL;
    }

    const char *initEntry = _nameToInitMethod(fragmenterName);
    AthenaFragmenter_Init *_init = dlsym(athenaFragmenter->module, initEntry);
    parcMemory_Deallocate(&initEntry);

    if (_init == NULL) {
        athenaFragmenter_Release(&athenaFragmenter);
        errno = EFAULT;
        return NULL;
    }

    if (_init(athenaFragmenter) == NULL) {
        athenaFragmenter_Release(&athenaFragmenter);
        errno = ENODEV;
        return NULL;
    }

    return athenaFragmenter;
}

parcObject_ImplementAcquire(athenaFragmenter, AthenaFragmenter);

parcObject_ImplementRelease(athenaFragmenter, AthenaFragmenter);

PARCBuffer *
athenaFragmenter_ReceiveFragment(AthenaFragmenter *athenaFragmenter, PARCBuffer *wireFormatBuffer)
{
    if (athenaFragmenter && athenaFragmenter->receiveFragment) {
        return athenaFragmenter->receiveFragment(athenaFragmenter, wireFormatBuffer);
    }
    return wireFormatBuffer;
}

CCNxCodecEncodingBufferIOVec *
athenaFragmenter_CreateFragment(AthenaFragmenter *athenaFragmenter, PARCBuffer *message, size_t mtu, int fragmentNumber)
{
    if (athenaFragmenter && athenaFragmenter->createFragment) {
        return athenaFragmenter->createFragment(athenaFragmenter, message, mtu, fragmentNumber);
    } else {
        errno = ENOENT;
        return NULL;
    }
    return 0;
}
