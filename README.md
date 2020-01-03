# Console interface to NETCONF servers

This program provides an interactive console for working with YANG data.
It can connect to NETCONF servers, and also talk to [sysrepo](http://www.sysrepo.org/) locally.

## Installation

For building, one needs:

* A C++17 compiler
* [Boost](https://www.boost.org/) version 1.69
* [cmake](https://cmake.org/download/) for managing the build
* [libyang](https://github.com/CESNET/libyang) for working with YANG models
* [libnetconf2](https://github.com/CESNET/libnetconf2) for connecting to NETCONF servers
* [sysrepo](https://github.com/sysrepo/sysrepo/) **version 0.7.x** for the local sysrepo backend
* [replxx](https://github.com/AmokHuginnsson/replxx) which provides interactive line prompts
* [docopt](https://github.com/docopt/docopt.cpp) for CLI option parsing
* [pkg-config](https://www.freedesktop.org/wiki/Software/pkg-config/) for building
* [Doctest](https://github.com/onqtam/doctest/) as a C++ unit test framework
* [trompeloeil](https://github.com/rollbear/trompeloeil) for mock objects in C++

Use an **exact commit** of any dependencies as specified in `submodules/dependencies/*`.

The build process uses [CMake](https://cmake.org/runningcmake/).
A quick-and-dirty build with no fancy options can be as simple as `mkdir build && cd build && cmake .. && make && make install`.

## Bug Reporting

Issue reporting and feature requests are welcome [via Taiga.io](https://tree.taiga.io/project/jktjkt-netconf-cli/issues?status=1900205,1900206,1900207).

## Development

We are using [Gerrit](https://gerrit.cesnet.cz/q/project:CzechLight%252Fnetconf-cli+status:open) for patch submission, code review and Continuous Integration (CI).
Development roadmap and planning happens [over Taiga.io](https://tree.taiga.io/project/jktjkt-netconf-cli/kanban).

## Credits

Copyright © CESNET, https://www.cesnet.cz/ .
Portions copyright © Faculty of Information Technology, Czech Technical University in Prague, https://fit.cvut.cz/ .
Most of the code was written by Václav Kubernát (CESNET, formerly FIT ČVUT) and Jan Kundrát (CESNET).
The project is distributed under the terms of the Apache 2.0 license.
