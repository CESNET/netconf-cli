#pragma once
#include <boost/spirit/home/x3.hpp>
#include "ast_handlers.hpp"
x3::rule<module_identifier_class, std::string> const module_identifier = "module_identifier";
x3::rule<module_class, module_> const module = "module";
x3::rule<node_identifier_class, std::string> const node_identifier = "node_identifier";
auto const node_identifier_def =
    x3::lexeme[
            ((x3::alpha | x3::char_("_")) >> *(x3::alnum | x3::char_("_") | x3::char_("-") | x3::char_(".")))
    ];

auto const module_def =
    module_identifier >> x3::no_skip[':'] >> !x3::no_skip[x3::space];

auto const module_identifier_def =
    x3::lexeme[
            ((x3::alpha | x3::char_("_")) >> *(x3::alnum | x3::char_("_") | x3::char_("-") | x3::char_(".")))
    ];

BOOST_SPIRIT_DEFINE(node_identifier)
BOOST_SPIRIT_DEFINE(module)
BOOST_SPIRIT_DEFINE(module_identifier)
