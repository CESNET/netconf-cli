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
#include "CParser.hpp"
#include "CTree.hpp"
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


std::string getInput()
{
    std::string line;
    std::getline(std::cin, line);
    return line;
}

int main(int argc, char* argv[])
{
    auto args = docopt::docopt(usage,
                               {argv + 1, argv + argc},
                               true,
                               "netconf-cli " NETCONF_CLI_VERSION,
                               true);
    std::cout << "Welcome to netconf-cli" << std::endl;

    CTree tree;
    tree.addContainer("", "a");
    tree.addContainer("", "b");
    tree.addContainer("a", "a2");
    tree.addContainer("b", "b2");
    tree.addContainer("a/a2", "a3");
    tree.addContainer("b/b2", "b3");
    tree.addList("", "list", {"number"});
    tree.addContainer("list", "contInList");
    tree.addList("", "twoKeyList", {"number", "name"});
    CParser parser(tree);

    while(true)
    {
        std::cout << tree.currentNode() << ">";
        std::string input;
        std::getline(std::cin, input);


        try {
            cd_ command = parser.parseCommand(input, std::cout);
            tree.changeNode(command.m_path);
        }
        catch(InvalidCommandException ex)
        {
            std::cerr << "Couldn't parse " << input << std::endl;
        }

    }

    return 0;
}
