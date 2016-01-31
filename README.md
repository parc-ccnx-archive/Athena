Athena
=================

A simple forwarder application for CCNx.

[CCNx.org](https://www.ccnx.org/)

This is ...

The point of ...

After building, Athana demo consists of 2 programs:

* `athena`: The Athena forwarder
* `athenaControl`: Application to send control messages to Athena

REQUIREMENTS
------------

Athena needs the Distillery CCNx distribution installed on the
system. Please download and install Distillery. [https://www.ccnx.org/download/] (https://www.ccnx.org/download/)

Obtaining Athena
-----------------------

You can obtain Athena code by downloading it from [github] (https://github.com/PARC/Athena).


Building and Running
--------------------

Assuming you've unpacked the Distillery tarball into the default location
(`/usr/local/ccnx/`), `/usr/local/`, Athena is installed with the rest of the CCNx
software in `/usr/local/ccnx/bin`.

Compiling the tutorial:

1. Go into the Athena directory created when you cloned or unpacked the
   package:
   `$ cd Athena`

2. Configure the tutorial program:
`$ ./configure --prefix=$HOME/ccnx`.
The `--prefix=` argument specifies the destination directory if you run
 `make install`

3. Compile the tutorial, setting the `LD_RUN_PATH` for the compiled executables:
`$ make`

4. At this point, the compiled binaries for `athena` and
`athenaControl` can be found in the `Athena/src` directory.

5. Install the Athena binaries to the specified prefix in the
configure step (eg `$HOME/ccn`). You will then be able to find the binaries in
the bin directory (eg `$HOME/ccn/bin`)
`make install`

6. Start the CCNx forwarder, `athena`:
`$ /usr/local/ccnx/bin/athena &`

7. ... Running the tutorial_Server and tutorial_Client to test the forwarder ...

## Notes: ##

If you have any problems with the system, please discuss them on the developer
mailing list:  `ccnx@ccnx.org`.  


CONTACT
-------

For any questions please use the CCNx mailing list.  ccnx@ccnx.org


LICENSE
-------

Copyright (c) 2015, Xerox Corporation (Xerox)and Palo Alto Research Center (PARC)
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Patent rights are not granted under this agreement. Patent rights are
      available under FRAND terms.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL XEROX or PARC BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=
