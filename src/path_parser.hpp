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

x3::rule<cdPath_class, dataPath_> const cdPath = "cdPath";
x3::rule<getPath_class, decltype(get_::m_path)> const getPath = "getPath";
x3::rule<rpcPath_class, dataPath_> const rpcPath = "rpcPath";
x3::rule<presenceContainerPath_class, dataPath_> const presenceContainerPath = "presenceContainerPath";
x3::rule<listInstancePath_class, dataPath_> const listInstancePath = "listInstancePath";
x3::rule<leafListElementPath_class, dataPath_> const leafListElementPath = "leafListElementPath";
x3::rule<initializePath_class, x3::unused_type> const initializePath = "initializePath";
x3::rule<trailingSlash_class, TrailingSlash> const trailingSlash = "trailingSlash";
x3::rule<absoluteStart_class, Scope> const absoluteStart = "absoluteStart";
x3::rule<keyValue_class, keyValue_> const keyValue = "keyValue";
x3::rule<key_identifier_class, std::string> const key_identifier = "key_identifier";
x3::rule<listSuffix_class, std::vector<keyValue_>> const listSuffix = "listSuffix";
x3::rule<createKeySuggestions_class, x3::unused_type> const createKeySuggestions = "createKeySuggestions";
x3::rule<createValueSuggestions_class, x3::unused_type> const createValueSuggestions = "createValueSuggestions";
x3::rule<suggestKeysEnd_class, x3::unused_type> const suggestKeysEnd = "suggestKeysEnd";
x3::rule<class leafListValue_class, leaf_data_> const leafListValue = "leafListValue";

enum class NodeParserMode {
    CompleteDataNode,
    IncompleteDataNode,
    CompletionsOnly,
    SchemaNode
};

template <auto>
struct ModeToAttribute;
template <>
struct ModeToAttribute<NodeParserMode::CompleteDataNode> {
    using type = dataNode_;
};
template <>
struct ModeToAttribute<NodeParserMode::IncompleteDataNode> {
    using type = dataNode_;
};
template <>
struct ModeToAttribute<NodeParserMode::SchemaNode> {
    using type = schemaNode_;
};
// The CompletionsOnly attribute is dataNode_ only because of convenience:
// having the same return type means we can get by without a ton of `if constexpr` stanzas.
// So the code will still "parse data into the target attr" for simplicity.
template <>
struct ModeToAttribute<NodeParserMode::CompletionsOnly> {
    using type = dataNode_;
};

enum class CompletionMode {
    Schema,
    Data
};

template <NodeParserMode PARSER_MODE, CompletionMode COMPLETION_MODE>
struct NodeParser : x3::parser<NodeParser<PARSER_MODE, COMPLETION_MODE>> {
    using attribute_type = typename ModeToAttribute<PARSER_MODE>::type;

    std::function<bool(const Schema&, const std::string& path)> m_filterFunction;

    NodeParser(const std::function<bool(const Schema&, const std::string& path)>& filterFunction)
        : m_filterFunction(filterFunction)
    {
    }

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
            attribute_type out;
            std::string parseString;
            if (child.first) {
                out.m_prefix = module_{*child.first};
                parseString = *child.first + ":";
            }
            parseString += child.second;

            if (!m_filterFunction(parserContext.m_schema, joinPaths(pathToSchemaString(parserContext.currentSchemaPath(), Prefixes::Always), parseString))) {
                continue;
            }

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

                    if constexpr (COMPLETION_MODE == CompletionMode::Schema) {
                        parserContext.m_suggestions.emplace(Completion{parseString + "/"});
                    } else {
                        parserContext.m_suggestions.emplace(Completion{parseString, "[", Completion::WhenToAdd::IfFullMatch});
                    }
                    break;
                case yang::NodeTypes::LeafList:
                    if constexpr (std::is_same<attribute_type, schemaNode_>()) {
                        out.m_suffix = leafList_{child.second};
                    } else {
                        out.m_suffix = leafListElement_{child.second, {}};
                    }

