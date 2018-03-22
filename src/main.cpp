/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Jan Kundrát <jan.kundrat@cesnet.cz>
 *
*/

#include <docopt.h>
#include <string>
#include "NETCONF_CLI_VERSION.h"

static const char usage[] =
R"(CLI interface to remote NETCONF hosts

Usage:
  netconf-cli (-h | --help)
  netconf-cli --version
)";

int main(int argc, char* argv[])
{
    auto args = docopt::docopt(usage,
                               {argv + 1, argv + argc},
                               true,
                               "netconf-cli " NETCONF_CLI_VERSION,
                               true);
    return 0;
}
