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

auto pathEnd = x3::rule<struct PathEnd>{"pathEnd"} = &space_separator | x3::eoi;

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

auto const createKeySuggestions = x3::rule<createKeySuggestions_class, x3::unused_type>{"createKeySuggestions"} =
    x3::eps;

auto const key_identifier = x3::rule<key_identifier_class, std::string>{"key_identifier"} =
    ((x3::alpha | char_("_")) >> *(x3::alnum | char_("_") | char_("-") | char_(".")));

auto const createValueSuggestions = x3::rule<createValueSuggestions_class, x3::unused_type>{"createValueSuggestions"} =
    x3::eps;

auto const keyValue = x3::rule<keyValue_class, keyValue_>{"keyValue"} =
    key_identifier > '=' > createValueSuggestions > leaf_data;

auto const suggestKeysEnd = x3::rule<suggestKeysEnd_class, x3::unused_type>{"suggestKeysEnd"} =
    x3::eps;

auto const keyValueWrapper =
    '[' > createKeySuggestions > keyValue > suggestKeysEnd > ']';

// even though we don't allow no keys to be supplied, the star allows me to check which keys are missing
auto const listSuffix = x3::rule<listSuffix_class, std::vector<keyValue_>>{"listSuffix"} =
    *keyValueWrapper;

auto const suggestLeafListEnd = staticSuggestions({"]"});

auto const leafListValue = x3::rule<struct leafListValue_class, leaf_data_>{"leafListValue"} =
    '[' >> leaf_data >> suggestLeafListEnd >> ']';

template <NodeParserMode PARSER_MODE, CompletionMode COMPLETION_MODE>
struct NodeParser : x3::parser<NodeParser<PARSER_MODE, COMPLETION_MODE>> {
    using attribute_type = typename ModeToAttribute<PARSER_MODE>::type;

    std::function<bool(const Schema&, const std::string& path)> m_filterFunction;

    NodeParser(const std::function<bool(const Schema&, const std::string& path)>& filterFunction)
        : m_filterFunction(filterFunction)
    {
    }

    template <typename It, typename Ctx, typename RCtx, typename Attr>
    bool parse(It& begin, It end, Ctx const& ctx, RCtx& rctx, Attr& attr) const
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
            It saveIter = begin;

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
                // After a path fragment, there can only be a slash or a "pathEnd". If this is not the case
                // then that means there are other unparsed characters after the fragment. In that case the parsing
                // needs to fail.
                res = (pathEnd | &char_('/')).parse(begin, end, ctx, rctx, x3::unused);
                if (!res) {
                    begin = saveIter;
                } else {
                    parserContext.pushPathFragment(attr);
                }
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
    DataPathListEnd,
    DataPathAbsolute
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

template <>
struct ModeToAttribute<PathParserMode::DataPathAbsolute> {
    using type = dataPath_;
};

auto const trailingSlash = x3::rule<struct trailingSlash_class, x3::unused_type>{"trailingSlash"} =
    x3::omit['/'];

// A "nothing" parser, which is used to indicate we tried to parse a path
auto const initializePath = x3::rule<initializePath_class, x3::unused_type>{"initializePath"} =
    x3::eps;

auto const absoluteStart = x3::rule<absoluteStart_class, Scope>{"absoluteStart"} =
    x3::omit['/'] >> x3::attr(Scope::Absolute);

template <PathParserMode PARSER_MODE, CompletionMode COMPLETION_MODE>
struct PathParser : x3::parser<PathParser<PARSER_MODE, COMPLETION_MODE>> {
    using attribute_type = typename ModeToAttribute<PARSER_MODE>::type;
    std::function<bool(const Schema&, const std::string& path)> m_filterFunction;

    PathParser(const std::function<bool(const Schema&, const std::string& path)>& filterFunction = [](const auto&, const auto&) { return true; })
        : m_filterFunction(filterFunction)
    {
    }

