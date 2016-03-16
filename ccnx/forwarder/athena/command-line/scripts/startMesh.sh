#!/bin/bash
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
# Start first forwarder on 9695 then create $INSTANCES additional instances in a fully
# connected mesh.
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
# "local=true" may cause non-terminating forwarding loops if a service doesn't answer
#
NOTLOCAL="local=false"

#####
#####

#
# Default locations for applications that we depend upon
#
ATHENA=../../../../../../../usr/bin/athena
ATHENACTL=../../../../../../../usr/bin/athenactl
PARC_PUBLICKEY=../../../../../../../usr/bin/parc-publickey
TUTORIAL_SERVER=../../../../../../../usr/bin/ccnxSimpleFileTransfer_Server
TUTORIAL_CLIENT=../../../../../../../usr/bin/ccnxSimpleFileTransfer_Client

DEPENDENCIES="ATHENA ATHENACTL PARC_PUBLICKEY TUTORIAL_SERVER TUTORIAL_CLIENT"

#
# Check any set CCNX_HOME/PARC_HOME environment settings, then the default path, then our build directories.
#
for program in ${DEPENDENCIES}
do
    eval defaultPath=\${${program}}               # <NAME>=<DEFAULT_PATH>
    appName=`expr ${defaultPath} : '.*/\(.*\)'`
    if [ -x ${CCNX_HOME}/bin/${appName} ]; then   # check CCNX_HOME
        eval ${program}=${CCNX_HOME}/bin/${appName}
    elif [ -x ${PARC_HOME}/bin/${appName} ]; then # check PARC_HOME
        eval ${program}=${PARC_HOME}/bin/${appName}
    else                                          # check PATH
        eval ${program}=""
        localPathLookup=`which ${appName}`
        if [ $? -eq 0 ]; then
            eval ${program}=${localPathLookup}
        else                                      # use default build directory location
            [ -f ${defaultPath} ] && eval ${program}=${defaultPath}
        fi
    fi
    eval using=\${${program}}
    if [ "${using}" = "" ]; then
        echo Couldn\'t locate ${appName}, set CCNX_HOME or PARC_HOME to its location.
        exit 1
    fi
    echo Using ${program}=${using}
done

#
# Add key and password parameters to athenactl arguments
#
ATHENACTL="${ATHENACTL} -p ${PASSWORD} -f ${KEYFILE}"

#
# Setup the key for athenactl
#
${PARC_PUBLICKEY} -c ${KEYFILE} ${PASSWORD} athena 1024 365

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
