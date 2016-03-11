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

#
# Start the Athena tutorial Server
#
# Configuration topology is a series of forwarders in a ring.
#
# FIB entries are created around the entire ring, so if the server isn't started interest messages
# will continue being forwarded until their hoplimit is exceeded (255)
#

#
# URI to service, routes are set on each forwarder instance to forward this URI to its neighbor
#
URI=lci:/ccnx/tutorial

#
# Start first forwarder on 9695 then create 20 forwarder instances in a ring
#
FIRSTPORT=9695
INSTANCES=20

#
# Logging and credential information
#
LOGLEVEL=debug
ATHENA_LOGFILE=athena.out
KEYFILE=keyfile
PASSWORD=foo

#
# Set NOTLOCAL to "local=true" to turn off hoplimit counting
# "local=true" will cause non-terminating forwarding loops in the ring if a service isn't answering
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

ATHENACTL="${ATHENACTL} -p ${PASSWORD} -f ${KEYFILE}"

#
# Setup a key for athenactl
#
${PARC_PUBLICKEY} -c ${KEYFILE} ${PASSWORD} athena 1024 365

#
# Start the default Athena forwarder instance listening on localhost
# and add a non-local link for the local nodename
#
${ATHENA} -c tcp://localhost:${FIRSTPORT}/listener 2>&1 > ${ATHENA_LOGFILE} &
trap "kill -HUP $!" INT

${ATHENACTL} -a tcp://localhost:${FIRSTPORT} add link tcp://`uname -n`:${FIRSTPORT}/listener/${NOTLOCAL}/name=athena${FIRSTPORT}
${ATHENACTL} -a tcp://localhost:${FIRSTPORT} set level ${LOGLEVEL}

#
# Spawn ${INSTANCES} of forwarders, each has a link from its predecesor and its predecesor has a FIB entry to it
#
# eg:
#   9695->9696
#   9696->9697
#   ...
#
instance=1
while [ $instance -le ${INSTANCES} ];
do
    echo

    newport=`expr ${FIRSTPORT} + ${instance}`
    connect_to=`expr ${newport} - 1`

    echo Starting athena${newport}
    ${ATHENACTL} -a tcp://localhost:${FIRSTPORT} spawn ${newport}
    ${ATHENACTL} -a tcp://localhost:${newport} set level ${LOGLEVEL}

    # add our listener
    ${ATHENACTL} -a tcp://localhost:${newport} add link tcp://`uname -n`:${newport}/listener/${NOTLOCAL}/name=athena${newport}Listener

    # have our predecesor connect to us
    echo Adding link from athena${connect_to} -\> athena${newport}
    ${ATHENACTL} -a tcp://localhost:${connect_to} add link tcp://`uname -n`:${newport}/${NOTLOCAL}/name=athena${newport}

    # create a route on our predecesor to us
    echo Establishing FIB for ${URI} athena${connect_to} -\> athena${newport}
    ${ATHENACTL} -a tcp://localhost:${connect_to} add route athena${newport} ${URI}

    instance=`expr $instance + 1`
done
echo

#
# Attach the last instance to the first, creating a ring
#
echo Attaching last forwarder to first
LASTPORT=`expr ${FIRSTPORT} + ${INSTANCES}`
echo Adding link from athena${LASTPORT} -\> athena${FIRSTPORT}
${ATHENACTL} -a tcp://localhost:${LASTPORT} add link tcp://`uname -n`:${FIRSTPORT}/${NOTLOCAL}/name=athena${FIRSTPORT}
${ATHENACTL} -a tcp://localhost:${LASTPORT} set level ${LOGLEVEL}

#
# complete the routing loop
#
echo Establishing FIB for ${URI} athena${LASTPORT} -\> athena${FIRSTPORT}
${ATHENACTL} -a tcp://localhost:${LASTPORT} add route athena${FIRSTPORT} ${URI}

#
# Start a tutorial server on the last instance
#
echo starting tutorial_Server on port ${LASTPORT}
export METIS_PORT=${LASTPORT}
${TUTORIAL_SERVER} /tmp
wait
