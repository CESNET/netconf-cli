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
#include "3rdparty/linenoise/linenoise.hpp"
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
        linenoise::SetCompletionCallback([&parser](const char* editBuffer, std::vector<std::string>& completions) {
            std::stringstream stream;
            auto completionsSet = parser.completeCommand(editBuffer, stream);
            std::transform(completionsSet.begin(), completionsSet.end(), std::back_inserter(completions),
                           [editBuffer](auto it) { return std::string(editBuffer) + it; });
        });
        linenoise::SetHistoryMaxLen(4);
        linenoise::SetMultiLine(true);
        std::string line;
        auto quit = linenoise::Readline((parser.currentNode() + "> ").c_str(), line);
        if (quit) {
            break;
        }

        std::locale C_locale("C");
        if (std::all_of(line.begin(), line.end(),
                        [C_locale](const auto c) { return std::isspace(c, C_locale);})) {
            continue;
        }

        try {
            command_ cmd = parser.parseCommand(line, std::cout);
            boost::apply_visitor(Interpreter(parser, datastore), cmd);
        } catch (InvalidCommandException& ex) {
            std::cerr << ex.what() << std::endl;
        }

        linenoise::AddHistory(line.c_str());
    }

    return 0;
}
