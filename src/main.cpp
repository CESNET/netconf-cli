/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/

#include <docopt.h>
#include <experimental/filesystem>
#include <iostream>
#include "NETCONF_CLI_VERSION.h"
#include "interpreter.hpp"
#include "sysrepo_access.hpp"
#include "yang_schema.hpp"


static const char usage[] =
    R"(CLI interface to remote NETCONF hosts

Usage:
  netconf-cli
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
    std::cout << "Welcome to netconf-cli" << std::endl;

    SysrepoAccess datastore("netconf-cli");
    Parser parser(datastore.schema());

    while (true) {
        std::cout << parser.currentNode() << "> ";
        std::string input;
        std::getline(std::cin, input);
        if (std::cin.eof())
            break;

        std::locale C_locale("C");
        if (std::all_of(input.begin(), input.end(),
                        [C_locale](const auto c) { return std::isspace(c, C_locale);})) {
            continue;
        }

        try {
            command_ cmd = parser.parseCommand(input, std::cout);
            boost::apply_visitor(Interpreter(parser, datastore), cmd);
        } catch (InvalidCommandException& ex) {
            std::cerr << ex.what() << std::endl;
        }
    }

    return 0;
}
