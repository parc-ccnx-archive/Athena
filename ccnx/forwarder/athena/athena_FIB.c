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
#include <config.h>

#include <stdio.h>

#include <ccnx/forwarder/athena/athena.h>
#include <parc/algol/parc_BitVector.h>
#include <parc/algol/parc_HashMap.h>
#include <parc/algol/parc_TreeRedBlack.h>

#include <ccnx/forwarder/athena/athena_FIB.h>

/**
 * @typedef AthenaFIB
 * @brief FIB tables, tableByName (KEY == CCNxName, VALUE == PARCBitVector
 *                    listOfLinks (List ( index = linkId ) of lists (CCNxNames))
 */
struct athena_FIB {
    PARCHashMap *tableByName;
    PARCList *listOfLinks;
    PARCBitVector *defaultRoute;
};

/**
 * @typedef AthenaFIBListEntry
 * @brief Element for FIB table entry list
 */
struct athena_FIB_list_entry {
    CCNxName *name;
    int linkId;
};

CCNxName *
athenaFIBListEntry_GetName(AthenaFIBListEntry *entry)
{
    return entry->name;
}

int
athenaFIBListEntry_GetLinkId(AthenaFIBListEntry *entry)
{
    return entry->linkId;
}


static void
_athenaFIB_Destroy(AthenaFIB **fib)
{
    AthenaFIB *pFib = *fib;
    parcHashMap_Release(&pFib->tableByName);
    parcList_Release(&pFib->listOfLinks);
    if (pFib->defaultRoute != NULL) {
        parcBitVector_Release(&pFib->defaultRoute);
    }
}

parcObject_ExtendPARCObject(AthenaFIB, _athenaFIB_Destroy, NULL, NULL, NULL, NULL, NULL, NULL);

parcObject_ImplementAcquire(athenaFIB, AthenaFIB);

parcObject_ImplementRelease(athenaFIB, AthenaFIB);

AthenaFIB *
athenaFIB_Create()
{
    AthenaFIB *newFIB = parcObject_CreateInstance(AthenaFIB);
    if (newFIB != NULL) {
        newFIB->listOfLinks = parcList(parcArrayList_Create((void (*)(void**))parcList_Release), PARCArrayListAsPARCList);
        newFIB->tableByName = parcHashMap_Create();
        newFIB->defaultRoute = NULL;
    }

    return newFIB;
}

PARCBitVector *
athenaFIB_Lookup(AthenaFIB *athenaFIB, const CCNxName *ccnxName, PARCBitVector *ingressVector)
{
    CCNxName *name = ccnxName_Copy(ccnxName);
    PARCBitVector *result = NULL;

    // Return the longest prefix match which contains at least one link other than the ingress.
    // If the result happens to contain the ingress link, make a copy and remove it before returning.
    while ((ccnxName_GetSegmentCount(name) > 0) && (result == NULL)) {
        result = (PARCBitVector *) parcHashMap_Get(athenaFIB->tableByName, (PARCObject *) name);
        if (result) {
            // If there's an ingressVector provided, return a copy of the result the ingress link cleared.
            // If that would result in an empty vector, continue looking for a substring match.
            if (ingressVector != NULL) {
                assertTrue(parcBitVector_NumberOfBitsSet(ingressVector) <= 1, "Ingress vector with more than one link set");
                if (parcBitVector_Contains(result, ingressVector)) {
                    if (parcBitVector_NumberOfBitsSet(result) > 1) {
                        result = parcBitVector_Copy(result);
                        parcBitVector_ClearVector(result, ingressVector);
                    } else { // ingress was only link, keep looking
                        result = NULL;
                        name = ccnxName_Trim(name, 1);
                    }
                    continue;
                }
            }
            result = parcBitVector_Acquire(result);
        }
        name = ccnxName_Trim(name, 1);
    }
    ccnxName_Release(&name);

    // The default route is outside of the Lookup table, so we need to check it independently
    // and remove the ingress link if it's in the entry.
    if ((result == NULL) && (athenaFIB->defaultRoute != NULL)) {
        if (ingressVector != NULL) {
            if (parcBitVector_Contains(athenaFIB->defaultRoute, ingressVector)) {
                // The ingress link is in the link vector list
                // either make a copy and remove it, or return an empty egress
                if (parcBitVector_NumberOfBitsSet(athenaFIB->defaultRoute) > 1) {
                    result = parcBitVector_Copy(athenaFIB->defaultRoute);
                    parcBitVector_ClearVector(result, ingressVector);
                } else { // ingress was the only link
                    result = NULL;
                }
            } else { // ingress was not in the default route
                result = parcBitVector_Acquire(athenaFIB->defaultRoute);
            }
        } else { // no ingress vector was provided
            result = parcBitVector_Acquire(athenaFIB->defaultRoute);
        }
    }

    return result;
}

