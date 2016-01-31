#! /usr/bin/python
#
# Copyright (c) 2016, Xerox Corporation (Xerox)and Palo Alto Research Center (PARC)
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
# @author Priti Goel, Palo Alto Research Center (PARC)
# @copyright 2016, Xerox Corporation (Xerox)and Palo Alto Research Center (PARC).  All rights reserved.
#
# This script reads the dot file and builds athena connections based on specified topology

import argparse
import subprocess
from pydotplus.graphviz import graph_from_dot_file

# Build argument parser (and help)
argparser = argparse.ArgumentParser(description='Build links between nodes according to a dotfile graph description.')
argparser.add_argument('dotfile', metavar='DOTFILE', help='The DOT file containing the graph/network description')

args = argparser.parse_args()

print('Parsing dot file (' + args.dotfile + ')...')
graph = graph_from_dot_file(args.dotfile)

# Setup dircetory variables
keyfileName = 'keyfile'
password = 'foo'
build_dir = '../../../../../../../usr'
athena = build_dir + '/bin/athena'
athena_ctl = build_dir + '/bin/athenactl -f ' + keyfileName + ' -p ' + password

# Initialize a list of nodes running forwarder
running_fwder_nodes= []

# Dictionary of UDP port number to be used for next link on athena instance
next_udp_port = {}

# kill athena if running already
cmd_output = subprocess.Popen('pkill athena', stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=True)

# Create the keystore
create_key_cmd = [build_dir + '/bin/parc_publickey -c' + ' ' + keyfileName + ' ' + password + ' ' + 'athena 1024 365']
print "Processing command %s." % create_key_cmd
cmd_output = subprocess.Popen(create_key_cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=True)
print cmd_output.stdout.readlines()

# Find all edges of the graph
edges = graph.get_edge_list()

# Function to extract fwder control port number from name
def find_fwder_port(name):
    return name[name.index("_")+1:]

# Initialize udp port number for each name
def get_next_udp_port(name):
    return (50 + int(name[1:name.index("_")])) * 100

# Build first_forwarder command
def build_first_fwder_cmd(name, first_fwder_port):
    first_fwder_cmd = [athena + ' -c tcp://localhost:' + str(first_fwder_port) + '/listener &']
    print "Processing on %s command %s." % (name, first_fwder_cmd)
    return first_fwder_cmd

# Build new forwarder instance command
def build_fwder_cmd(name, first_fwder_port, fwder_port):
    fwder_cmd = [athena_ctl + ' -a tcp://localhost:' + str(first_fwder_port) + ' spawn ' + str(fwder_port)]
    print "Processing on %s command %s." % (name, fwder_cmd)
    return fwder_cmd

# Build link name from node names
def build_link_name_from_nodes(node1, node2):
    return str(node1[:node1.index("_")]+node2[:node2.index("_")])

# Build link command from src to dest
def build_link_cmd(src, dest, link_name):
    link_cmd = [athena_ctl + ' -a tcp://localhost:' + str(src[src.index("_")+1:]) + ' add link udp://localhost:' + str(next_udp_port[dest]) +  '/local=false/name=' + link_name + '/src=localhost:' + str(next_udp_port[src])]
    print "Processing command %s." % (link_cmd)
    return link_cmd

# Run forwarder command
def run_fwder_cmd(cmd):
    return subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=True)

# For all nodes connecting edges, start athena if not done already and create links
for x in edges:
    src = x.get_source()
    dest = x.get_destination()
    # If athena not running already, start it, else use athena_ctl to spawn a new instance
    if src not in running_fwder_nodes:
        if not running_fwder_nodes:
            first_fwder_port = find_fwder_port(src)
            # Construct first forwarder command and run it
            first_fwder_cmd = build_first_fwder_cmd(src, first_fwder_port)
            cmd_output = run_fwder_cmd(first_fwder_cmd)
            running_fwder_nodes += [src]
            next_udp_port[src] = get_next_udp_port(src)
        else:
            fwder_port = find_fwder_port(src)
            # Construct forwarder command and run it
            fwder_cmd = build_fwder_cmd(src, first_fwder_port, fwder_port)
            cmd_output = run_fwder_cmd(fwder_cmd)
            print cmd_output.stdout.readlines()
            running_fwder_nodes += [src]
            next_udp_port[src] = get_next_udp_port(src)
    if dest not in running_fwder_nodes:
        fwder_port = find_fwder_port(dest)
        # Construct forwarder command and run it
        fwder_cmd = build_fwder_cmd(dest, first_fwder_port, fwder_port)
        cmd_output = run_fwder_cmd(fwder_cmd)
        print cmd_output.stdout.readlines()
        running_fwder_nodes += [dest]
        next_udp_port[dest] = get_next_udp_port(dest)

    # Add links on both end points of the edge
    link_name = build_link_name_from_nodes(src,dest)

    link_cmd = build_link_cmd(src,dest, link_name)
    cmd_output = run_fwder_cmd(link_cmd)

    link_cmd = build_link_cmd(dest, src, link_name)
    cmd_output = run_fwder_cmd(link_cmd)

    # Increment the next udp port number to be used for each instance
    next_udp_port[src] += 1
    next_udp_port[dest] += 1

print "Nodes on which forwarders are running "
print running_fwder_nodes