                    if constexpr (COMPLETION_MODE == CompletionMode::Schema) {
                        parserContext.m_suggestions.emplace(Completion{parseString + "/"});
                    } else {
                        parserContext.m_suggestions.emplace(Completion{parseString, "[", Completion::WhenToAdd::IfFullMatch});
                    }
                    break;
                case yang::NodeTypes::Rpc:
                    out.m_suffix = rpcNode_{child.second};
                    parserContext.m_suggestions.emplace(Completion{parseString + "/"});
                    break;
                case yang::NodeTypes::Action:
                    out.m_suffix = actionNode_{child.second};
                    parserContext.m_suggestions.emplace(Completion{parseString + "/"});
                    break;
                case yang::NodeTypes::AnyXml:
                case yang::NodeTypes::Notification:
                    continue;
            }
            table.add(parseString, out);
            if (!child.first) {
                auto topLevelModule = parserContext.currentSchemaPath().m_nodes.begin()->m_prefix;
                out.m_prefix = topLevelModule;
                table.add(topLevelModule->m_name + ":" + parseString, out);
            }
        }
        table.add("..", attribute_type{nodeup_{}});
        parserContext.m_completionIterator = begin;

        if constexpr (PARSER_MODE == NodeParserMode::CompletionsOnly) {
            return true;
        } else {
            It saveIter;
            // GCC complains that I assign saveIter because I use it only if NodeType is dataNode_
            // FIXME: GCC 10.1 doesn't emit a warning here. Make this unconditional when GCC 10 is available.
            if constexpr (std::is_same<attribute_type, dataNode_>()) {
                saveIter = begin;
            }

            auto res = table.parse(begin, end, ctx, rctx, attr);

            if (std::holds_alternative<leaf_>(attr.m_suffix)) {
                parserContext.m_tmpListKeyLeafPath.m_location = parserContext.currentSchemaPath();
                ModuleNodePair node{attr.m_prefix.flat_map([](const auto& it) {
                                        return boost::optional<std::string>{it.m_name};
                                    }),
                                    std::get<leaf_>(attr.m_suffix).m_name};
                parserContext.m_tmpListKeyLeafPath.m_node = node;
            }

            if constexpr (std::is_same<attribute_type, dataNode_>()) {
                if (std::holds_alternative<listElement_>(attr.m_suffix)) {
                    parserContext.m_tmpListPath = parserContext.currentDataPath();
                    auto tmpList = list_{std::get<listElement_>(attr.m_suffix).m_name};
                    parserContext.m_tmpListPath.m_nodes.emplace_back(attr.m_prefix, tmpList);

                    res = listSuffix.parse(begin, end, ctx, rctx, std::get<listElement_>(attr.m_suffix).m_keys);

                    // FIXME: think of a better way to do this, that is, get rid of manual iterator reverting
                    if (!res) {
                        // If listSuffix didn't succeed, we check, if we allow incomplete nodes. If we do, then we replace listElement_ with list_.
                        // If we don't, we fail the whole symbol table.
                        if constexpr (PARSER_MODE == NodeParserMode::IncompleteDataNode) {
                            res = true;
                            attr.m_suffix = list_{std::get<listElement_>(attr.m_suffix).m_name};
                        } else {
                            begin = saveIter;
                        }
                    }
                }

                if (std::holds_alternative<leafListElement_>(attr.m_suffix)) {
                    parserContext.m_tmpListKeyLeafPath.m_location = parserContext.currentSchemaPath();
                    ModuleNodePair node{attr.m_prefix.flat_map([](const auto& it) {
                                            return boost::optional<std::string>{it.m_name};
                                        }),
                                        std::get<leafListElement_>(attr.m_suffix).m_name};
                    parserContext.m_tmpListKeyLeafPath.m_node = node;
                    res = leafListValue.parse(begin, end, ctx, rctx, std::get<leafListElement_>(attr.m_suffix).m_value);

                    if (!res) {
                        if constexpr (PARSER_MODE == NodeParserMode::IncompleteDataNode) {
                            res = true;
                            attr.m_suffix = leafList_{std::get<leafListElement_>(attr.m_suffix).m_name};
                        } else {
                            begin = saveIter;
                        }
                    }
                }
            }

            if (res) {
                parserContext.pushPathFragment(attr);
            }

            return res;
        }
    }
};

template <CompletionMode COMPLETION_MODE> using schemaNode = NodeParser<NodeParserMode::SchemaNode, COMPLETION_MODE>;
template <CompletionMode COMPLETION_MODE> using dataNode = NodeParser<NodeParserMode::CompleteDataNode, COMPLETION_MODE>;
template <CompletionMode COMPLETION_MODE> using incompleteDataNode = NodeParser<NodeParserMode::IncompleteDataNode, COMPLETION_MODE>;
template <CompletionMode COMPLETION_MODE> using pathCompletions = NodeParser<NodeParserMode::CompletionsOnly, COMPLETION_MODE>;

using AnyPath = boost::variant<schemaPath_, dataPath_>;

enum class PathParserMode {
    AnyPath,
    DataPath,
    DataPathListEnd
};

template <>
struct ModeToAttribute<PathParserMode::AnyPath> {
    using type = AnyPath;
};

