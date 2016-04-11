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
#ifndef libathena_EthernetFragmenter
#define libathena_EthernetFragmenter

#include <parc/algol/parc_Deque.h>
#include <ccnx/forwarder/athena/athena_Ethernet.h>

typedef struct AthenaEthernetFragmenter AthenaEthernetFragmenter;

typedef int (AthenaEthernetFragmenter_Send)(AthenaEthernetFragmenter *athenaEthernetFragmenter,
                                            AthenaEthernet *athenaEthernet,
                                            size_t mtu, struct ether_header *etherHeader,
                                            CCNxMetaMessage *ccnxMetaMessage);
typedef PARCBuffer *(AthenaEthernetFragmenter_Receive)(AthenaEthernetFragmenter *athenaEthernetFragmenter,
                                                       PARCBuffer *wireFormatBuffer);

typedef void (AthenaEthernetFragmenter_Fini)(AthenaEthernetFragmenter *athenaEthernetFragmenter);

//
// Private data for each fragmented connection
//
struct AthenaEthernetFragmenter {
    void *module;
    uint32_t sendSequenceNumber;
    uint32_t receiveSequenceNumber;
    PARCDeque *fragments;
    AthenaEthernetFragmenter_Send *send;
    AthenaEthernetFragmenter_Receive *receive;
    AthenaEthernetFragmenter_Fini *fini;
};

typedef AthenaEthernetFragmenter *(AthenaEthernetFragmenter_Init)();

AthenaEthernetFragmenter *athenaEthernetFragmenter_Create(const char *fragmenterName);
AthenaEthernetFragmenter *athenaEthernetFragmenter_Acquire(const AthenaEthernetFragmenter *);
void athenaEthernetFragmenter_Release(AthenaEthernetFragmenter **);
int athenaEthernetFragmenter_Send(AthenaEthernetFragmenter *athenaEthernetFragmenter,
                                  AthenaEthernet *athenaEthernet, size_t mtu,
                                  struct ether_header *header, CCNxMetaMessage *ccnxMetaMessage);
PARCBuffer *athenaEthernetFragmenter_Receive(AthenaEthernetFragmenter *athenaEthernetFragmenter,
                                             PARCBuffer *wireFormatBuffer);

#endif // libathena_EthernetFragmenter
