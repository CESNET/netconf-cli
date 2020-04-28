/*
 * Copyright (C) 2020 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
*/

#pragma once

#include <boost/spirit/home/x3.hpp>
#include "ast_handlers.hpp"
#include "common_parsers.hpp"
#include "leaf_data.hpp"

namespace x3 = boost::spirit::x3;

x3::rule<dataPath_class, dataPath_> const dataPath = "dataPath";
x3::rule<schemaPath_class, schemaPath_> const schemaPath = "schemaPath";
x3::rule<dataNodeList_class, decltype(dataPath_::m_nodes)::value_type> const dataNodeList = "dataNodeList";
x3::rule<dataNodesListEnd_class, decltype(dataPath_::m_nodes)> const dataNodesListEnd = "dataNodesListEnd";
x3::rule<dataPathListEnd_class, dataPath_> const dataPathListEnd = "dataPathListEnd";
x3::rule<leaf_path_class, dataPath_> const leafPath = "leafPath";
x3::rule<presenceContainerPath_class, dataPath_> const presenceContainerPath = "presenceContainerPath";
x3::rule<listInstancePath_class, dataPath_> const listInstancePath = "listInstancePath";
x3::rule<initializePath_class, x3::unused_type> const initializePath = "initializePath";
x3::rule<createPathSuggestions_class, x3::unused_type> const createPathSuggestions = "createPathSuggestions";
x3::rule<trailingSlash_class, TrailingSlash> const trailingSlash = "trailingSlash";
x3::rule<dataNode_class, dataNode_> const dataNode = "dataNode";
x3::rule<absoluteStart_class, Scope> const absoluteStart = "absoluteStart";
x3::rule<keyValue_class, keyValue_> const keyValue = "keyValue";
x3::rule<key_identifier_class, std::string> const key_identifier = "key_identifier";
x3::rule<listPrefix_class, std::string> const listPrefix = "listPrefix";
x3::rule<listSuffix_class, std::vector<keyValue_>> const listSuffix = "listSuffix";
x3::rule<listElement_class, listElement_> const listElement = "listElement";
x3::rule<list_class, list_> const list = "list";
x3::rule<nodeup_class, nodeup_> const nodeup = "nodeup";
x3::rule<container_class, container_> const container = "container";
x3::rule<leaf_class, leaf_> const leaf = "leaf";
x3::rule<createKeySuggestions_class, x3::unused_type> const createKeySuggestions = "createKeySuggestions";
x3::rule<createValueSuggestions_class, x3::unused_type> const createValueSuggestions = "createValueSuggestions";
x3::rule<suggestKeysEnd_class, x3::unused_type> const suggestKeysEnd = "suggestKeysEnd";


struct schemaNode : x3::parser<schemaNode> {
    using attribute_type = schemaNode_;
    template <typename It, typename Ctx, typename RCtx, typename Attr>
    bool parse(It& begin, It end, Ctx const& ctx, RCtx& rctx, Attr& attr) const
    {
        x3::symbols<schemaNode_> table(std::string{"schemaNode"}); // The constructor doesn't work with just the string literal

        ParserContext& parserContext = x3::get<parser_context_tag>(ctx);
        parserContext.m_suggestions.clear();
        for (const auto& child : parserContext.m_schema.availableNodes(parserContext.currentSchemaPath(), Recursion::NonRecursive)) {
            schemaNode_ out;
            std::string parseString;
            if (child.first) {
                out.m_prefix = module_{*child.first};
                parseString = *child.first + ":";
            }
            parseString += child.second;
            switch (parserContext.m_schema.nodeType(parserContext.currentSchemaPath(), child)) {
                case yang::NodeTypes::Container:
                case yang::NodeTypes::PresenceContainer:
                    out.m_suffix = container_{child.second};
                    parserContext.m_suggestions.emplace(Completion{parseString + "/"});
                    break;
                case yang::NodeTypes::Leaf:
                    out.m_suffix = leaf_{child.second};
                    parserContext.m_suggestions.emplace(Completion{parseString + " "});
                    break;
                case yang::NodeTypes::List:
                    out.m_suffix = list_{child.second};
                    parserContext.m_suggestions.emplace(Completion{parseString, "[", Completion::WhenToAdd::IfFullMatch});
                    break;
                case yang::NodeTypes::Action:
                case yang::NodeTypes::AnyXml:
                case yang::NodeTypes::LeafList:
                case yang::NodeTypes::Notification:
                case yang::NodeTypes::Rpc:
                    continue;
            }
            table.add(parseString, out);
            table.add("..", attribute_type{nodeup_{}});
            if (!child.first) {
                auto topLevelModule = parserContext.currentSchemaPath().m_nodes.begin()->m_prefix;
                out.m_prefix = topLevelModule;
                table.add(topLevelModule->m_name + ":" + parseString, out);
            }
        }
        auto res = table.parse(begin, end, ctx, rctx, attr);
        if (res) {
            parserContext.pushPathFragment(attr);
        }
        return res;
    }
} schemaNode;

#if __clang__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Woverloaded-shift-op-parentheses"
#endif

auto const rest =
    x3::omit[x3::no_skip[+(x3::char_ - '/' - space_separator)]];