bool
athenaFIB_AddRoute(AthenaFIB *athenaFIB, const CCNxName *ccnxName, const PARCBitVector *ccnxLinkVector)
{
    PARCBitVector *linkV = NULL;

    // Check if this is a mapping for the default route
    if (ccnxName_GetSegmentCount(ccnxName) == 1) {
        CCNxNameSegment *segment = ccnxName_GetSegment(ccnxName, 0);
        if ((ccnxNameSegment_GetType(segment) == CCNxNameLabelType_NAME) &&
            (ccnxNameSegment_Length(segment) == 0)) {
            if (athenaFIB->defaultRoute == NULL) {
                athenaFIB->defaultRoute = parcBitVector_Create();
            }
            linkV = athenaFIB->defaultRoute;
        }
    }

    if (linkV == NULL) { // It's not the default link
        // for each bit in the link vector, add an entry for the name in the list of links for future
        // cleanup
        for (int i = 0, bit = 0; i < parcBitVector_NumberOfBitsSet(ccnxLinkVector); ++i, ++bit) {
            bit = parcBitVector_NextBitSet(ccnxLinkVector, bit);
            if (bit >= parcList_Size(athenaFIB->listOfLinks)) {
                //Expand the list if needed
                for (size_t j = parcList_Size(athenaFIB->listOfLinks); j <= bit; ++j) {
                    parcList_Add(athenaFIB->listOfLinks, NULL);
                }
            }
            PARCList *nameList = parcList_GetAtIndex((PARCList *) athenaFIB->listOfLinks, bit);
            if (nameList == NULL) {
                nameList = parcList(parcArrayList_Create((void (*)(void **))ccnxName_Release), PARCArrayListAsPARCList);
                parcList_SetAtIndex(athenaFIB->listOfLinks, bit, (PARCObject *) nameList);
                parcList_Add(nameList, (PARCObject *) ccnxName_Acquire(ccnxName));
            } else {
                // Make sure the entry is only added once to the list
                bool found = false;
                for (int j = 0; (j < parcList_Size(nameList)) && !found; ++j) {
                    CCNxName *key = (CCNxName *) parcList_GetAtIndex(nameList, j);
                    if (ccnxName_Equals(ccnxName, key)) {
                        found = true;
                    }
                }
                if (!found) {
                    parcList_Add(nameList, (PARCObject *) ccnxName_Acquire(ccnxName));
                }
            }
        }

        // Now add the actual fib mapping
        linkV = (PARCBitVector *) parcHashMap_Get(athenaFIB->tableByName, (PARCObject *) ccnxName);
        if (linkV == NULL) {
            PARCBitVector *newLinkV = parcBitVector_Create();
            linkV = newLinkV;
            parcHashMap_Put(athenaFIB->tableByName, (PARCObject *) ccnxName, (PARCObject *) newLinkV);
            parcBitVector_Release(&newLinkV);
        }
    }

    parcBitVector_SetVector(linkV, ccnxLinkVector);

    return true;
}

bool
athenaFIB_DeleteRoute(AthenaFIB *athenaFIB, const CCNxName *ccnxName, const PARCBitVector *ccnxLinkVector)
{
    bool result = false;

    // Check if this is a mapping for the default route
    if (ccnxName_GetSegmentCount(ccnxName) == 1) {
        CCNxNameSegment *segment = ccnxName_GetSegment(ccnxName, 0);
        if ((ccnxNameSegment_GetType(segment) == CCNxNameLabelType_NAME) &&
            (ccnxNameSegment_Length(segment) == 0)) {
            if (athenaFIB->defaultRoute != NULL) {
                PARCBitVector *linkSet = parcBitVector_And(athenaFIB->defaultRoute, ccnxLinkVector);
                if (parcBitVector_NumberOfBitsSet(linkSet) > 0) {
                    parcBitVector_ClearVector(athenaFIB->defaultRoute, ccnxLinkVector);
                    result = true;
                }
                parcBitVector_Release(&linkSet);
                if (parcBitVector_NumberOfBitsSet(athenaFIB->defaultRoute) == 0) {
                    parcBitVector_Release(&(athenaFIB->defaultRoute));
                    athenaFIB->defaultRoute = NULL;
                }
            }
            return result;
        }
    }

    PARCBitVector *linkV = athenaFIB_Lookup(athenaFIB, ccnxName, NULL);
    if (linkV != NULL) {
        // Only clear bits if the link sets intersect
        PARCBitVector *linkSet = parcBitVector_And(linkV, ccnxLinkVector);
        if (parcBitVector_NumberOfBitsSet(linkSet) > 0) {
            parcBitVector_ClearVector(linkV, ccnxLinkVector);
            if (parcBitVector_NumberOfBitsSet(linkV) == 0) {
                parcHashMap_Remove(athenaFIB->tableByName, (PARCObject *) ccnxName);
            }
            //
            // Traverse each referenced interface list and remove the route if a reference is found there.
            //
            for (int bit = 0; (bit = parcBitVector_NextBitSet(ccnxLinkVector, bit)) >= 0; bit++) {
                PARCList *nameList = parcList_GetAtIndex((PARCList *) athenaFIB->listOfLinks, bit);
                if (nameList) {
                    for (int j = 0; j < parcList_Size(nameList); ++j) {
                        CCNxName *key = (CCNxName *) parcList_GetAtIndex(nameList, j);
                        if (ccnxName_Equals(ccnxName, key)) {
                            parcList_RemoveAtIndex(nameList, j);
                            ccnxName_Release(&key);
                            break;
                        }
                    }
                }
            }
            result = true;
        }
        parcBitVector_Release(&linkSet);
        parcBitVector_Release(&linkV);
    }

    return result;
}

