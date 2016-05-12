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
#ifndef libathena_Control_h
#define libathena_Control_h

#include <ccnx/forwarder/athena/athena.h>
#include <ccnx/common/ccnx_Interest.h>
#include <parc/algol/parc_BitVector.h>

/**
 * @abstract process a CCNx control message
 * @discussion
 *
 * @param [in] athena forwarder context
 * @param [in] control pointer to control message to process
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
 *     athenaControl(athena, ccnxMessage, ingressVector);
 *
 *     ccnxMetaMessage_Release(&ccnxMessage);
 *     parcBitVector_Release(&ingressVector);
 *     athena_Release(&athena);
 * }
 * @endcode
 */
int athenaControl(Athena *athena, CCNxControl *control, PARCBitVector *ingressVector);

/**
 * Process a message (e.g. an Interest) addressed to this module. For example, it might be a
 * message asking for a particular statistic or a control message. The response can be NULL,
 * or a response. A response to a query message might be a ContentObject with the requested data.
 * The function should return NULL if the interest request is unknown and the caller should handle the response.
 *
 * @param athena instance
 * @param message the message addressed to this instance of the store
 * @return NULL if message request is unknown.
 * @return a `CCNxMetaMessage` instance containing a response.
 */
CCNxMetaMessage *athenaControl_ProcessMessage(Athena *athena, const CCNxMetaMessage *message);
#endif // libathena_Control_h
