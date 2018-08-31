/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/

#pragma once

#include "ast_commands.hpp"
#include "ast_handlers.hpp"


x3::rule<keyValue_class, keyValue_> const keyValue = "keyValue";
x3::rule<node_identifier_class, std::string> const node_identifier = "node_identifier";
x3::rule<module_identifier_class, std::string> const module_identifier = "module_identifier";
x3::rule<listPrefix_class, std::string> const listPrefix = "listPrefix";
x3::rule<listSuffix_class, std::vector<keyValue_>> const listSuffix = "listSuffix";
x3::rule<listElement_class, listElement_> const listElement = "listElement";
x3::rule<nodeup_class, nodeup_> const nodeup = "nodeup";
x3::rule<container_class, container_> const container = "container";
x3::rule<leaf_class, leaf_> const leaf = "leaf";
x3::rule<module_class, module_> const module = "module";
x3::rule<node_class, node_> const node = "node";
x3::rule<absoluteStart_class, Scope> const absoluteStart = "absoluteStart";
x3::rule<path_class, path_> const path = "path";
x3::rule<leaf_path_class, path_> const leafPath = "leafPath";

x3::rule<leaf_data_class, leaf_data_> const leaf_data = "leaf_data";
x3::rule<leaf_data_enum_class, enum_> const leaf_data_enum = "leaf_data_enum";
x3::rule<leaf_data_decimal_class, double> const leaf_data_decimal = "leaf_data_decimal";
x3::rule<leaf_data_bool_class, bool> const leaf_data_bool = "leaf_data_bool";
x3::rule<leaf_data_int_class, int32_t> const leaf_data_int = "leaf_data_int";
x3::rule<leaf_data_uint_class, uint32_t> const leaf_data_uint = "leaf_data_uint";
x3::rule<leaf_data_string_class, std::string> const leaf_data_string = "leaf_data_string";

x3::rule<ls_class, ls_> const ls = "ls";
x3::rule<cd_class, cd_> const cd = "cd";
x3::rule<set_class, set_> const set = "set";
x3::rule<get_class, get_> const get = "get";
x3::rule<create_class, create_> const create = "create";
x3::rule<delete_class, delete_> const delete_rule = "delete_rule";
x3::rule<commit_class, commit_> const commit = "commit";
x3::rule<command_class, command_> const command = "command";

#if __clang__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Woverloaded-shift-op-parentheses"
#endif

auto const keyValue_def =
        lexeme[+alnum > '=' > +alnum];

auto const module_identifier_def =
        lexeme[
                ((alpha | char_("_")) >> *(alnum | char_("_") | char_("-") | char_(".")))
        ];

auto const node_identifier_def =
        lexeme[
                ((alpha | char_("_")) >> *(alnum | char_("_") | char_("-") | char_(".")))
        ];

auto const listPrefix_def =
        node_identifier >> '[';

// even though we don't allow no keys to be supplied, the star allows me to check which keys are missing
auto const listSuffix_def =
        *keyValue > ']';

auto const listElement_def =
        listPrefix > listSuffix;

auto const nodeup_def =
        lit("..") > x3::attr(nodeup_());

auto const container_def =
        node_identifier;

auto const module_def =
        module_identifier >> x3::no_skip[':'] >> !x3::no_skip[space];

auto const leaf_def =
        node_identifier;

// leaf cannot be in the middle of a path, however, I need the grammar's attribute to be a vector of variants
auto const node_def =
        -(module) >> x3::expect[container | listElement | nodeup | leaf];

auto const absoluteStart_def =
        x3::omit['/'] >> x3::attr(Scope::Absolute);

// I have to insert an empty vector to the first alternative, otherwise they won't have the same attribute
auto const path_def =
        absoluteStart >> x3::attr(decltype(path_::m_nodes)()) >> x3::eoi |
        -(absoluteStart) >> node % '/';

auto const leafPath_def =
        path;

auto const leaf_data_enum_def =
        +char_;
auto const leaf_data_decimal_def =
        double_;

struct bool_symbol_table : x3::symbols<bool> {
    bool_symbol_table()
    {
        add
            ("true", true)
            ("false", false);
    }
} const bool_rule;

auto const leaf_data_bool_def =
        bool_rule;
auto const leaf_data_int_def =
        int_;
auto const leaf_data_uint_def =
        uint_;
auto const leaf_data_string_def =
        *char_;

auto const leaf_data_def =
x3::expect[
        leaf_data_enum |
        leaf_data_decimal |
        leaf_data_bool |
        leaf_data_int |
        leaf_data_uint |
        leaf_data_string];

auto const space_separator =
        x3::omit[x3::no_skip[space]];

auto const ls_def =
        lit("ls") >> -path;

auto const cd_def =
        lit("cd") >> space_separator > path;

auto const create_def =
        lit("create") >> space_separator > path;

auto const delete_rule_def =
        lit("delete") >> space_separator > path;

auto const get_def =
        lit("get") >> space_separator > path;

auto const set_def =
        lit("set") >> space_separator > leafPath > leaf_data;

auto const commit_def =
        lit("commit") >> x3::attr(commit_());

auto const command_def =
        x3::expect[cd | create | delete_rule | set | get | commit | ls] >> x3::eoi;

#if __clang__
#pragma GCC diagnostic pop
#endif

BOOST_SPIRIT_DEFINE(keyValue)
BOOST_SPIRIT_DEFINE(node_identifier)
BOOST_SPIRIT_DEFINE(module_identifier)
BOOST_SPIRIT_DEFINE(listPrefix)
BOOST_SPIRIT_DEFINE(listSuffix)
BOOST_SPIRIT_DEFINE(listElement)
BOOST_SPIRIT_DEFINE(nodeup)
BOOST_SPIRIT_DEFINE(container)
BOOST_SPIRIT_DEFINE(leaf)
BOOST_SPIRIT_DEFINE(leafPath)
BOOST_SPIRIT_DEFINE(node)
BOOST_SPIRIT_DEFINE(absoluteStart)
BOOST_SPIRIT_DEFINE(path)
BOOST_SPIRIT_DEFINE(module)
BOOST_SPIRIT_DEFINE(leaf_data)
BOOST_SPIRIT_DEFINE(leaf_data_enum)
BOOST_SPIRIT_DEFINE(leaf_data_decimal)
BOOST_SPIRIT_DEFINE(leaf_data_bool)
BOOST_SPIRIT_DEFINE(leaf_data_int)
BOOST_SPIRIT_DEFINE(leaf_data_uint)
BOOST_SPIRIT_DEFINE(leaf_data_string)
BOOST_SPIRIT_DEFINE(set)
BOOST_SPIRIT_DEFINE(get)
BOOST_SPIRIT_DEFINE(commit)
BOOST_SPIRIT_DEFINE(ls)
BOOST_SPIRIT_DEFINE(cd)
BOOST_SPIRIT_DEFINE(create)
BOOST_SPIRIT_DEFINE(delete_rule)
BOOST_SPIRIT_DEFINE(command)
