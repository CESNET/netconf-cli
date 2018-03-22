/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
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

/*struct TCommand {
    std::string m_keyword;
    std::string m_args;
};*/


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
void handleInput(const Cmd& cmd, CTree& tree)
{
    if (cmd.at(0) == "cd") {
        if (cmd.size() == 1)
            tree.changeNode("");
        else
            tree.changeNode(cmd.at(1));
    } else
        std::cout << "I don't know the keyword \"" << cmd.at(0) << "\"" << std::endl;
}


int main(int argc, char* argv[])
{
    auto args = docopt::docopt(usage,
                               {argv + 1, argv + argc},
                               true,
                               "netconf-cli " NETCONF_CLI_VERSION,
                               true);
    std::cout << "Welcome to netconf-cli" << std::endl;
    /*CTree tree;
    tree.initDefault();
    CParser parser(tree);

    try {

        handleInput(parser.parseInput("   cd    aaa   "), tree);
        handleInput(parser.parseInput("cd aaabbb  sda"), tree);
        std::cout << "curdir = " << tree.currentNode() << std::endl;

        handleInput(parser.parseInput("cd "), tree);
        std::cout << "curdir = " << tree.currentNode() << std::endl;
    } catch (TooManyArgumentsException ex) {
        std::cout << ex.what() << std::endl;
    } catch (InvalidNodeException ex) {
        std::cout << ex.what() << std::endl;
    }*/
    return 0;
}
