#pragma once
#include <boost/spirit/home/x3.hpp>
#include <vector>
#include <boost/spirit/home/x3/support/ast/position_tagged.hpp>
#include <boost/fusion/adapted/struct/adapt_struct.hpp>
#include <boost/fusion/include/adapt_struct.hpp>
#include "CTree.hpp"
namespace x3 = boost::spirit::x3;
namespace ascii = boost::spirit::x3::ascii;

using x3::alpha;
using x3::alnum;
using x3::lit;
using x3::char_;
using x3::_attr;
using x3::lexeme;
using x3::raw;
using ascii::space;

using nodeString = std::string;

struct ParserContext
{
    CTree& m_tree;
    std::string m_current_context;
};

struct parser_context_tag;



struct container_
{
    char m_first;
    std::string m_name;
};

BOOST_FUSION_ADAPT_STRUCT(container_, m_first, m_name)

struct path_
{
    std::vector<container_> m_nodes;
};

BOOST_FUSION_ADAPT_STRUCT(path_, m_nodes)

struct cd_
{
    path_ m_path;
};

BOOST_FUSION_ADAPT_STRUCT(cd_, m_path)

struct container_class
{
    template <typename T, typename Iterator, typename Context>
        inline void on_success(Iterator const& first, Iterator const& last
                , T& ast, Context const& context)
        {
            ast.m_name = ast.m_first + ast.m_name;
            std::cout <<"parsed " << ast.m_name << "(container)\n";
        }

};

struct path_class
{
    template <typename T, typename Iterator, typename Context>
    inline void on_success(Iterator const& first, Iterator const& last
    , T& ast, Context const& context)
    {

        std::cout << "parsed path:" << std::endl;
        for (auto it : ast.m_nodes)
            std::cout << it.m_name << std::endl;
    }

};

struct cd_class
{
    template <typename T, typename Iterator, typename Context>
    inline void on_success(Iterator const& first, Iterator const& last
    , T& ast, Context const& context)
    {

        std::cout << "parsed cd! final path:" << std::endl;
        for (auto it : ast.m_path.m_nodes)
            std::cout << it.m_name << std::endl;
    }

};

typedef x3::rule<container_class, container_> container_type;
typedef x3::rule<path_class, path_> path_type;
typedef x3::rule<cd_class, cd_> cd_type;

container_type const container = "container";
path_type const path = "path";
cd_type const cd = "cd";


auto const container_def =
    lexeme[
        ((alpha | char_('_')) >> *(alnum | char_('_') | char_('-') | char_('.')))
    ];
auto const path_def =
    container % '/';

auto const cd_def =
    lit("cd") >> path >> x3::eoi;

BOOST_SPIRIT_DEFINE(container);
BOOST_SPIRIT_DEFINE(path);
BOOST_SPIRIT_DEFINE(cd);

