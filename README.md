Athena
=======
A CCNx forwarder

## Quick Start ##
```
$ git clone git@github.com:PARC/Athena.git Athena
$ mkdir Athena.build
$ cd Athena.build
$ cmake -DCMAKE_INSTALL_PREFIX=${INSTALL_DIR} ../Athena
$ make
$ make test
$ make install
```

## Introduction ##

Athena is a CCNx Forwarder.

## Using Athena ##

### Distillery ###

Athena is part of [CCNx Distillery](https://github.com/PARC/CCNx_Distillery). You may want to get the software via that distribution if you want to work on CCNx.

### Platforms ###

Athena has been tested in:

- Ubuntu 14.04 (x86_64)
- MacOSX 10.10
- MacOSX 10.11

Other platforms and architectures may work.

### Dependencies ###

Build dependencies:

- c99 ( clang / gcc )
- CMake 3.4

Basic dependencies:

- OpenSSL
- pthreads
- Libevent
- [LongBow](https://github.com/PARC/LongBow)
- [Libparc](https://github.com/PARC/Libparc)
- [Libccnx-common](https://github.com/PARC/Libccnx-common)
- [Libccnx-transport-rta](https://github.com/PARC/Libccnx-transport-rta)


Documentation dependencies:

- Doxygen


### Getting Started ###

Athena is built using cmake. You will need to have CMake 3.4 installed in order to build it.

```
Download Athena
$ git clone git@github.com:PARC/Athena.git Athena

Create build directory
$ mkdir Athena.build
$ cd Athena.build

Prepare the build, give an install directory
$ cmake -DCMAKE_INSTALL_PREFIX=${INSTALL_DIR} ../Athena

Compile the software
$ make

Run unit tests
$ make test

Install the software
$ make install
```

This will place the Athena binaries in the `bin` directory of `${INSTALL_DIR}`.



### Using Athena ###

Athena is a set of binary executables that are used to run a CCNx forwarder instance. Please refer to the Athena documentation for detailed information.  You can also try the -h flags of the executables

- `athena -h`
- `athenactl -h`

### Contact ###

- [Athena GitHub](https://github.com/PARC/Athena)
- [CCNx Website](http://www.ccnx.org/)
- [CCNx Mailing List](https://www.ccnx.org/mailman/listinfo/ccnx/)


### License ###

This software is distributed under the following license:

```
Copyright (c) 2013,2014,2015,2016, Xerox Corporation (Xerox) and Palo Alto Research Center, Inc (PARC)
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright
  notice, this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright
  notice, this list of conditions and the following disclaimer in the
  documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL XEROX OR PARC BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

################################################################################
#
# PATENT NOTICE
#
# This software is distributed under the BSD 2-clause License (see LICENSE
# file).  This BSD License does not make any patent claims and as such, does
# not act as a patent grant.  The purpose of this section is for each contributor
# to define their intentions with respect to intellectual property.
#
# Each contributor to this source code is encouraged to state their patent
# claims and licensing mechanisms for any contributions made. At the end of
# this section contributors may each make their own statements.  Contributor's
# claims and grants only apply to the pieces (source code, programs, text,
# media, etc) that they have contributed directly to this software.
#
# There is no guarantee that this section is complete, up to date or accurate. It
# is up to the contributors to maintain their portion of this section and up to
# the user of the software to verify any claims herein.
#
# Do not remove this header notification.  The contents of this section must be
# present in all distributions of the software.  You may only modify your own
# intellectual property statements.  Please provide contact information.

- Palo Alto Research Center, Inc
This software distribution does not grant any rights to patents owned by Palo
Alto Research Center, Inc (PARC). Rights to these patents are available via
various mechanisms. As of January 2016 PARC has committed to FRAND licensing any
intellectual property used by its contributions to this software. You may
contact PARC at cipo@parc.com for more information or visit http://www.ccnx.org
```
