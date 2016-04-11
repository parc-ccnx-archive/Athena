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

/**
 * @typedef AthenaEthernetFragmenter
 * @brief Ethernet fragmenter instance private data
 */
typedef struct AthenaEthernetFragmenter AthenaEthernetFragmenter;

/**
 * @typedef AthenaEthernetFragmenter_Send
 * @brief Ethernet fragmenter send method
 */
typedef int (AthenaEthernetFragmenter_Send)(AthenaEthernetFragmenter *athenaEthernetFragmenter,
                                            AthenaEthernet *athenaEthernet,
                                            size_t mtu, struct ether_header *etherHeader,
                                            CCNxMetaMessage *ccnxMetaMessage);
/**
 * @typedef AthenaEthernetFragmenter_Receive
 * @brief Ethernet fragmenter receive method
 */
typedef PARCBuffer *(AthenaEthernetFragmenter_Receive)(AthenaEthernetFragmenter *athenaEthernetFragmenter,
                                                       PARCBuffer *wireFormatBuffer);

/**
 * @typedef AthenaEthernetFragmenter_Init
 * @brief Ethernet fragmenter initialization method
 */
typedef AthenaEthernetFragmenter *(AthenaEthernetFragmenter_Init)(AthenaEthernetFragmenter *athenaEthernetFragmenter);

/**
 * @typedef AthenaEthernetFragmenter_Init
 * @brief Ethernet fragmenter initialization method
 */
typedef void (AthenaEthernetFragmenter_Fini)(AthenaEthernetFragmenter *athenaEthernetFragmenter);

//
// Private data for each fragmented connection
//
struct AthenaEthernetFragmenter {
    AthenaTransportLink *athenaTransportLink; // link associated with fragmenter
    void *module; // so library can be unloaded
    AthenaEthernetFragmenter_Send *send;
    AthenaEthernetFragmenter_Receive *receive;
    AthenaEthernetFragmenter_Fini *fini;
    void *fragmenterData;
};

/**
 * @abstract create a new fragmenter instance
 * @discussion
 *
 * @param [in] athenaTransportLink associated with the fragmenter
 * @param [in] fragmenterName of new fragmenter
 * @return pointer to new instance
 *
 * Example:
 * @code
 * void
 * {
 *     AthenaEthernetFragmenter *athenaEthernetFragmenter = athenaEthernetFragmenter_Create(athenaTransportLink, "BEFS");
 * }
 * @endcode
 */
AthenaEthernetFragmenter *athenaEthernetFragmenter_Create(AthenaTransportLink *athenaTransportLink, const char *fragmenterName);

/**
 * @abstract obtain a new reference to a fragmenter instance
 * @discussion
 *
 * @param [in] athenaEthernetFragmenter instance to acquire a reference to
 * @return pointer to new reference
 *
 * Example:
 * @code
 * void
 * {
 *     AthenaEthernetFragmenter *newReference = athenaEthernetFragmenter_Acquire(athenaEthernetFragmenter);
 * }
 * @endcode
 */
AthenaEthernetFragmenter *athenaEthernetFragmenter_Acquire(const AthenaEthernetFragmenter *athenaEthernetFragmenter);

/**
 * @abstract release a fragmenter reference
 * @discussion
 *
 * @param [in] athenaEthernetFragmenter instance to release
 *
 * Example:
 * @code
 * void
 * {
 *     athenaEthernetFragmenter_Release(&athenaEthernetFragmenter);
 * }
 * @endcode
 */
void athenaEthernetFragmenter_Release(AthenaEthernetFragmenter **);

/**
 * @abstract send a message fragmenting it by the provided mtu size
 * @discussion
 *
 * @param [in] athenaEthernetFragmenter
 * @param [in] athenaEthernet
 * @param [in] mtu
 * @param [in] header
 * @param [in] ccnxMetaMessage
 * @return 0 on success, -1 on failure with errno set to indicate failure
 *
 * Example:
 * @code
 * void
 * {
 *     int result = athenaEthernetFragmenter_Send(athenaEthernetFragmenter, athenaEthernet, mtu, header, ccnxMetaMessage);
 * }
 * @endcode
 */
int athenaEthernetFragmenter_Send(AthenaEthernetFragmenter *athenaEthernetFragmenter,
                                  AthenaEthernet *athenaEthernet, size_t mtu,
                                  struct ether_header *header, CCNxMetaMessage *ccnxMetaMessage);

/**
 * @abstract construct a message from received fragments
 * @discussion
 *
 * @param [in] athenaEthernetFragmenter
 * @param [in] wireFormatBuffer
 * @return pointer to reassembled message,
 *         NULL if waiting for more fragments,
 *         or original inputWireFormatBuffer if it was not recognized as a fragment.
 *
 * Example:
 * @code
 * void
 * {
 *     PARCBuffer *wireFormatBuffer = athenaEthernetFragmenter_Receive(athenaEthernetFragmenter, inputWireFormatBuffer);
 * }
 * @endcode
 */
PARCBuffer *athenaEthernetFragmenter_Receive(AthenaEthernetFragmenter *athenaEthernetFragmenter,
                                             PARCBuffer *wireFormatBuffer);

#endif // libathena_EthernetFragmenter
