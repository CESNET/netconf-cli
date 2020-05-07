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

x3::rule<dataNodeList_class, decltype(dataPath_::m_nodes)::value_type> const dataNodeList = "dataNodeList";
x3::rule<dataNodesListEnd_class, decltype(dataPath_::m_nodes)> const dataNodesListEnd = "dataNodesListEnd";
x3::rule<dataPathListEnd_class, dataPath_> const dataPathListEnd = "dataPathListEnd";
x3::rule<leaf_path_class, dataPath_> const leafPath = "leafPath";
x3::rule<presenceContainerPath_class, dataPath_> const presenceContainerPath = "presenceContainerPath";
x3::rule<listInstancePath_class, dataPath_> const listInstancePath = "listInstancePath";
x3::rule<initializePath_class, x3::unused_type> const initializePath = "initializePath";
x3::rule<createPathSuggestions_class, x3::unused_type> const createPathSuggestions = "createPathSuggestions";
x3::rule<trailingSlash_class, TrailingSlash> const trailingSlash = "trailingSlash";
x3::rule<absoluteStart_class, Scope> const absoluteStart = "absoluteStart";
x3::rule<keyValue_class, keyValue_> const keyValue = "keyValue";
x3::rule<key_identifier_class, std::string> const key_identifier = "key_identifier";
x3::rule<listSuffix_class, std::vector<keyValue_>> const listSuffix = "listSuffix";
x3::rule<list_class, list_> const list = "list";
x3::rule<createKeySuggestions_class, x3::unused_type> const createKeySuggestions = "createKeySuggestions";
x3::rule<createValueSuggestions_class, x3::unused_type> const createValueSuggestions = "createValueSuggestions";
x3::rule<suggestKeysEnd_class, x3::unused_type> const suggestKeysEnd = "suggestKeysEnd";

template <typename NodeType>
struct NodeParser : x3::parser<NodeParser<NodeType>> {
    using attribute_type = NodeType;
    template <typename It, typename Ctx, typename RCtx, typename Attr>
    bool parse(It& begin, It end, Ctx const& ctx, RCtx& rctx, Attr& attr) const
    {
        std::string tableName;
        if constexpr (std::is_same<NodeType, schemaNode_>()) {
            tableName = "schemaNode";
        } else {
            tableName = "dataNode";
        }
        x3::symbols<NodeType> table(tableName);

        ParserContext& parserContext = x3::get<parser_context_tag>(ctx);
        parserContext.m_suggestions.clear();
        for (const auto& child : parserContext.m_schema.availableNodes(parserContext.currentSchemaPath(), Recursion::NonRecursive)) {
            NodeType out;
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
                    if constexpr (std::is_same<NodeType, schemaNode_>()) {
                        out.m_suffix = list_{child.second};
                    } else {
                        out.m_suffix = listElement_{child.second, {}};
                    }
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
            table.add("..", NodeType{nodeup_{}});
            if (!child.first) {
                auto topLevelModule = parserContext.currentSchemaPath().m_nodes.begin()->m_prefix;
                out.m_prefix = topLevelModule;
                table.add(topLevelModule->m_name + ":" + parseString, out);
            }
        }
        parserContext.m_completionIterator = begin;
        auto res = table.parse(begin, end, ctx, rctx, attr);

        if (attr.m_prefix) {
            parserContext.m_curModule = attr.m_prefix->m_name;
        }

        if (attr.m_suffix.type() == typeid(leaf_)) {
            parserContext.m_tmpListKeyLeafPath.m_location = parserContext.currentSchemaPath();
            ModuleNodePair node{attr.m_prefix.flat_map([](const auto& it) {
                return boost::optional<std::string>{it.m_name};
            }), boost::get<leaf_>(attr.m_suffix).m_name};
            parserContext.m_tmpListKeyLeafPath.m_node = node;
        }

        if constexpr (std::is_same<NodeType, dataNode_>()) {
            if (attr.m_suffix.type() == typeid(listElement_)) {
                parserContext.m_tmpListName = boost::get<listElement_>(attr.m_suffix).m_name;
                res = listSuffix.parse(begin, end, ctx, rctx, boost::get<listElement_>(attr.m_suffix).m_keys);
            }
        }

        if (res) {
            parserContext.pushPathFragment(attr);
            parserContext.m_topLevelModulePresent = true;
        }

        if (attr.m_prefix) {
            parserContext.m_curModule = boost::none;
        }
        return res;
    }
};

NodeParser<schemaNode_> schemaNode;
NodeParser<dataNode_> dataNode;

struct PathParser : x3::parser<PathParser> {
    template <typename It, typename Ctx, typename RCtx, typename Attr>
    bool parse(It& begin, It end, Ctx const& ctx, RCtx& rctx, Attr& attr) const
    {
        initializePath.parse(begin, end, ctx, rctx, attr);

        auto schemaPathGrammar = -absoluteStart >> schemaNode % '/' >> -trailingSlash;
        auto dataPathGrammar = -absoluteStart >> dataNode % '/' >> -trailingSlash;

        bool res;
        if constexpr (std::is_same<Attr, schemaPath_>()) {
            res = schemaPathGrammar.parse(begin, end, ctx, rctx, attr);
        } else {
            res = dataPathGrammar.parse(begin, end, ctx, rctx, attr);
        }

        return res;
    }
} pathParser;

// Need to use these wrappers so that my PathParser class gets the proper
// attribute. Otherwise, Spirit injects the attribute of the outer parser that
// uses my PathParser.
// Example grammar: schemaPath | dataPath.
// The PathParser class would get a boost::variant as the attribute, but I
// don't want to deal with that, so I use these wrappers to ensure the
// attribute I want (and let Spirit deal with boost::variant). Also, the
// attribute gets passed to PathParser::parse via a template argument, so the
// class doesn't even to need to be a template. Convenient!
auto const schemaPath = x3::rule<class schemaPath_class, schemaPath_>{"schemaPath"} = pathParser;
auto const dataPath = x3::rule<class dataPath_class, dataPath_>{"dataPath"} = pathParser;

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

// even though we don't allow no keys to be supplied, the star allows me to check which keys are missing
auto const listSuffix_def =
    *keyValueWrapper;

auto const list_def =
    node_identifier >> !char_('[');

auto const absoluteStart_def =
    x3::omit['/'] >> x3::attr(Scope::Absolute);

auto const trailingSlash_def =
    x3::omit['/'] >> x3::attr(TrailingSlash::Present);

auto const createPathSuggestions_def =
    x3::eps;

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
BOOST_SPIRIT_DEFINE(listSuffix)
BOOST_SPIRIT_DEFINE(list)
BOOST_SPIRIT_DEFINE(dataNodeList)
BOOST_SPIRIT_DEFINE(dataNodesListEnd)
BOOST_SPIRIT_DEFINE(leafPath)
BOOST_SPIRIT_DEFINE(presenceContainerPath)
BOOST_SPIRIT_DEFINE(listInstancePath)
BOOST_SPIRIT_DEFINE(dataPathListEnd)
BOOST_SPIRIT_DEFINE(initializePath)
BOOST_SPIRIT_DEFINE(createKeySuggestions)
BOOST_SPIRIT_DEFINE(createPathSuggestions)
BOOST_SPIRIT_DEFINE(createValueSuggestions)
BOOST_SPIRIT_DEFINE(suggestKeysEnd)
BOOST_SPIRIT_DEFINE(absoluteStart)
BOOST_SPIRIT_DEFINE(trailingSlash)
