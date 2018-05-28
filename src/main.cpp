/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/

#include <boost/spirit/home/x3.hpp>
#include <docopt.h>
#include <iostream>
#include <string>
#include "parser.hpp"
#include "schema.hpp"
#include "NETCONF_CLI_VERSION.h"


static const char usage[] =
    R"(CLI interface to remote NETCONF hosts

Usage:
  netconf-cli
  netconf-cli (-h | --help)
  netconf-cli --version
)";

namespace x3 = boost::spirit::x3;
namespace ascii = boost::spirit::x3::ascii;

using Cmd = std::vector<std::string>;
using x3::alpha;
using x3::lit;
using x3::char_;
using x3::_attr;
using x3::lexeme;
using ascii::space;

void interpret(cd_ command, Parser& parser)
{
    parser.changeNode(command.m_path);
}

int main(int argc, char* argv[])
{
    auto args = docopt::docopt(usage,
                               {argv + 1, argv + argc},
                               true,
                               "netconf-cli " NETCONF_CLI_VERSION,
                               true);
    std::cout << "Welcome to netconf-cli" << std::endl;

    Schema schema;
    schema.addContainer("", "a");
    schema.addContainer("", "b");
    schema.addContainer("a", "a2");
    schema.addContainer("b", "b2");
    schema.addContainer("a/a2", "a3");
    schema.addContainer("b/b2", "b3");
    schema.addList("", "list", {"number"});
    schema.addContainer("list", "contInList");
    schema.addList("", "twoKeyList", {"number", "name"});
    Parser parser(schema);

    while (true) {
        std::cout << parser.currentNode() << ">";
        std::string input;
        std::getline(std::cin, input);
        if (std::cin.eof())
            break;

        try {
            cd_ command = parser.parseCommand(input, std::cout);
            interpret(command, parser);
        } catch (InvalidCommandException& ex) {
            std::cerr << ex.what() << std::endl;
        }
    }

    return 0;
}
