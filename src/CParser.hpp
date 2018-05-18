/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/

#pragma once
#include <boost/spirit/home/x3.hpp>
#include "grammars.hpp"
namespace x3 = boost::spirit::x3;
namespace ascii = boost::spirit::x3::ascii;
using Cmd = std::vector<std::string>;
using x3::alpha;
using x3::lit;
using x3::char_;
using x3::_attr;
using x3::lexeme;
using ascii::space;

class InvalidCommandException : public std::invalid_argument {
public:
    using std::invalid_argument::invalid_argument;
    ~InvalidCommandException() override;
};

class TooManyArgumentsException : public std::invalid_argument {
public:
    using std::invalid_argument::invalid_argument;
    ~TooManyArgumentsException() override;
};


class CParser {
public:
    CParser(const CTree& tree);
    cd_ parseCommand(const std::string& line, std::ostream& errorStream);

private:
    const CTree& m_tree;
};
