/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Jan Kundrát <jan.kundrat@cesnet.cz>
 *
*/

#include <docopt.h>
#include <string>
#include <iostream>
#include <../submodules/spirit/include/boost/spirit/home/x3.hpp>
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
std::string curDir = "";

struct TCommand
{
    std::string m_keyword;
    std::string m_args;
};

void changeDir(const std::string& args)
{
    using x3::phrase_parse;
    using x3::char_;
    using ascii::space;
    if(phrase_parse(
                args.begin(),
                args.end(),
                *space >> x3::eoi,
                x3::eoi))
    {
        std::cout << "returned to root" << std::endl;
        curDir = "";
        return;
    }
    std::string dir;
    auto farg = [&](auto& val){ dir += _attr(val); };
    auto it = args.begin();
    bool r = phrase_parse(
            it,
            args.end(),
            +(char_ - ' ')[farg] >> (*space |x3::eoi), // parse the dirname(con't contain spaces)
            x3::eol);
    if(!r || it != args.end())
        std::cout << "Couldn't parse the argument: " << args << std::endl;
    else
    {
        curDir += "/";
        curDir += dir;
        std::cout << "cd to " << curDir<< std::endl;
    }
}

std::string getInput()
{
    std::string line;
    std::getline(std::cin, line);
    return line;
}
TCommand parseCommand(const std::string& line) //parses the keyword
{
    using x3::char_;
    using ascii::space;
    TCommand cmd;
    auto it = line.begin();
    auto fcmd = [&](auto& val){ cmd.m_keyword += _attr(val); };
    bool r = phrase_parse(
            it,
            line.end(),
            *space >> +(char_ - space)[fcmd] >> (+space|x3::eoi), //keyword parser
            x3::eol);
    cmd.m_args = std::string(it, line.end());
    return cmd;
}
void handleInput(const TCommand& cmd)
{
    if(cmd.m_keyword == "cd")
        changeDir(cmd.m_args);
    else
        std::cout << "I don't know the keyword \"" << cmd.m_keyword << "\"" << std::endl;
}


int main(int argc, char* argv[])
{
    auto args = docopt::docopt(usage,
                               {argv + 1, argv + argc},
                               true,
                               "netconf-cli " NETCONF_CLI_VERSION,
                               true);
    std::cout << "Welcome to netconf-cli" << std::endl;
    while(true)
    {
        std::cout << (curDir==""?"/":"") << curDir<< " >> ";
        std::string line = getInput();
        TCommand cmd = parseCommand(line);
        handleInput(cmd);
    }

    return 0;
}