    template <typename It, typename Ctx, typename RCtx, typename Attr>
    bool parse(It& begin, It end, Ctx const& ctx, RCtx& rctx, Attr& attr) const
    {
        initializePath.parse(begin, end, ctx, rctx, x3::unused);
        dataPath_ attrData;

        auto res = [](){
            if constexpr (PARSER_MODE == PathParserMode::DataPathAbsolute) {
                return absoluteStart;
            } else {
                return -absoluteStart;
            }
        }
        // absoluteStart has to be separate from the dataPath parser,
        // otherwise, if the "dataNode % '/'" parser fails, the begin iterator
        // gets reverted to before the starting slash.
        ().parse(begin, end, ctx, rctx, attrData.m_scope);
        if (!res) {
            x3::get<parser_context_tag>(ctx).m_suggestions.emplace(Completion{"/"});
            return res;
        }

        auto dataPath = x3::attr(attrData.m_scope)
            >> (dataNode<COMPLETION_MODE>{m_filterFunction} % '/' | pathEnd >> x3::attr(std::vector<dataNode_>{}))
            >> -trailingSlash;
        res = dataPath.parse(begin, end, ctx, rctx, attrData);

        // If we allow data paths with a list at the end, we just try to parse that separately.
        if constexpr (PARSER_MODE == PathParserMode::DataPathListEnd || PARSER_MODE == PathParserMode::AnyPath) {
            if (!res || !pathEnd.parse(begin, end, ctx, rctx, x3::unused)) {
                dataNode_ attrNodeList;
                auto hasListEnd = incompleteDataNode<COMPLETION_MODE>{m_filterFunction}.parse(begin, end, ctx, rctx, attrNodeList);
                if (hasListEnd) {
                    attrData.m_nodes.emplace_back(attrNodeList);
                    // If the trailing slash matches, no more nodes are parsed. That means no more completion. So, I
                    // generate them manually, but only if we're in AnyPath mode, so, for example, inside an `ls`
                    // command. If we're in DataPathListEnd it doesn't make sense to parse put any more nodes after the
                    // final list.
                    if constexpr (PARSER_MODE == PathParserMode::AnyPath) {
                        res = (-(trailingSlash >> x3::omit[pathCompletions<COMPLETION_MODE>{m_filterFunction}])).parse(begin, end, ctx, rctx, x3::unused);
                    } else {
                        res = (-trailingSlash).parse(begin, end, ctx, rctx, x3::unused);
                    }
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
                    res = trailing.parse(begin, end, ctx, rctx, x3::unused);
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
auto const anyPath = x3::rule<struct anyPath_class, AnyPath>{"anyPath"} = PathParser<PathParserMode::AnyPath, CompletionMode::Schema>{};
auto const dataPath = x3::rule<struct dataPath_class, dataPath_>{"dataPath"} = PathParser<PathParserMode::DataPath, CompletionMode::Data>{};
auto const dataPathListEnd = x3::rule<struct dataPath_class, dataPath_>{"dataPath"} = PathParser<PathParserMode::DataPathListEnd, CompletionMode::Data>{};
auto const dataPathAbsolute = x3::rule<struct dataPath_class, dataPath_>{"dataPathAbsolute"} = PathParser<PathParserMode::DataPathAbsolute, CompletionMode::Data>{};

#if __clang__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Woverloaded-shift-op-parentheses"
#endif

auto const filterConfigFalse = [](const Schema& schema, const std::string& path) {
    return schema.isConfig(path);
};

// A WritableOps value is injected through the `x3::with` with this tag (see usage of the tag). It controls whether
// `config: false` data can be set with the `set` command. This is used by yang-cli because that tool needs modeling of
// the full datastore, including the "read-only" data.
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

} const writableLeafPath;

enum class AllowInput {
    Yes,
    No
};

template <AllowInput ALLOW_INPUT>
struct RpcActionPath : x3::parser<RpcActionPath<ALLOW_INPUT>> {
    using attribute_type = dataPath_;
    template <typename It, typename Ctx, typename RCtx, typename Attr>
    static bool parse(It& begin, It end, Ctx const& ctx, RCtx& rctx, Attr& attr)
    {
        auto grammar = PathParser<PathParserMode::DataPath, CompletionMode::Data>{[] (const Schema& schema, const std::string& path) {
            if constexpr (ALLOW_INPUT == AllowInput::No) {
                auto nodeType = schema.nodeType(path);
                if (nodeType == yang::NodeTypes::Rpc || nodeType == yang::NodeTypes::Action) {
                    return !schema.hasInputNodes(path);
                }
            }

            return true;
        }};
        bool res = grammar.parse(begin, end, ctx, rctx, attr);
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

auto const noRpcOrAction = [](const Schema& schema, const std::string& path) {
    auto nodeType = schema.nodeType(path);
    return nodeType != yang::NodeTypes::Rpc && nodeType != yang::NodeTypes::Action;
};

auto const getPath = x3::rule<struct getPath_class, decltype(get_::m_path)::value_type>{"getPath"} =
    PathParser<PathParserMode::DataPathListEnd, CompletionMode::Data>{noRpcOrAction} |
    (module >> "*");

auto const cdPath = x3::rule<struct cdPath_class, dataPath_>{"cdPath"} =
    PathParser<PathParserMode::DataPath, CompletionMode::Data>{[] (const Schema& schema, const std::string& path) {
        return noRpcOrAction(schema, path) && schema.nodeType(path) != yang::NodeTypes::Leaf;
    }};

auto const presenceContainerPath = x3::rule<presenceContainerPath_class, dataPath_>{"presenceContainerPath"} =
    dataPath;

auto const listInstancePath = x3::rule<listInstancePath_class, dataPath_>{"listInstancePath"} =
    dataPath;

auto const leafListElementPath = x3::rule<leafListElementPath_class, dataPath_>{"leafListElementPath"} =
    dataPath;

template <typename It, typename Ctx, typename RCtx, typename Attr>
bool leaf_data_parse_data_path(It& first, It last, Ctx const& ctx, RCtx& rctx, Attr& attr)
{
    dataPath_ path;
    auto res = dataPathAbsolute.parse(first, last, ctx, rctx, path);
    if (res) {
        // FIXME: list key escaping in XPath is different from netconf-cli.
        // If the key(s) are not all strings, this will produce netconf-cli-style
        // string like /foo:xxx[k1=blah][k2=666] instead of standard XPaths like
        // /foo:xxx[k1='blah'][k2='666'], and these will cause troubles when passed
        // to libyang later on.
        attr = instanceIdentifier_{pathToDataString(path, Prefixes::WhenNeeded)};
    }
    return res;
}

#if __clang__
#pragma GCC diagnostic pop
#endif
