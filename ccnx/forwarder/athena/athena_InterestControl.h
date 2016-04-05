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
#ifndef libathena_InterestControl_h
#define libathena_InterestControl_h

#include "athena.h"
#include <ccnx/common/ccnx_Interest.h>
#include <parc/algol/parc_BitVector.h>

#define JSON_KEY_RESULT "result"
#define JSON_KEY_NAME "name"
#define JSON_KEY_LINK "link"

/**
 * @abstract process a CCNx interest control message
 * @discussion
 *
 * @param [in] athena forwarder context
 * @param [in] control pointer to interest control message to process
 * @param [in] ingressVector link message was received from
 * @return 0 on success
 *
 * Example:
 * @code
 * {
 *     Athena *athena = athena_Create();
 *     PARCBitVector *ingressVector;
 *     CCNxMetaMessage *ccnxMessage = athenaTransportLinkAdapter_Receive(athena->athenaTransportLinkAdapter, &ingressVector, -1)
 *
 *     athenaInterestControl(athena, ccnxMessage, ingressVector);
 *
 *     ccnxMetaMessage_Release(&ccnxMessage);
 *     parcBitVector_Release(&ingressVector);
 *     athena_Release(&athena);
 * }
 * @endcode
 */
int athenaInterestControl(Athena *athena, CCNxInterest *interest, PARCBitVector *ingressVector);

/**
 * @abstract log a change in the forwarder configuration
 * @discussion
 *
 * @param [in] athena forwarder context
 * @param [in] ccnx interest control name
 * @param [in] control message payload to log
 *
 * Example:
 * @code
 * {
 *     Athena *athena = athena_Create();
 *     CCNxName *addLink = ccnxName_CreateFromCString(CCNxNameAthenaCommand_LinkConnect);
 *     const char *connectionSpecification = "tcp://localhost:9695/name=myLoopback";
 *     athenaInterestControl_LogConfigurationChange(athena, addLink, "%s", connectionSpecification);
 *     ccnxName_Release(&addLink);
 *     athena_Release(&athena);
 * }
 * @endcode
 */
void athenaInterestControl_LogConfigurationChange(Athena *athena, CCNxName *ccnxName, const char *format, ...);
#endif // libathena_InterestControl_h
