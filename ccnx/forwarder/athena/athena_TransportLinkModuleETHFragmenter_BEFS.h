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
#ifndef libathena_EthernetFragmenter_BEFS_h
#define libathena_EthernetFragmenter_BEFS_h

/*
 * Fragmenter module supporting point to point fragmentation that is documented in:
 *
 *    ICN "Begin-End" Hop by Hop Fragmentation (draft-mosko-icnrg-beginendfragment-00)
 */

// Legacy names from original Metis implementation
#define METIS_PACKET_TYPE_HOPFRAG 4
#define T_HOPFRAG_PAYLOAD  0x0005

typedef struct hopbyhop_header {
    uint8_t version;
    uint8_t packetType;
    uint16_t packetLength;
    uint8_t blob[3];
    uint8_t headerLength;
    uint16_t tlvType;
    uint16_t tlvLength;
} __attribute__((packed)) _HopByHopHeader;

/*
 * The B bit value in the top byte of the header
 */
#define BMASK 0x40

/*
 * The E bit value in the top byte of the header
 */
#define EMASK 0x20

/*
 * The I bit value in the top byte of the header
 */
#define IMASK 0x10

/*
 * Sets the B flag in the header
 */
#define _hopByHopHeader_SetBFlag(header) ((header)->blob[0] |= BMASK)

/*
 * Sets the E flag in the header
 */
#define _hopByHopHeader_SetEFlag(header) ((header)->blob[0] |= EMASK)

/*
 * non-zero if the B flag is set
 */
#define _hopByHopHeader_GetBFlag(header) ((header)->blob[0] & BMASK)

/*
 * non-zero if the E flag is set
 */
#define _hopByHopHeader_GetEFlag(header) ((header)->blob[0] & EMASK)

/*
 * non-zero if the I flag is set
 */
#define _hopByHopHeader_GetIFlag(header) ((header)->blob[0] & IMASK)

AthenaEthernetFragmenter *athenaEthernetFragmenter_BEFS_Init(AthenaEthernetFragmenter *athenaEthernetFragmenter);
#endif // libathena_EthernetFragmenter_BEFS_h
