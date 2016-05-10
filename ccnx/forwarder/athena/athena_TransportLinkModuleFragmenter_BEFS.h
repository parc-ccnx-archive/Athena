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
#ifndef libathena_Fragmenter_BEFS_h
#define libathena_Fragmenter_BEFS_h

/**
 * @abstract Create and initialize a new BEFS fragmenter instance
 * @discussion
 *
 * This method is called indirectly by athenaFragmenter_Create when its associated shared library is loaded.
 * It is the Init methods responsibility to populate the data structure with its
 * private data, along with methods to call to create and receive fragments and a
 * finalization method for cleaning up, and closing the fragmentation instance.
 *
 * @param [in] athenaFragmenter instance to initialize
 * @return pointer to initialized instance
 *
 * Example:
 * @code
 * void
 * {
 *     AthenaFragmenter *athenaFragmenter = athenaFragmenter_Create(athenaTransportLink, "BEFS");
 * }
 * @endcode
 */
AthenaFragmenter *athenaFragmenter_BEFS_Init(AthenaFragmenter *athenaFragmenter);
#endif // libathena_Fragmenter_BEFS_h
