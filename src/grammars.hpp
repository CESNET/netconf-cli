/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/

#pragma once

#include <boost/spirit/home/x3.hpp>
#include "ast_commands.hpp"
#include "ast_handlers.hpp"


x3::rule<keyValue_class, keyValue_> const keyValue = "keyValue";
x3::rule<key_identifier_class, std::string> const key_identifier = "key_identifier";
x3::rule<node_identifier_class, std::string> const node_identifier = "node_identifier";
x3::rule<module_identifier_class, std::string> const module_identifier = "module_identifier";
x3::rule<listPrefix_class, std::string> const listPrefix = "listPrefix";
x3::rule<listSuffix_class, std::vector<keyValue_>> const listSuffix = "listSuffix";
x3::rule<listElement_class, listElement_> const listElement = "listElement";
x3::rule<list_class, list_> const list = "list";
x3::rule<nodeup_class, nodeup_> const nodeup = "nodeup";
x3::rule<container_class, container_> const container = "container";
x3::rule<leaf_class, leaf_> const leaf = "leaf";
x3::rule<module_class, module_> const module = "module";
x3::rule<dataNode_class, dataNode_> const dataNode = "dataNode";
x3::rule<schemaNode_class, schemaNode_> const schemaNode = "schemaNode";
x3::rule<absoluteStart_class, Scope> const absoluteStart = "absoluteStart";
x3::rule<schemaPath_class, schemaPath_> const schemaPath = "schemaPath";
x3::rule<trailingSlash_class, TrailingSlash> const trailingSlash = "trailingSlash";
x3::rule<dataNodeList_class, decltype(dataPath_::m_nodes)::value_type> const dataNodeList = "dataNodeList";
x3::rule<dataNodesListEnd_class, decltype(dataPath_::m_nodes)> const dataNodesListEnd = "dataNodesListEnd";
x3::rule<dataPathListEnd_class, dataPath_> const dataPathListEnd = "dataPathListEnd";
x3::rule<dataPath_class, dataPath_> const dataPath = "dataPath";
x3::rule<leaf_path_class, dataPath_> const leafPath = "leafPath";
x3::rule<presenceContainerPath_class, dataPath_> const presenceContainerPath = "presenceContainerPath";
x3::rule<listInstancePath_class, dataPath_> const listInstancePath = "listInstancePath";
x3::rule<space_separator_class, x3::unused_type> const space_separator = "space_separator";

x3::rule<leaf_data_class, leaf_data_> const leaf_data = "leaf_data";
x3::rule<leaf_data_enum_class, enum_> const leaf_data_enum = "leaf_data_enum";
x3::rule<leaf_data_base_class<yang::LeafDataTypes::Decimal>, double> const leaf_data_decimal = "leaf_data_decimal";
x3::rule<leaf_data_base_class<yang::LeafDataTypes::Bool>, bool> const leaf_data_bool = "leaf_data_bool";
x3::rule<leaf_data_base_class<yang::LeafDataTypes::Int8>, int8_t> const leaf_data_int8 = "leaf_data_int8";
x3::rule<leaf_data_base_class<yang::LeafDataTypes::Uint8>, uint8_t> const leaf_data_uint8 = "leaf_data_uint8";
x3::rule<leaf_data_base_class<yang::LeafDataTypes::Int16>, int16_t> const leaf_data_int16 = "leaf_data_int16";
x3::rule<leaf_data_base_class<yang::LeafDataTypes::Uint16>, uint16_t> const leaf_data_uint16 = "leaf_data_uint16";
x3::rule<leaf_data_base_class<yang::LeafDataTypes::Int32>, int32_t> const leaf_data_int32 = "leaf_data_int32";
x3::rule<leaf_data_base_class<yang::LeafDataTypes::Uint32>, uint32_t> const leaf_data_uint32 = "leaf_data_uint32";
x3::rule<leaf_data_base_class<yang::LeafDataTypes::Int64>, int64_t> const leaf_data_int64 = "leaf_data_int64";
x3::rule<leaf_data_base_class<yang::LeafDataTypes::Uint64>, uint64_t> const leaf_data_uint64 = "leaf_data_uint64";
x3::rule<leaf_data_base_class<yang::LeafDataTypes::String>, std::string> const leaf_data_string = "leaf_data_string";
x3::rule<leaf_data_binary_data_class, std::string> const leaf_data_binary_data = "leaf_data_binary_data";
x3::rule<leaf_data_base_class<yang::LeafDataTypes::Binary>, binary_> const leaf_data_binary = "leaf_data_binary";
x3::rule<leaf_data_identityRef_data_class, identityRef_> const leaf_data_identityRef_data = "leaf_data_identityRef_data";
x3::rule<leaf_data_identityRef_class, identityRef_> const leaf_data_identityRef = "leaf_data_identityRef";

