/*
 * Copyright (C) 2020 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
*/

#pragma once
#include <boost/spirit/home/x3.hpp>
#include "ast_handlers.hpp"

// This is a pseudo-parser, that fails if we're not completing a command
auto const completing = x3::rule<completing_class, x3::unused_type>{"completing"} =
    x3::eps;

auto const node_identifier = x3::rule<node_identifier_class, std::string>{"node_identifier"} =
    ((x3::alpha | x3::char_("_")) >> *(x3::alnum | x3::char_("_") | x3::char_("-") | x3::char_(".")));

auto const module_identifier = x3::rule<module_identifier_class, std::string>{"module_identifier"} =
    ((x3::alpha | x3::char_("_")) >> *(x3::alnum | x3::char_("_") | x3::char_("-") | x3::char_(".")));

auto const module = x3::rule<module_class, module_>{"module"} =
    module_identifier >> ':' >> !x3::space;

auto const space_separator = x3::rule<space_separator_class, x3::unused_type>{"a space"} =
    x3::omit[+x3::space];

template <typename CoerceTo>
struct as_type {
    template <typename...> struct Tag{};

    template <typename ParserType>
    auto operator[](ParserType p) const {
        return x3::rule<Tag<CoerceTo, ParserType>, CoerceTo> {"as"} = x3::as_parser(p);
    }
};

// The `as` parser creates an ad-hoc x3::rule with the attribute specified with `CoerceTo`.
// Example usage: as<std::string>[someParser]
// someParser will have its attribute coerced to std::string
// https://github.com/boostorg/spirit/issues/530#issuecomment-584836532
template <typename CoerceTo> const as_type<CoerceTo> as{};
