/*
 * Copyright (c) 2016, Xerox Corporation (Xerox) and Palo Alto Research Center, Inc (PARC)
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
 * @copyright (c) 2016, Xerox Corporation (Xerox) and Palo Alto Research Center, Inc (PARC).  All rights reserved.
 */

/*
 * Provide support for loadable fragmentation modules.  The fragmenter library must be named
 * libathena_Fragmenter_<name>, and must contain an initialization routine that is named
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
    parcLog_Debug(athenaTransportLink_GetLogger((*athenaFragmenter)->athenaTransportLink), "Detached %s", (*athenaFragmenter)->moduleName);
    if ((*athenaFragmenter)->fini) {
        (*athenaFragmenter)->fini(*athenaFragmenter);
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
    const char *initEntry = _nameToInitMethod(fragmenterName);

    // Check to see if the module is already linked in.
    void *linkModule = RTLD_DEFAULT;
    AthenaFragmenter_Init _init = dlsym(linkModule, initEntry);

    if (_init == NULL) {
        const char *moduleLibrary = _nameToLibrary(fragmenterName);
        linkModule = dlopen(moduleLibrary, RTLD_NOW | RTLD_GLOBAL);
        parcMemory_Deallocate(&moduleLibrary);

        if (linkModule == NULL) {
            athenaFragmenter_Release(&athenaFragmenter);
            parcMemory_Deallocate(&initEntry);
            errno = ENOENT;
            return NULL;
        }
       _init = dlsym(linkModule, initEntry);
    }
    parcMemory_Deallocate(&initEntry);

    if (_init == NULL) {
        athenaFragmenter_Release(&athenaFragmenter);
        if (linkModule != RTLD_DEFAULT) {
            dlclose(linkModule);
        }
        errno = EFAULT;
        return NULL;
    }

    if (_init(athenaFragmenter) == NULL) {
        athenaFragmenter_Release(&athenaFragmenter);
        errno = ENODEV;
        return NULL;
    }
    parcLog_Debug(athenaTransportLink_GetLogger(athenaFragmenter->athenaTransportLink), "Attached %s", athenaFragmenter->moduleName);

    return athenaFragmenter;
}

parcObject_ImplementAcquire(athenaFragmenter, AthenaFragmenter);

parcObject_ImplementRelease(athenaFragmenter, AthenaFragmenter);

PARCBuffer *
athenaFragmenter_ReceiveFragment(AthenaFragmenter *athenaFragmenter, PARCBuffer *wireFormatBuffer)
{
    if (athenaFragmenter && athenaFragmenter->receiveFragment) {
        parcLog_Debug(athenaTransportLink_GetLogger(athenaFragmenter->athenaTransportLink),
                      "%s received fragment (%zu)", athenaFragmenter->moduleName, parcBuffer_Remaining(wireFormatBuffer));
        return athenaFragmenter->receiveFragment(athenaFragmenter, wireFormatBuffer);
    }
    return wireFormatBuffer;
}

CCNxCodecEncodingBufferIOVec *
athenaFragmenter_CreateFragment(AthenaFragmenter *athenaFragmenter, PARCBuffer *message, size_t mtu, int fragmentNumber)
{
    if (athenaFragmenter && athenaFragmenter->createFragment) {
        parcLog_Debug(athenaTransportLink_GetLogger(athenaFragmenter->athenaTransportLink),
                      "%s created fragment (%zu)", athenaFragmenter->moduleName, mtu);
        return athenaFragmenter->createFragment(athenaFragmenter, message, mtu, fragmentNumber);
    } else {
        errno = ENOENT;
        return NULL;
    }
    return 0;
}