x3::rule<discard_class, discard_> const discard = "discard";
x3::rule<ls_class, ls_> const ls = "ls";
x3::rule<cd_class, cd_> const cd = "cd";
x3::rule<set_class, set_> const set = "set";
x3::rule<get_class, get_> const get = "get";
x3::rule<create_class, create_> const create = "create";
x3::rule<delete_class, delete_> const delete_rule = "delete_rule";
x3::rule<commit_class, commit_> const commit = "commit";
x3::rule<help_class, help_> const help = "help";
x3::rule<command_class, command_> const command = "command";

x3::rule<initializePath_class, x3::unused_type> const initializePath = "initializePath";
x3::rule<createPathSuggestions_class, x3::unused_type> const createPathSuggestions = "createPathSuggestions";
x3::rule<createKeySuggestions_class, x3::unused_type> const createKeySuggestions = "createKeySuggestions";
x3::rule<suggestKeysStart_class, x3::unused_type> const suggestKeysStart = "suggestKeysStart";
x3::rule<suggestKeysEnd_class, x3::unused_type> const suggestKeysEnd = "suggestKeysEnd";
x3::rule<createCommandSuggestions_class, x3::unused_type> const createCommandSuggestions = "createCommandSuggestions";
x3::rule<completing_class, x3::unused_type> const completing = "completing";
x3::rule<createEnumSuggestions_class, x3::unused_type> const createEnumSuggestions = "createEnumSuggestions";
x3::rule<createIdentitySuggestions_class, x3::unused_type> const createIdentitySuggestions = "createIdentitySuggestions";

#if __clang__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Woverloaded-shift-op-parentheses"
#endif

namespace ascii = boost::spirit::x3::ascii;

using ascii::space;
using x3::_attr;
using x3::alnum;
using x3::alpha;
using x3::char_;
using x3::double_;
using x3::expect;
using x3::lexeme;
using x3::lit;
using x3::int8;
using x3::int16;
using x3::int32;
using x3::int64;
using x3::uint8;
using x3::uint16;
using x3::uint32;
using x3::uint64;

auto const key_identifier_def =
    lexeme[
        ((alpha | char_("_")) >> *(alnum | char_("_") | char_("-") | char_(".")))
    ];

auto const quotedValue =
    ('\'' > +(char_-'\'') > '\'') |
    ('\"' > +(char_-'\"') > '\"');

auto const number =
    +x3::digit;

auto const createKeySuggestions_def =
    x3::eps;

auto const suggestKeysStart_def =
    x3::eps;

auto const suggestKeysEnd_def =
    x3::eps;

auto const keyValue_def =
    key_identifier > '=' > (quotedValue | number);

auto const keyValueWrapper =
    lexeme['[' > createKeySuggestions > keyValue > suggestKeysEnd > ']'];

auto const module_identifier_def =
    lexeme[
            ((alpha | char_("_")) >> *(alnum | char_("_") | char_("-") | char_(".")))
    ];

auto const node_identifier_def =
    lexeme[
            ((alpha | char_("_")) >> *(alnum | char_("_") | char_("-") | char_(".")))
    ];

auto const listPrefix_def =
    node_identifier;

// even though we don't allow no keys to be supplied, the star allows me to check which keys are missing
auto const listSuffix_def =
    *keyValueWrapper;

auto const listElement_def =
    listPrefix >> -(!char_('[') >> suggestKeysStart) >> &char_('[') > listSuffix;

auto const list_def =
    node_identifier >> !char_('[');

auto const nodeup_def =
    lit("..") > x3::attr(nodeup_());

auto const container_def =
    node_identifier;

auto const module_def =
    module_identifier >> x3::no_skip[':'] >> !x3::no_skip[space];

auto const leaf_def =
    node_identifier;

auto const createPathSuggestions_def =
    x3::eps;

// leaf cannot be in the middle of a path, however, I need the grammar's attribute to be a vector of variants
auto const schemaNode_def =
    createPathSuggestions >> -(module) >> (container | list | nodeup | leaf);

auto const dataNode_def =
    createPathSuggestions >> -(module) >> (container | listElement | nodeup | leaf);

auto const absoluteStart_def =
    x3::omit['/'] >> x3::attr(Scope::Absolute);

auto const trailingSlash_def =
    x3::omit['/'] >> x3::attr(TrailingSlash::Present);

auto const space_separator_def =
    x3::omit[x3::no_skip[space]];

// This is a pseudo-parser, that fails if we're not completing a command
auto const completing_def =
    x3::eps;

