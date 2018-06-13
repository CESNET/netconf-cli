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

    auto schema = std::make_shared<StaticSchema>();
    schema->addContainer("", "a", yang::ContainerTraits::Presence);
    schema->addContainer("", "b");
    schema->addLeaf("", "leafString", yang::LeafDataTypes::String);
    schema->addLeaf("", "leafDecimal", yang::LeafDataTypes::Decimal);
    schema->addLeaf("", "leafBool", yang::LeafDataTypes::Bool);
    schema->addLeaf("", "leafInt", yang::LeafDataTypes::Int);
    schema->addLeaf("", "leafUint", yang::LeafDataTypes::Uint);
    schema->addLeafEnum("", "leafEnum", {"lol", "data", "coze"});
    schema->addContainer("a", "a2");
    schema->addLeaf("a", "leafa", yang::LeafDataTypes::String);
    schema->addContainer("b", "b2", yang::ContainerTraits::Presence);
    schema->addContainer("a/a2", "a3", yang::ContainerTraits::Presence);
    schema->addContainer("b/b2", "b3");
    schema->addList("", "list", {"number"});
    schema->addContainer("list", "contInList", yang::ContainerTraits::Presence);
    schema->addList("", "twoKeyList", {"number", "name"});
    Parser parser(schema);

    while (true) {
        std::cout << parser.currentNode() << "> ";
        std::string input;
        std::getline(std::cin, input);
        if (std::cin.eof())
            break;

        try {
            command_ cmd = parser.parseCommand(input, std::cout);
            boost::apply_visitor(Interpreter(parser, *schema), cmd);
        } catch (InvalidCommandException& ex) {
            std::cerr << ex.what() << std::endl;
        }
    }

    return 0;
}
