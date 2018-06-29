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
#include "NETCONF_CLI_VERSION.h"
#include "interpreter.hpp"
#include "parser.hpp"
#include "static_schema.hpp"


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
using ascii::space;
using x3::_attr;
using x3::alpha;
using x3::char_;
using x3::lexeme;
using x3::lit;


int main(int argc, char* argv[])
{
    auto args = docopt::docopt(usage,
                               {argv + 1, argv + argc},
                               true,
                               "netconf-cli " NETCONF_CLI_VERSION,
                               true);
    std::cout << "Welcome to netconf-cli" << std::endl;

    auto yangschema = std::make_shared<StaticSchema>();
    yangschema->addModule("mod");
    yangschema->addContainer("", "mod:a", yang::ContainerTraits::Presence);
    yangschema->addContainer("", "mod:b");
    yangschema->addContainer("mod:a", "mod:a2", yang::ContainerTraits::Presence);
    yangschema->addContainer("mod:b", "mod:b2");
    yangschema->addContainer("mod:a/mod:a2", "mod:a3");
    yangschema->addContainer("mod:b/mod:b2", "mod:b3");
    yangschema->addList("", "mod:list", {"number"});
    yangschema->addContainer("mod:list", "contInList");
    yangschema->addList("", "mod:twoKeyList", {"number", "name"});
    Parser parser(yangschema);

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