// I have to insert an empty vector to the first alternative, otherwise they won't have the same attribute
auto const dataPath_def =
    initializePath >> absoluteStart >> createPathSuggestions >> x3::attr(decltype(dataPath_::m_nodes)()) >> x3::attr(TrailingSlash::NonPresent) >> x3::eoi |
    initializePath >> -(absoluteStart >> createPathSuggestions) >> dataNode % '/' >> (trailingSlash >> createPathSuggestions >> (completing | x3::eoi) | (&space_separator | x3::eoi));

auto const dataNodeList_def =
    createPathSuggestions >> -(module) >> list;

// This intermediate rule is mandatory, because we need the first alternative
// to be collapsed to a vector. If we didn't use the intermediate rule,
// Spirit wouldn't know we want it to collapse.
// https://github.com/boostorg/spirit/issues/408
auto const dataNodesListEnd_def =
    initializePath >> dataNode % '/' >> '/' >> dataNodeList >> -(&char_('/') >> createPathSuggestions) |
    initializePath >> x3::attr(decltype(dataPath_::m_nodes)()) >> dataNodeList;

auto const dataPathListEnd_def =
    initializePath >> absoluteStart >> createPathSuggestions >> x3::attr(decltype(dataPath_::m_nodes)()) >> x3::attr(TrailingSlash::NonPresent) >> x3::eoi |
    initializePath >> -(absoluteStart >> createPathSuggestions) >> dataNodesListEnd >> (trailingSlash >> createPathSuggestions >> (completing | x3::eoi) | (&space_separator | x3::eoi));

auto const schemaPath_def =
    initializePath >> absoluteStart >> createPathSuggestions >> x3::attr(decltype(schemaPath_::m_nodes)()) >> x3::attr(TrailingSlash::NonPresent) >> x3::eoi |
    initializePath >> -(absoluteStart >> createPathSuggestions) >> schemaNode % '/' >> (trailingSlash >> createPathSuggestions >> (completing | x3::eoi) | (&space_separator | x3::eoi));

auto const leafPath_def =
    dataPath;

auto const presenceContainerPath_def =
    dataPath;

auto const listInstancePath_def =
    dataPath;

auto const createEnumSuggestions_def =
    x3::eps;

auto const leaf_data_enum_def =
    createEnumSuggestions >> +char_;
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
auto const leaf_data_int8_def =
    int8;
auto const leaf_data_int16_def =
    int16;
auto const leaf_data_int32_def =
    int32;
auto const leaf_data_int64_def =
    int64;
auto const leaf_data_uint8_def =
    uint8;
auto const leaf_data_uint16_def =
    uint16;
auto const leaf_data_uint32_def =
    uint32;
auto const leaf_data_uint64_def =
    uint64;
auto const leaf_data_string_def =
    *char_;

// This intermediate rule is neccessary for coercing to std::string.
auto const leaf_data_binary_data_def =
    +(x3::alnum | char_('+') | char_('/')) >> -char_('=') >> -char_('=');

auto const leaf_data_binary_def =
    leaf_data_binary_data;

auto const leaf_data_identityRef_data_def =
    -module  >> node_identifier;

auto const createIdentitySuggestions_def =
    x3::eps;

auto const leaf_data_identityRef_def =
    createIdentitySuggestions >> leaf_data_identityRef_data;

auto const leaf_data_def =
x3::expect[
    leaf_data_enum |
    leaf_data_decimal |
    leaf_data_bool |
    leaf_data_int8 |
    leaf_data_int16 |
    leaf_data_int32 |
    leaf_data_int64 |
    leaf_data_uint8 |
    leaf_data_uint16 |
    leaf_data_uint32 |
    leaf_data_uint64 |
    leaf_data_binary |
    leaf_data_identityRef |
    leaf_data_string];

struct ls_options_table : x3::symbols<LsOption> {
    ls_options_table()
    {
    add
        ("--recursive", LsOption::Recursive);
    }
} const ls_options;

// A "nothing" parser, which is used to indicate we tried to parse a path
auto const initializePath_def =
    x3::eps;

auto const ls_def =
    ls_::name >> *(space_separator >> ls_options) >> -(space_separator >> ((dataPathListEnd | dataPath | schemaPath) | (module >> "*")));

auto const cd_def =
    cd_::name >> space_separator > dataPath;

auto const create_def =
    create_::name >> space_separator > (presenceContainerPath | listInstancePath);

auto const delete_rule_def =
    delete_::name >> space_separator > (presenceContainerPath | listInstancePath);

auto const get_def =
    get_::name >> -(space_separator >> ((dataPathListEnd | dataPath) | (module >> "*")));

auto const set_def =
    set_::name >> space_separator > leafPath > space_separator > leaf_data;

auto const commit_def =
    commit_::name >> x3::attr(commit_());

