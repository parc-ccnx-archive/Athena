#!/bin/sh
#
# Copyright (c) 2015, Xerox Corporation (Xerox)and Palo Alto Research Center (PARC)
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
#     * Redistributions of source code must retain the above copyright
#       notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above copyright
#       notice, this list of conditions and the following disclaimer in the
#       documentation and/or other materials provided with the distribution.
#     * Patent rights are not granted under this agreement. Patent rights are
#       available under FRAND terms.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL XEROX or PARC BE LIABLE FOR ANY
# DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
# ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
# @author Kevin Fox, Palo Alto Research Center (PARC)
# @copyright 2015, Xerox Corporation (Xerox)and Palo Alto Research Center (PARC).  All rights reserved.
#
# Start the Athena tutorial Server
#
# Configuration topology is a series of forwarders in a fully connected star/mesh.
#
#   A       B
#    \-----/
#    | \ / |
#    | / \ |
#    /-----\
#   C       D
#

#
# Set ATHENADIR to your CCNx build
#
ATHENADIR=${CCNX_HOME:-../../../../../../../usr}

#
# Start first forwarder on 9695 then create $INSTANCES additional instances in a fully
# connected mesh (star).
#
FIRSTPORT=9695
INSTANCES=4 # Mac OS X may run out of file descriptors above 33

#
# Logging and credential information
#
LOGLEVEL=debug
ATHENA_LOGFILE=athena.out
KEYFILE=keyfile
PASSWORD=foo

#
# Set NOTLOCAL to "local=true" to turn off hoplimit counting
# "local=true" will cause non-terminating forwarding loops if a service isn't answering
#
NOTLOCAL="local=false"

#####
#####

ATHENA="${ATHENADIR}/bin/athena"
ATHENACTL="${ATHENADIR}/bin/athenactl -p ${PASSWORD} -f ${KEYFILE}"
TUTORIAL_SERVER=${ATHENADIR}/bin/tutorial_Server
TUTORIAL_CLIENT=${ATHENADIR}/bin/tutorial_Client

#
# Setup a key for athenactl
#
${ATHENADIR}/bin/parc_publickey -c ${KEYFILE} ${PASSWORD} athena 1024 365

${ATHENA} -c tcp://localhost:${FIRSTPORT}/listener &
trap "kill -HUP $!" INT

${ATHENACTL} -a tcp://localhost:${FIRSTPORT} add link udp://localhost:${FIRSTPORT}/listener/name=A_Listener/${NOTLOCAL}

instance=1
while [ $instance -lt ${INSTANCES} ];
do
    NEXTPORT=`expr ${FIRSTPORT} + ${instance}`
    name=`echo ${instance} | tr 0-9 A-J`
    # echo Starting ${name}_Listener
    ${ATHENACTL} -a tcp://localhost:${FIRSTPORT} spawn ${NEXTPORT}
    ${ATHENACTL} -a tcp://localhost:${NEXTPORT} add link udp://localhost:${NEXTPORT}/listener/name=${name}_Listener/${NOTLOCAL}
    instance=`expr $instance + 1`
done

instance=0
while [ $instance -lt ${INSTANCES} ];
do
    neighbor=0
    while [ $neighbor -lt ${INSTANCES} ];
    do
        if [ $neighbor -eq $instance ]; then
            neighbor=`expr $neighbor + 1`
            continue
        fi
        neighborName=`echo ${neighbor} | tr 0-9 A-J`
        neighborPort=`expr ${FIRSTPORT} + ${neighbor}`
        myName=`echo ${instance} | tr 0-9 A-J`
        myPort=`expr ${FIRSTPORT} + ${instance}`

        # echo Adding link $myName -\> $neighborName
        ${ATHENACTL} -a tcp://localhost:${myPort} add link udp://localhost:${neighborPort}/name=${neighborName}-${neighborPort}/${NOTLOCAL}

        neighbor=`expr $neighbor + 1`
    done
    instance=`expr $instance + 1`
done

wait
