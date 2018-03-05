/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/


#include <boost/spirit/home/x3.hpp>
#include <docopt.h>
#include "CTree.hpp"
#include <iostream>
#include <string>
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

CTree tree;


auto const checkNode =
[](auto& ctx)
{
    std::cout << "trying to enter " << _attr(ctx)<< std::endl;
    
    tree.checkNode(std::string(tree.currentNode() == "" ? "" : "/") + _attr(ctx));
};
auto const node_up =
    x3::string("..");
auto const nodename =
    +alpha;
auto const cd_args =
    (
     (+(node_up >> +(char_('/'))) >> -node_up) |
     (*(node_up >> +(char_('/'))) >> nodename[checkNode] >> *("/" >> nodename[checkNode]))
    )
    >> x3::eoi;
auto const cd = 
    x3::string("cd")
    >> cd_args;

auto const keywords = 
    cd;


std::string getInput()
{
    std::string line;
    std::getline(std::cin, line);
    return line;
}
Cmd parseInput(const std::string& line)
{
    Cmd args;
    auto it = line.begin();
    //auto fcmd = [&](auto& val) { cmd.m_keyword += _attr(val); };
    std::cout << "success: "<< x3::phrase_parse(it, line.end(), keywords, space, args) << std::endl;
    std::cout <<"end: "<< std::string(it, line.end())<< std::endl;
    std::cout << "cd-arg:" <<args.at(1) << std::endl;
    return args;
}
void handleInput(const Cmd& cmd)
{
    if (cmd.at(0) == "cd")
        tree.changeNode(cmd.at(1));
    else
        std::cout << "I don't know the keyword \"" << cmd.at(0) << "\"" << std::endl;
}


int main(int argc, char* argv[])
{
    auto args = docopt::docopt(usage,
                               {argv + 1, argv + argc},
                               true,
                               "netconf-cli " NETCONF_CLI_VERSION,
                               true);
    tree.initDefault();
    std::cout << "Welcome to netconf-cli" << std::endl;

    parseInput("cd aaa");
    parseInput("cd aaabbb");
    parseInput("cd ");
    parseInput("cd bbb");
    parseInput("cd uuu");
    

    return 0;
}