auto const discard_def =
    discard_::name >> x3::attr(discard_());

struct command_names_table : x3::symbols<decltype(help_::m_cmd)> {
    command_names_table()
    {
        boost::mpl::for_each<CommandTypes, boost::type<boost::mpl::_>>([this](auto cmd) {
            add(commandNamesVisitor()(cmd), decltype(help_::m_cmd)(cmd));
        });
    }
} const command_names;

auto const help_def =
    help_::name > createCommandSuggestions >> -command_names;

auto const createCommandSuggestions_def =
    x3::eps;

auto const command_def =
    createCommandSuggestions >> x3::expect[cd | create | delete_rule | set | commit | get | ls | discard | help];

#if __clang__
#pragma GCC diagnostic pop
#endif

BOOST_SPIRIT_DEFINE(keyValue)
BOOST_SPIRIT_DEFINE(key_identifier)
BOOST_SPIRIT_DEFINE(node_identifier)
BOOST_SPIRIT_DEFINE(module_identifier)
BOOST_SPIRIT_DEFINE(listPrefix)
BOOST_SPIRIT_DEFINE(listSuffix)
BOOST_SPIRIT_DEFINE(listElement)
BOOST_SPIRIT_DEFINE(list)
BOOST_SPIRIT_DEFINE(nodeup)
BOOST_SPIRIT_DEFINE(schemaNode)
BOOST_SPIRIT_DEFINE(dataNode)
BOOST_SPIRIT_DEFINE(container)
BOOST_SPIRIT_DEFINE(leaf)
BOOST_SPIRIT_DEFINE(leafPath)
BOOST_SPIRIT_DEFINE(presenceContainerPath)
BOOST_SPIRIT_DEFINE(listInstancePath)
BOOST_SPIRIT_DEFINE(space_separator)
BOOST_SPIRIT_DEFINE(schemaPath)
BOOST_SPIRIT_DEFINE(dataPath)
BOOST_SPIRIT_DEFINE(dataNodeList)
BOOST_SPIRIT_DEFINE(dataNodesListEnd)
BOOST_SPIRIT_DEFINE(dataPathListEnd)
BOOST_SPIRIT_DEFINE(absoluteStart)
BOOST_SPIRIT_DEFINE(trailingSlash)
BOOST_SPIRIT_DEFINE(module)
BOOST_SPIRIT_DEFINE(leaf_data)
BOOST_SPIRIT_DEFINE(leaf_data_enum)
BOOST_SPIRIT_DEFINE(leaf_data_decimal)
BOOST_SPIRIT_DEFINE(leaf_data_bool)
BOOST_SPIRIT_DEFINE(leaf_data_int8)
BOOST_SPIRIT_DEFINE(leaf_data_int16)
BOOST_SPIRIT_DEFINE(leaf_data_int32)
BOOST_SPIRIT_DEFINE(leaf_data_int64)
BOOST_SPIRIT_DEFINE(leaf_data_uint8)
BOOST_SPIRIT_DEFINE(leaf_data_uint16)
BOOST_SPIRIT_DEFINE(leaf_data_uint32)
BOOST_SPIRIT_DEFINE(leaf_data_uint64)
BOOST_SPIRIT_DEFINE(leaf_data_string)
BOOST_SPIRIT_DEFINE(leaf_data_binary_data)
BOOST_SPIRIT_DEFINE(leaf_data_binary)
BOOST_SPIRIT_DEFINE(leaf_data_identityRef_data)
BOOST_SPIRIT_DEFINE(leaf_data_identityRef)
BOOST_SPIRIT_DEFINE(initializePath)
BOOST_SPIRIT_DEFINE(set)
BOOST_SPIRIT_DEFINE(commit)
BOOST_SPIRIT_DEFINE(get)
BOOST_SPIRIT_DEFINE(ls)
BOOST_SPIRIT_DEFINE(discard)
BOOST_SPIRIT_DEFINE(cd)
BOOST_SPIRIT_DEFINE(create)
BOOST_SPIRIT_DEFINE(delete_rule)
BOOST_SPIRIT_DEFINE(help)
BOOST_SPIRIT_DEFINE(command)
BOOST_SPIRIT_DEFINE(createPathSuggestions)
BOOST_SPIRIT_DEFINE(createKeySuggestions)
BOOST_SPIRIT_DEFINE(suggestKeysStart)
BOOST_SPIRIT_DEFINE(suggestKeysEnd)
BOOST_SPIRIT_DEFINE(createCommandSuggestions)
BOOST_SPIRIT_DEFINE(completing)
BOOST_SPIRIT_DEFINE(createEnumSuggestions)
BOOST_SPIRIT_DEFINE(createIdentitySuggestions)
