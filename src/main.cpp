/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/

#include <docopt.h>
#include <filesystem>
#include <iostream>
#include "NETCONF_CLI_VERSION.h"
#include "interpreter.hpp"
#include "yang_schema.hpp"


static const char usage[] =
    R"(CLI interface to remote NETCONF hosts

Usage:
  netconf-cli <path-to-yang-schema>...
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

    auto yangschema = std::make_shared<YangSchema>();
    Parser parser(yangschema);

    for (auto it : args.at("<path-to-yang-schema>").asStringList()) {
        yangschema->addSchemaFile(it.c_str());
        auto dir = std::filesystem::absolute(it).remove_filename();
        yangschema->addSchemaDirectory(dir.c_str());
    }

    while (true) {
        std::cout << parser.currentNode() << "> ";
        std::string input;
        std::getline(std::cin, input);
        if (std::cin.eof())
            break;

        try {
            command_ cmd = parser.parseCommand(input, std::cout);
            boost::apply_visitor(Interpreter(parser, *yangschema), cmd);
        } catch (InvalidCommandException& ex) {
            std::cerr << ex.what() << std::endl;
        }
    }

    return 0;
}
