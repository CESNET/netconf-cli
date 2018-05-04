/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/
#pragma once
#include <boost/spirit/home/x3.hpp>
#include <boost/spirit/home/x3/support/ast/position_tagged.hpp>

#include <boost/fusion/adapted/struct/adapt_struct.hpp>
#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/variant.hpp>
#include <vector>

#include "CTree.hpp"
#include "utils.hpp"
namespace x3 = boost::spirit::x3;
namespace ascii = boost::spirit::x3::ascii;

using x3::alpha;
using x3::alnum;
using x3::lit;
using x3::char_;
using x3::_attr;
using x3::lexeme;
using ascii::space;

struct ParserContext {
    ParserContext(const CTree& tree);
    const CTree& m_tree;
    std::string m_curPath;
};

struct parser_context_tag;

struct container_ {
    container_() {}
    container_(const std::string& name);

    bool operator==(const container_& b) const;

    std::string m_name;
};

BOOST_FUSION_ADAPT_STRUCT(container_, m_name)

struct listElement_ {
    listElement_() {}
    listElement_(const std::string& listName, const std::string& key);

    bool operator==(const listElement_& b) const;

    std::string m_listName;
    std::string m_key;
};

BOOST_FUSION_ADAPT_STRUCT(listElement_, m_listName, m_key)


struct path_ {
    bool operator==(const path_& b) const;
    std::vector<boost::variant<container_, listElement_>> m_nodes;
};

BOOST_FUSION_ADAPT_STRUCT(path_, m_nodes)

struct cd_ {
    bool operator==(const cd_& b) const;
    path_ m_path;
};

BOOST_FUSION_ADAPT_STRUCT(cd_, m_path)

struct identifier_class;

struct listElement_class {
    template <typename T, typename Iterator, typename Context>
    void on_success(Iterator const&, Iterator const&, T& ast, Context const& context)
    {
        auto& parserContext = x3::get<parser_context_tag>(context);
        const auto& tree = parserContext.m_tree;
        if (tree.isListElement(parserContext.m_curPath, ast.m_listName, ast.m_key)) {
            parserContext.m_curPath += joinPaths(parserContext.m_curPath, ast.m_listName);
            parserContext.m_curPath += "[" + ast.m_key + "]";
        } else {
            _pass(context) = false;
        }
    }
};

struct container_class {
    template <typename T, typename Iterator, typename Context>
    void on_success(Iterator const&, Iterator const&, T& ast, Context const& context)
    {
        auto& parserContext = x3::get<parser_context_tag>(context);
        const auto& tree = parserContext.m_tree;

        if (tree.isContainer(parserContext.m_curPath, ast.m_name)) {
            parserContext.m_curPath += joinPaths(parserContext.m_curPath, ast.m_name);
        } else {
            _pass(context) = false;
        }
    }
};

struct path_class {
    template <typename T, typename Iterator, typename Context>
    void on_success(Iterator const&, Iterator const&, T&, Context const&)
    {
    }
};

struct cd_class {
    template <typename T, typename Iterator, typename Context>
    void on_success(Iterator const&, Iterator const&, T&, Context const&)
    {
    }
};


x3::rule<identifier_class, std::string> const identifier = "indentifier";
x3::rule<listElement_class, listElement_> const listElement = "listElement";
x3::rule<container_class, container_> const container = "container";
x3::rule<path_class, path_> const path = "path";
x3::rule<cd_class, cd_> const cd = "cd";


auto const identifier_def =
    lexeme[
        ((alpha | char_("_")) >> *(alnum | char_("_") | char_("-") | char_(".")))
    ];

auto const key =
    identifier;

auto const listElement_def =
    identifier >> '[' >> key >> ']';
auto const container_def =
    identifier;

auto const path_def =
    (container | listElement) % '/';

auto const cd_def =
    lit("cd") >> path >> x3::eoi;

BOOST_SPIRIT_DEFINE(identifier)
BOOST_SPIRIT_DEFINE(listElement)
BOOST_SPIRIT_DEFINE(container)
BOOST_SPIRIT_DEFINE(path)
BOOST_SPIRIT_DEFINE(cd)
