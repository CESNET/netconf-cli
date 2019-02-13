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
#include <replxx.hxx>
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
    replxx::Replxx lineEditor;
    lineEditor.set_completion_callback([&parser](const std::string& input, int&) {
        std::stringstream stream;
        auto completionsSet = parser.completeCommand(input, stream);

        std::vector<std::string> res;
        std::transform(completionsSet.begin(), completionsSet.end(), std::back_inserter(res),
                [input](auto it) { return it; });
        return res;
    });
    lineEditor.set_word_break_characters(" '/[");

    while (true) {
        auto line = lineEditor.input((parser.currentNode() + "> ").c_str());
        if (!line) {
            break;
        }

        std::locale C_locale("C");
        std::string_view view = line;
        if (std::all_of(view.begin(), view.end(),
                        [C_locale](const auto c) { return std::isspace(c, C_locale);})) {
            continue;
        }

        try {
            command_ cmd = parser.parseCommand(line, std::cout);
            boost::apply_visitor(Interpreter(parser, datastore), cmd);
        } catch (InvalidCommandException& ex) {
            std::cerr << ex.what() << std::endl;
        }

        lineEditor.history_add(line);
    }

    return 0;
}
