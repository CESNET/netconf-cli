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

x3::rule<leaf_path_class, dataPath_> const leafPath = "leafPath";
x3::rule<presenceContainerPath_class, dataPath_> const presenceContainerPath = "presenceContainerPath";
x3::rule<listInstancePath_class, dataPath_> const listInstancePath = "listInstancePath";
x3::rule<initializePath_class, x3::unused_type> const initializePath = "initializePath";
x3::rule<trailingSlash_class, TrailingSlash> const trailingSlash = "trailingSlash";
x3::rule<absoluteStart_class, Scope> const absoluteStart = "absoluteStart";
x3::rule<keyValue_class, keyValue_> const keyValue = "keyValue";
x3::rule<key_identifier_class, std::string> const key_identifier = "key_identifier";
x3::rule<listSuffix_class, std::vector<keyValue_>> const listSuffix = "listSuffix";
x3::rule<createKeySuggestions_class, x3::unused_type> const createKeySuggestions = "createKeySuggestions";
x3::rule<createValueSuggestions_class, x3::unused_type> const createValueSuggestions = "createValueSuggestions";
x3::rule<suggestKeysEnd_class, x3::unused_type> const suggestKeysEnd = "suggestKeysEnd";

enum class AllowListDataNode {
    Allow,
    Disallow
};

enum class NodeParserMode {
    CompleteDataNode,
    IncompleteDataNode,
    CompletionsOnly,
    SchemaNode
};

template <NodeParserMode> struct ModeToAttribute;
template <> struct ModeToAttribute<NodeParserMode::CompleteDataNode> {using type = dataNode_;};
template <> struct ModeToAttribute<NodeParserMode::IncompleteDataNode> {using type = dataNode_;};
template <> struct ModeToAttribute<NodeParserMode::SchemaNode> {using type = schemaNode_;};
// The CompletionsOnly attribute is dataNode_ only because of convenience:
// otherwise I would have to if constexpr all the stuff that works with the
// symbol table.
template <> struct ModeToAttribute<NodeParserMode::CompletionsOnly> {using type = dataNode_;};

template <NodeParserMode PARSER_MODE>
struct NodeParser : x3::parser<NodeParser<PARSER_MODE>> {

