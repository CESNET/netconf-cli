/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
 */

#pragma once
#include <boost/spirit/home/x3.hpp>
#include "CTree.hpp"
namespace x3 = boost::spirit::x3;
namespace ascii = boost::spirit::x3::ascii;
using Cmd = std::vector<std::string>;
using x3::alpha;
using x3::lit;
using x3::char_;
using x3::_attr;
using x3::lexeme;
using ascii::space;

class TooManyArgumentsException : std::invalid_argument {
public:
    TooManyArgumentsException(const std::string& cmd_name)
        : std::invalid_argument(cmd_name)
    {
        m_msg = "Too many arguments : ";
        m_msg += std::invalid_argument::what();
    }
    const char* what() const noexcept
    {
        return m_msg.c_str();
    }

private:
    std::string m_msg;
};


class checkNode {
public:
    checkNode(const CTree& tree)
        : m_tree(tree)
    {
    }
    template <typename Context>
    void operator()(Context const& ctx) const
    {
        std::cout << "trying to enter " << _attr(ctx) << std::endl;
        m_tree.checkNode(m_tree.currentNode(), _attr(ctx));
    }

private:
    const CTree& m_tree;
};

class CParser {
public:
    CParser(const CTree& tree);
    Cmd parseInput(const std::string& line);

private:
    const CTree& m_tree;
};