auto const key_identifier_def =
    x3::lexeme[
        ((x3::alpha | char_("_")) >> *(x3::alnum | char_("_") | char_("-") | char_(".")))
    ];

auto const createKeySuggestions_def =
    x3::eps;

auto const createValueSuggestions_def =
    x3::eps;

auto const suggestKeysEnd_def =
    x3::eps;

auto const keyValue_def =
    key_identifier > '=' > createValueSuggestions > leaf_data;

auto const keyValueWrapper =
    x3::lexeme['[' > createKeySuggestions > keyValue > suggestKeysEnd > ']'];

auto const listPrefix_def =
    node_identifier;

// even though we don't allow no keys to be supplied, the star allows me to check which keys are missing
auto const listSuffix_def =
    *keyValueWrapper;

auto const listElement_def =
    listPrefix >> &char_('[') > listSuffix;

auto const list_def =
    node_identifier >> !char_('[');

auto const nodeup_def =
    x3::lit("..") > x3::attr(nodeup_());

auto const container_def =
    node_identifier;

auto const leaf_def =
    node_identifier;

auto const absoluteStart_def =
    x3::omit['/'] >> x3::attr(Scope::Absolute);

auto const trailingSlash_def =
    x3::omit['/'] >> x3::attr(TrailingSlash::Present);

auto const createPathSuggestions_def =
    x3::eps;

auto const dataNode_def =
    createPathSuggestions >> -(module) >> (container | listElement | nodeup | leaf);

// I have to insert an empty vector to the first alternative, otherwise they won't have the same attribute
auto const dataPath_def = initializePath >> absoluteStart >> createPathSuggestions >> x3::attr(decltype(dataPath_::m_nodes)()) >> x3::attr(TrailingSlash::NonPresent) >> x3::eoi | initializePath >> -(absoluteStart >> createPathSuggestions) >> dataNode % '/' >> (-(trailingSlash >> createPathSuggestions) >> -(completing >> rest) >> (&space_separator | x3::eoi));

auto const dataNodeList_def =
    createPathSuggestions >> -(module) >> list;

// This intermediate rule is mandatory, because we need the first alternative
// to be collapsed to a vector. If we didn't use the intermediate rule,
// Spirit wouldn't know we want it to collapse.
// https://github.com/boostorg/spirit/issues/408
auto const dataNodesListEnd_def =
    dataNode % '/' >> '/' >> dataNodeList >> -(&char_('/') >> createPathSuggestions) |
    x3::attr(decltype(dataPath_::m_nodes)()) >> dataNodeList;

auto const dataPathListEnd_def = initializePath >> absoluteStart >> createPathSuggestions >> x3::attr(decltype(dataPath_::m_nodes)()) >> x3::attr(TrailingSlash::NonPresent) >> x3::eoi | initializePath >> -(absoluteStart >> createPathSuggestions) >> dataNodesListEnd >> (-(trailingSlash >> createPathSuggestions) >> -(completing >> rest) >> (&space_separator | x3::eoi));

auto const schemaPath_def = initializePath >> absoluteStart >> createPathSuggestions >> x3::attr(decltype(schemaPath_::m_nodes)()) >> x3::attr(TrailingSlash::NonPresent) >> x3::eoi | initializePath >> -(absoluteStart >> createPathSuggestions) >> schemaNode % '/' >> (-(trailingSlash >> createPathSuggestions) >> -(completing >> rest) >> (&space_separator | x3::eoi));

auto const leafPath_def =
    dataPath;

auto const presenceContainerPath_def =
    dataPath;

auto const listInstancePath_def =
    dataPath;

// A "nothing" parser, which is used to indicate we tried to parse a path
auto const initializePath_def =
    x3::eps;



#if __clang__
#pragma GCC diagnostic pop
#endif

BOOST_SPIRIT_DEFINE(keyValue)
BOOST_SPIRIT_DEFINE(key_identifier)
BOOST_SPIRIT_DEFINE(listPrefix)
BOOST_SPIRIT_DEFINE(listSuffix)
BOOST_SPIRIT_DEFINE(listElement)
BOOST_SPIRIT_DEFINE(list)
BOOST_SPIRIT_DEFINE(nodeup)
BOOST_SPIRIT_DEFINE(dataNode)
BOOST_SPIRIT_DEFINE(container)
BOOST_SPIRIT_DEFINE(leaf)
BOOST_SPIRIT_DEFINE(dataNodeList)
BOOST_SPIRIT_DEFINE(dataNodesListEnd)
BOOST_SPIRIT_DEFINE(leafPath)
BOOST_SPIRIT_DEFINE(presenceContainerPath)
BOOST_SPIRIT_DEFINE(listInstancePath)
BOOST_SPIRIT_DEFINE(schemaPath)
BOOST_SPIRIT_DEFINE(dataPath)
BOOST_SPIRIT_DEFINE(dataPathListEnd)
BOOST_SPIRIT_DEFINE(initializePath)
BOOST_SPIRIT_DEFINE(createKeySuggestions)
BOOST_SPIRIT_DEFINE(createPathSuggestions)
BOOST_SPIRIT_DEFINE(createValueSuggestions)
BOOST_SPIRIT_DEFINE(suggestKeysEnd)
BOOST_SPIRIT_DEFINE(absoluteStart)
BOOST_SPIRIT_DEFINE(trailingSlash)
