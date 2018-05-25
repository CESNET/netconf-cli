/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/

#pragma once

#include "ast.hpp"
#include "ast_handlers.hpp"


x3::rule<keyValue_class, keyValue_> const keyValue = "keyValue";
x3::rule<identifier_class, std::string> const identifier = "identifier";
x3::rule<listPrefix_class, std::string> const listPrefix = "listPrefix";
x3::rule<listSuffix_class, std::vector<keyValue_>> const listSuffix = "listSuffix";
x3::rule<listElement_class, listElement_> const listElement = "listElement";
x3::rule<nodeup_class, nodeup_> const nodeup = "nodeup";
x3::rule<container_class, container_> const container = "container";
x3::rule<path_class, path_> const path = "path";
x3::rule<cd_class, cd_> const cd = "cd";


auto const keyValue_def =
        lexeme[+alnum >> '=' >> +alnum];

auto const identifier_def =
        lexeme[
                ((alpha | char_("_")) >> *(alnum | char_("_") | char_("-") | char_(".")))
        ];

auto const listPrefix_def =
        identifier >> '[';

auto const listSuffix_def =
        *keyValue > ']';

auto const listElement_def =
        listPrefix > listSuffix;

auto const nodeup_def =
        x3::string("..");

auto const container_def =
        identifier;

auto const path_def =
        (container | listElement | nodeup) % '/';

auto const cd_def =
      lit("cd ") > path >> x3::eoi;

BOOST_SPIRIT_DEFINE(keyValue)
BOOST_SPIRIT_DEFINE(identifier)
BOOST_SPIRIT_DEFINE(listPrefix)
BOOST_SPIRIT_DEFINE(listSuffix)
BOOST_SPIRIT_DEFINE(listElement)
BOOST_SPIRIT_DEFINE(nodeup)
BOOST_SPIRIT_DEFINE(container)
BOOST_SPIRIT_DEFINE(path)
BOOST_SPIRIT_DEFINE(cd)