    using attribute_type = typename ModeToAttribute<PARSER_MODE>::type;
    // GCC complains that `end` isn't used when doing completions only
    // FIXME: GCC 10.1 doesn't emit a warning here. Remove [[maybe_unused]] when GCC 10 is available
    template <typename It, typename Ctx, typename RCtx, typename Attr>
    bool parse(It& begin, [[maybe_unused]] It end, Ctx const& ctx, RCtx& rctx, Attr& attr) const
    {
        std::string tableName;
        if constexpr (std::is_same<attribute_type, schemaNode_>()) {
            tableName = "schemaNode";
        } else {
            tableName = "dataNode";
        }
        x3::symbols<attribute_type> table(tableName);

        ParserContext& parserContext = x3::get<parser_context_tag>(ctx);
        parserContext.m_suggestions.clear();
        for (const auto& child : parserContext.m_schema.availableNodes(parserContext.currentSchemaPath(), Recursion::NonRecursive)) {
            // GCC complains that `out` isn't used when doing completions only
            // FIXME: GCC 10.1 doesn't emit a warning here. Remove [[maybe_unused]] when GCC 10 is available
            [[maybe_unused]] attribute_type out;
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
                    if constexpr (std::is_same<attribute_type, schemaNode_>()) {
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
            table.add("..", attribute_type{nodeup_{}});
            if (!child.first) {
                auto topLevelModule = parserContext.currentSchemaPath().m_nodes.begin()->m_prefix;
                out.m_prefix = topLevelModule;
                table.add(topLevelModule->m_name + ":" + parseString, out);
            }
        }
        parserContext.m_completionIterator = begin;

        if constexpr (PARSER_MODE == NodeParserMode::CompletionsOnly) {
            return true;
        } else {
            It saveIter;
            // GCC complains that I assign saveIter because I use it only if NodeType is dataNode_
            // FIXME: GCC 10.1 doesn't emit a warning here. Remove this if constexpr block when GCC 10 is available
            if constexpr (std::is_same<attribute_type, dataNode_>()) {
                saveIter = begin;
            }

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

            if constexpr (std::is_same<attribute_type, dataNode_>()) {
                if (attr.m_suffix.type() == typeid(listElement_)) {
                    parserContext.m_tmpListName = boost::get<listElement_>(attr.m_suffix).m_name;
                    res = listSuffix.parse(begin, end, ctx, rctx, boost::get<listElement_>(attr.m_suffix).m_keys);

                    // FIXME: think of a better way to do this, that is, get rid of manual iterator reverting
                    if (res) {
                        // If listSuffix succeeded, we must disable
                        // backtracking to other types of paths, because
                        // the current path can't be a schema path anymore.
                        parserContext.m_pathBacktracking = Backtracking::Disabled;
                    } else {
                        // If it didn't succeed, we check, if we allow incomplete nodes. If we do, then we replace listElement_ with list_.
                        // If we don't, we fail the whole symbol table.
                        if constexpr (PARSER_MODE == NodeParserMode::IncompleteDataNode) {
                            res = true;
                            attr.m_suffix = list_{boost::get<listElement_>(attr.m_suffix).m_name};
                        } else {
                            begin = saveIter;
                        }
                    }
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
    }
};

NodeParser<NodeParserMode::SchemaNode> schemaNode;
NodeParser<NodeParserMode::CompleteDataNode> dataNode;
NodeParser<NodeParserMode::IncompleteDataNode> dataNodeAllowList;
NodeParser<NodeParserMode::CompletionsOnly> pathCompletions;

using AnyPath = boost::variant<schemaPath_, dataPath_>;

template <typename PathType, AllowListDataNode ALLOW_LIST_END>
struct PathParser : x3::parser<PathParser<PathType, ALLOW_LIST_END>> {
    using attribute_type = PathType;
    template <typename It, typename Ctx, typename RCtx, typename Attr>
    bool parse(It& begin, It end, Ctx const& ctx, RCtx& rctx, Attr& attr) const
    {
        initializePath.parse(begin, end, ctx, rctx, x3::unused);
        ParserContext& parserContext = x3::get<parser_context_tag>(ctx);
        dataPath_ attrData;

        auto pathEnd = x3::rule<class PathEnd>{"pathEnd"} = &space_separator | x3::eoi;
        // absoluteStart has to be separate from the dataPath parser,
        // otherwise, if the "dataNode % '/'" parser fails, the begin iterator
        // gets reverted to before the starting slash.
        auto res = (-absoluteStart).parse(begin, end, ctx, rctx, attrData.m_scope);
        auto dataPath = x3::attr(attrData.m_scope)
            >> (dataNode % '/' | pathEnd >> x3::attr(std::vector<dataNode_>{}))
            >> -trailingSlash;
        res = dataPath.parse(begin, end, ctx, rctx, attrData);

        // If we allow data paths with a list at the end, we just try to parse that separately.
        if constexpr (ALLOW_LIST_END == AllowListDataNode::Allow) {
            if (!res || !pathEnd.parse(begin, end, ctx, rctx, x3::unused)) {
                dataNode_ attrNodeList;
                res = dataNodeAllowList.parse(begin, end, ctx, rctx, attrNodeList);
                if (res) {
                    attrData.m_nodes.push_back(attrNodeList);
                    // If the trailing slash matches, no more nodes are parsed.
                    // That means no more completion. So, I generate them
                    // manually.
                    res = (-(trailingSlash >> x3::omit[pathCompletions])).parse(begin, end, ctx, rctx, attrData.m_trailingSlash);
                }
            }
        }

        attr = attrData;
        if (parserContext.m_pathBacktracking == Backtracking::Enabled) {
            if constexpr (std::is_same<Attr, AnyPath>()) {
                // If parsing failed, or if there's more input we try parsing schema nodes.
                if (!res || !pathEnd.parse(begin, end, ctx, rctx, x3::unused)) {
                    // If dataPath parsed some nodes, they will be saved in `attrData`. We have to keep these.
                    schemaPath_ attrSchema = dataPathToSchemaPath(attrData);
                    auto schemaPath = schemaNode % '/';
                    // The schemaPath parser continues where the dataPath parser ended.
                    res = schemaPath.parse(begin, end, ctx, rctx, attrSchema.m_nodes);
                    auto trailing = -trailingSlash >> pathEnd;
                    res = trailing.parse(begin, end, ctx, rctx, attrSchema.m_trailingSlash);
                    attr = attrSchema;
                }
            }
        }
        return res;
    }
};

// Need to use these wrappers so that my PathParser class gets the proper
// attribute. Otherwise, Spirit injects the attribute of the outer parser that
// uses my PathParser.
// Example grammar: anyPath | module.
// The PathParser class would get a boost::variant as the attribute, but I
// don't want to deal with that, so I use these wrappers to ensure the
// attribute I want (and let Spirit deal with boost::variant).
auto const anyPath = x3::rule<class anyPath_class, AnyPath>{"anyPath"} = PathParser<AnyPath, AllowListDataNode::Allow>{};
auto const dataPath = x3::rule<class dataPath_class, dataPath_>{"dataPath"} = PathParser<dataPath_, AllowListDataNode::Disallow>{};
auto const dataPathListEnd = x3::rule<class dataPath_class, dataPath_>{"dataPath"} = PathParser<dataPath_, AllowListDataNode::Allow>{};

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
BOOST_SPIRIT_DEFINE(leafPath)
BOOST_SPIRIT_DEFINE(presenceContainerPath)
BOOST_SPIRIT_DEFINE(listInstancePath)
BOOST_SPIRIT_DEFINE(initializePath)
BOOST_SPIRIT_DEFINE(createKeySuggestions)
BOOST_SPIRIT_DEFINE(createValueSuggestions)
BOOST_SPIRIT_DEFINE(suggestKeysEnd)
BOOST_SPIRIT_DEFINE(absoluteStart)
BOOST_SPIRIT_DEFINE(trailingSlash)
