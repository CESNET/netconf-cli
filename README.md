# Console interface to NETCONF servers

![License](https://img.shields.io/github/license/CESNET/netconf-cli)
[![Gerrit](https://img.shields.io/badge/patches-via%20Gerrit-blue)](https://gerrit.cesnet.cz/q/project:CzechLight/netconf-cli)
[![Zuul CI](https://img.shields.io/badge/zuul-checked-green)](https://zuul.gerrit.cesnet.cz/t/public/buildsets?project=CzechLight/netconf-cli)
[![taiga.io](https://img.shields.io/badge/bugs-via%20taiga.io-blue)](https://tree.taiga.io/project/jktjkt-netconf-cli)

This program provides an interactive console for working with YANG data.
It can connect to NETCONF servers, work as a standalone YANG data editor, or talk to [sysrepo](http://www.sysrepo.org/) locally.

## Installation

For building, one needs:

* A C++20 compiler
* [Boost](https://www.boost.org/) (we're testing with `1.78`)
* [cmake](https://cmake.org/download/) for managing the build
* [libyang](https://github.com/CESNET/libyang) plus the [C++ bindings](https://github.com/CESNET/libyang-cpp)
* [libnetconf2](https://github.com/CESNET/libnetconf2) plus the [C++ bindings](https://github.com/CESNET/libnetconf2-cpp) for connecting to NETCONF servers
* [replxx](https://github.com/AmokHuginnsson/replxx) which provides interactive line prompts
* [docopt](https://github.com/docopt/docopt.cpp) for CLI option parsing
* [pkg-config](https://www.freedesktop.org/wiki/Software/pkg-config/) for building
* optionally, [Doctest](https://github.com/onqtam/doctest/) as a C++ unit test framework
* optionally, [trompeloeil](https://github.com/rollbear/trompeloeil) for mock objects in C++
* optionally, [sysrepo](https://github.com/sysrepo/sysrepo/) plus the [C++ bindings](https://github.com/sysrepo/sysrepo-cpp) for the local sysrepo backend, and for the comprehensive test suite
* optionally, [netopeer2](https://github.com/CESNET/netopeer2) for a test suite

At times the `devel` branch of `libyang`, `sysrepo`, `libnetconf2` and `Netopeer2` might be required due to fresh bugfixes and behavior changes.

The build process uses [CMake](https://cmake.org/runningcmake/).
A quick-and-dirty build with no fancy options can be as simple as `mkdir build && cd build && cmake .. && make && make install`.

## Bug Reporting

Issue reporting and feature requests are welcome over GitHub.

## Development

We are using [Gerrit](https://gerrit.cesnet.cz/q/project:CzechLight%252Fnetconf-cli) for patch submission, code review and Continuous Integration (CI).
A [quick introduction](https://gerrit.cesnet.cz/Documentation/intro-user.html) is recommended for first-time Gerrit users.
Choose *CESNET - Sign in with GitHub* for login.
Development roadmap and planning happens [over Taiga.io](https://tree.taiga.io/project/jktjkt-netconf-cli/kanban).

## Credits

Copyright © CESNET, https://www.cesnet.cz/ .
Portions copyright © Faculty of Information Technology, Czech Technical University in Prague, https://fit.cvut.cz/ .
Most of the code was written by Václav Kubernát (CESNET, formerly FIT ČVUT) and Jan Kundrát (CESNET).
The project is distributed under the terms of the Apache 2.0 license.