bool
athenaFIB_RemoveLink(AthenaFIB *athenaFIB, const PARCBitVector *ccnxLinkVector)
{
    bool result = true;

    for (int i = 0, bit = 0; i < parcBitVector_NumberOfBitsSet(ccnxLinkVector); ++i) {
        bit = parcBitVector_NextBitSet(ccnxLinkVector, bit);
        if (parcList_Size(athenaFIB->listOfLinks) <= bit) {
            break;
        }
        PARCList *nameList = parcList_GetAtIndex(athenaFIB->listOfLinks, bit);
        if (nameList != NULL) {
            while (parcList_Size(nameList)) {
                CCNxName *key = (CCNxName *) parcList_GetAtIndex(nameList, 0);
                athenaFIB_DeleteRoute(athenaFIB, key, ccnxLinkVector);
            }
            parcList_Clear(nameList);
            result = true;
        }
    }
    if (athenaFIB->defaultRoute) {
        parcBitVector_ClearVector(athenaFIB->defaultRoute, ccnxLinkVector);
        if (parcBitVector_NumberOfBitsSet(athenaFIB->defaultRoute) == 0) {
            parcBitVector_Release(&(athenaFIB->defaultRoute));
            athenaFIB->defaultRoute = NULL;
        }
    }

    return result;
}

static void
_athenaFIBListEntry_Destroy(AthenaFIBListEntry **entryHandle)
{
    AthenaFIBListEntry *entry = *entryHandle;
    if (entry->name != NULL) {
        ccnxName_Release(&entry->name);
    }
}

parcObject_ExtendPARCObject(AthenaFIBListEntry, _athenaFIBListEntry_Destroy, NULL, NULL, NULL, NULL, NULL, NULL);

static
parcObject_ImplementRelease(_athenaFIBListEntry, AthenaFIBListEntry);

static AthenaFIBListEntry *
_athenaFIBListEntry_Create(const CCNxName *name, int linkId)
{
    AthenaFIBListEntry *entry = parcObject_CreateInstance(AthenaFIBListEntry);

    if (entry != NULL) {
        entry->name = ccnxName_Acquire(name);
        entry->linkId = linkId;
    }

    return entry;
}


PARCList *
athenaFIB_CreateEntryList(AthenaFIB *athenaFIB)
{
    PARCList *result =
        parcList(parcArrayList_Create((void (*)(void **))_athenaFIBListEntry_Release), PARCArrayListAsPARCList);

    if (athenaFIB->defaultRoute != NULL) {
        CCNxName *defaultPrefix = ccnxName_CreateFromCString("ccnx:/");
        for (int bit = 0; (bit = parcBitVector_NextBitSet(athenaFIB->defaultRoute, bit)) >= 0; bit++) {
            AthenaFIBListEntry *entry = _athenaFIBListEntry_Create(defaultPrefix, bit);
            parcList_Add(result, entry);
        }
        ccnxName_Release(&defaultPrefix);
    }
    for (size_t i = 0; i < parcList_Size(athenaFIB->listOfLinks); ++i) {
        PARCList *linksForId = parcList_GetAtIndex(athenaFIB->listOfLinks, i);
        if (linksForId != NULL) {
            for (size_t j = 0; j < parcList_Size(linksForId); ++j) {
                CCNxName *name = parcList_GetAtIndex(linksForId, j);
                AthenaFIBListEntry *entry = _athenaFIBListEntry_Create(name, (int) i);
                parcList_Add(result, entry);
            }
        }
    }
    return result;
}

CCNxMetaMessage *
athenaFIB_ProcessMessage(AthenaFIB *athenaFIB, const CCNxMetaMessage *message)
{
    CCNxMetaMessage *responseMessage = NULL;
    return responseMessage;
}