template <>
struct ModeToAttribute<PathParserMode::DataPath> {
    using type = dataPath_;
};

template <>
struct ModeToAttribute<PathParserMode::DataPathListEnd> {
    using type = dataPath_;
};

template <PathParserMode PARSER_MODE, CompletionMode COMPLETION_MODE>
struct PathParser : x3::parser<PathParser<PARSER_MODE, COMPLETION_MODE>> {
    using attribute_type = ModeToAttribute<PARSER_MODE>;
    std::function<bool(const Schema&, const std::string& path)> m_filterFunction;

    PathParser(const std::function<bool(const Schema&, const std::string& path)>& filterFunction = [] (const auto&, const auto&) {return true;})
        : m_filterFunction(filterFunction)
    {
    }

    template <typename It, typename Ctx, typename RCtx, typename Attr>
    bool parse(It& begin, It end, Ctx const& ctx, RCtx& rctx, Attr& attr) const
    {
        initializePath.parse(begin, end, ctx, rctx, x3::unused);
        dataPath_ attrData;

        auto pathEnd = x3::rule<class PathEnd>{"pathEnd"} = &space_separator | x3::eoi;
        // absoluteStart has to be separate from the dataPath parser,
        // otherwise, if the "dataNode % '/'" parser fails, the begin iterator
        // gets reverted to before the starting slash.
        auto res = (-absoluteStart).parse(begin, end, ctx, rctx, attrData.m_scope);
        auto dataPath = x3::attr(attrData.m_scope)
            >> (dataNode<COMPLETION_MODE>{m_filterFunction} % '/' | pathEnd >> x3::attr(std::vector<dataNode_>{}))
            >> -trailingSlash;
        res = dataPath.parse(begin, end, ctx, rctx, attrData);

        // If we allow data paths with a list at the end, we just try to parse that separately.
        if constexpr (PARSER_MODE == PathParserMode::DataPathListEnd || PARSER_MODE == PathParserMode::AnyPath) {
            if (!res || !pathEnd.parse(begin, end, ctx, rctx, x3::unused)) {
                dataNode_ attrNodeList;
                res = incompleteDataNode<COMPLETION_MODE>{m_filterFunction}.parse(begin, end, ctx, rctx, attrNodeList);
                if (res) {
                    attrData.m_nodes.emplace_back(attrNodeList);
                    // If the trailing slash matches, no more nodes are parsed.
                    // That means no more completion. So, I generate them
                    // manually.
                    res = (-(trailingSlash >> x3::omit[pathCompletions<COMPLETION_MODE>{m_filterFunction}])).parse(begin, end, ctx, rctx, attrData.m_trailingSlash);
                }
            }
        }

        attr = attrData;
        if constexpr (PARSER_MODE == PathParserMode::AnyPath) {
            // If our data path already has some listElement_ fragments, we can't parse rest of the path as a schema path
            auto hasLists = std::any_of(attrData.m_nodes.begin(), attrData.m_nodes.end(),
                [] (const auto& node) { return std::holds_alternative<listElement_>(node.m_suffix); });
            // If parsing failed, or if there's more input we try parsing schema nodes.
            if (!hasLists) {
                if (!res || !pathEnd.parse(begin, end, ctx, rctx, x3::unused)) {
                    // If dataPath parsed some nodes, they will be saved in `attrData`. We have to keep these.
                    schemaPath_ attrSchema = dataPathToSchemaPath(attrData);
                    auto schemaPath = schemaNode<COMPLETION_MODE>{m_filterFunction} % '/';
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
auto const anyPath = x3::rule<class anyPath_class, AnyPath>{"anyPath"} = PathParser<PathParserMode::AnyPath, CompletionMode::Schema>{};
auto const dataPath = x3::rule<class dataPath_class, dataPath_>{"dataPath"} = PathParser<PathParserMode::DataPath, CompletionMode::Data>{};
auto const dataPathListEnd = x3::rule<class dataPath_class, dataPath_>{"dataPath"} = PathParser<PathParserMode::DataPathListEnd, CompletionMode::Data>{};

#if __clang__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Woverloaded-shift-op-parentheses"
#endif

struct SuggestLeafListEnd : x3::parser<SuggestLeafListEnd> {
    using attribute_type = x3::unused_type;
    template <typename It, typename Ctx, typename RCtx, typename Attr>
    bool parse(It& begin, It, Ctx const& ctx, RCtx&, Attr&) const
    {
        auto& parserContext = x3::get<parser_context_tag>(ctx);
        parserContext.m_completionIterator = begin;
        parserContext.m_suggestions = {Completion{"]"}};

        return true;
    }
} const suggestLeafListEnd;

auto const leafListValue_def =
    '[' >> leaf_data >> suggestLeafListEnd >> ']';

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
    x3::no_skip['[' > createKeySuggestions > keyValue > suggestKeysEnd > ']'];

// even though we don't allow no keys to be supplied, the star allows me to check which keys are missing
auto const listSuffix_def =
    *keyValueWrapper;

auto const list_def =
    node_identifier >> !char_('[');

auto const absoluteStart_def =
    x3::omit['/'] >> x3::attr(Scope::Absolute);

auto const trailingSlash_def =
    x3::omit['/'] >> x3::attr(TrailingSlash::Present);

auto const filterConfigFalse = [] (const Schema& schema, const std::string& path) {
    return schema.isConfig(path);
};

// If added into the spirit context via blahblah::add<writableOps_tag>(), disregard `config: false` statements. This is
// used by the yang-cli because that tool needs modeling of the full datastore, including the "read-only" data.
struct writableOps_tag;

PathParser<PathParserMode::DataPath, CompletionMode::Data> const dataPathFilterConfigFalse{filterConfigFalse};

struct WritableLeafPath : x3::parser<WritableLeafPath> {
    using attribute_type = dataPath_;
    template <typename It, typename Ctx, typename RCtx, typename Attr>
    static bool parse(It& begin, It end, Ctx const& ctx, RCtx& rctx, Attr& attr)
    {
        bool res;
        if (x3::get<writableOps_tag>(ctx) == WritableOps::Yes) {
            res = dataPath.parse(begin, end, ctx, rctx, attr);
        } else {
            res = dataPathFilterConfigFalse.parse(begin, end, ctx, rctx, attr);
        }
        if (!res) {
            return false;
        }

        if (attr.m_nodes.empty() || !std::holds_alternative<leaf_>(attr.m_nodes.back().m_suffix)) {
            auto& parserContext = x3::get<parser_context_tag>(ctx);
            parserContext.m_errorMsg = "This is not a path to leaf.";
            return false;
        }

        return true;
    }

} writableLeafPath;

struct RpcActionPath : x3::parser<RpcActionPath> {
    using attribute_type = dataPath_;
    template <typename It, typename Ctx, typename RCtx, typename Attr>
    static bool parse(It& begin, It end, Ctx const& ctx, RCtx& rctx, Attr& attr)
    {
        bool res = dataPath.parse(begin, end, ctx, rctx, attr);
        if (!res) {
            return false;
        }

        if (attr.m_nodes.empty()
                || (!std::holds_alternative<rpcNode_>(attr.m_nodes.back().m_suffix) && !std::holds_alternative<actionNode_>(attr.m_nodes.back().m_suffix))) {
            auto& parserContext = x3::get<parser_context_tag>(ctx);
            parserContext.m_errorMsg = "This is not a path to an RPC/action.";
            return false;
        }

        return true;
    }
};

auto const rpcActionPath = as<dataPath_>[RpcActionPath()];

auto const noRpcOrAction = [] (const Schema& schema, const std::string& path) {
    auto nodeType = schema.nodeType(path);
    return nodeType != yang::NodeTypes::Rpc && nodeType != yang::NodeTypes::Action;
};

auto const getPath_def =
    PathParser<PathParserMode::DataPathListEnd, CompletionMode::Data>{noRpcOrAction} |
    PathParser<PathParserMode::DataPath, CompletionMode::Data>{noRpcOrAction} |
    (module >> "*");

auto const cdPath_def =
    PathParser<PathParserMode::DataPath, CompletionMode::Data>{noRpcOrAction};

auto const presenceContainerPath_def =
    dataPath;

auto const listInstancePath_def =
    dataPath;

auto const leafListElementPath_def =
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
BOOST_SPIRIT_DEFINE(cdPath)
BOOST_SPIRIT_DEFINE(getPath)
BOOST_SPIRIT_DEFINE(presenceContainerPath)
BOOST_SPIRIT_DEFINE(listInstancePath)
BOOST_SPIRIT_DEFINE(leafListElementPath)
BOOST_SPIRIT_DEFINE(initializePath)
BOOST_SPIRIT_DEFINE(createKeySuggestions)
BOOST_SPIRIT_DEFINE(createValueSuggestions)
BOOST_SPIRIT_DEFINE(suggestKeysEnd)
BOOST_SPIRIT_DEFINE(leafListValue)
BOOST_SPIRIT_DEFINE(absoluteStart)
BOOST_SPIRIT_DEFINE(trailingSlash)
