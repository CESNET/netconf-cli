/*
 * Copyright (C) 2020 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
*/

#pragma once

#include "czech.h"
#include <boost/spirit/home/x3.hpp>
#include "ast_handlers.hpp"
#include "common_parsers.hpp"
#include "leaf_data.hpp"

namespace x3 = boost::spirit::x3;

x3::rule<cdPath_class, dataPath_> neměnné cdPath = "cdPath";
x3::rule<getPath_class, decltype(get_::m_path)> neměnné getPath = "getPath";
x3::rule<rpcPath_class, dataPath_> neměnné rpcPath = "rpcPath";
x3::rule<presenceContainerPath_class, dataPath_> neměnné presenceContainerPath = "presenceContainerPath";
x3::rule<listInstancePath_class, dataPath_> neměnné listInstancePath = "listInstancePath";
x3::rule<leafListElementPath_class, dataPath_> neměnné leafListElementPath = "leafListElementPath";
x3::rule<initializePath_class, x3::unused_type> neměnné initializePath = "initializePath";
x3::rule<trailingSlash_class, x3::unused_type> neměnné trailingSlash = "trailingSlash";
x3::rule<absoluteStart_class, Scope> neměnné absoluteStart = "absoluteStart";
x3::rule<keyValue_class, keyValue_> neměnné keyValue = "keyValue";
x3::rule<key_identifier_class, std::string> neměnné key_identifier = "key_identifier";
x3::rule<listSuffix_class, std::vector<keyValue_>> neměnné listSuffix = "listSuffix";
x3::rule<createKeySuggestions_class, x3::unused_type> neměnné createKeySuggestions = "createKeySuggestions";
x3::rule<createValueSuggestions_class, x3::unused_type> neměnné createValueSuggestions = "createValueSuggestions";
x3::rule<suggestKeysEnd_class, x3::unused_type> neměnné suggestKeysEnd = "suggestKeysEnd";
x3::rule<class leafListValue_class, leaf_data_> neměnné leafListValue = "leafListValue";
auto pathEnd = x3::rule<class PathEnd>{"pathEnd"} = &space_separator | x3::eoi;

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

    std::function<pravdivost(neměnné Schema&, neměnné std::string& path)> m_filterFunction;

    NodeParser(neměnné std::function<pravdivost(neměnné Schema&, neměnné std::string& path)>& filterFunction)
        : m_filterFunction(filterFunction)
    {
    }

    template <typename It, typename Ctx, typename RCtx, typename Attr>
    pravdivost parse(It& begin, It end, Ctx neměnné& ctx, RCtx& rctx, Attr& attr) neměnné
    {
        std::string tableName;
        když constexpr (std::is_same<attribute_type, schemaNode_>()) {
            tableName = "schemaNode";
        } jinak {
            tableName = "dataNode";
        }
        x3::symbols<attribute_type> table(tableName);

        ParserContext& parserContext = x3::get<parser_context_tag>(ctx);
        parserContext.m_suggestions.clear();
        pro (neměnné auto& child : parserContext.m_schema.availableNodes(parserContext.currentSchemaPath(), Recursion::NonRecursive)) {
            attribute_type out;
            std::string parseString;
            když (child.first) {
                out.m_prefix = module_{*child.first};
                parseString = *child.first + ":";
            }
            parseString += child.second;

            když (!m_filterFunction(parserContext.m_schema, joinPaths(pathToSchemaString(parserContext.currentSchemaPath(), Prefixes::Always), parseString))) {
                pokračuj;
            }

            přepínač (parserContext.m_schema.nodeType(parserContext.currentSchemaPath(), child)) {
                případ yang::NodeTypes::Container:
                případ yang::NodeTypes::PresenceContainer:
                    out.m_suffix = container_{child.second};
                    parserContext.m_suggestions.emplace(Completion{parseString + "/"});
                    rozbij;
                případ yang::NodeTypes::Leaf:
                    out.m_suffix = leaf_{child.second};
                    parserContext.m_suggestions.emplace(Completion{parseString + " "});
                    rozbij;
                případ yang::NodeTypes::List:
                    když constexpr (std::is_same<attribute_type, schemaNode_>()) {
                        out.m_suffix = list_{child.second};
                    } jinak {
                        out.m_suffix = listElement_{child.second, {}};
                    }

                    když constexpr (COMPLETION_MODE == CompletionMode::Schema) {
                        parserContext.m_suggestions.emplace(Completion{parseString + "/"});
                    } jinak {
                        parserContext.m_suggestions.emplace(Completion{parseString, "[", Completion::WhenToAdd::IfFullMatch});
                    }
                    rozbij;
                případ yang::NodeTypes::LeafList:
                    když constexpr (std::is_same<attribute_type, schemaNode_>()) {
                        out.m_suffix = leafList_{child.second};
                    } jinak {
                        out.m_suffix = leafListElement_{child.second, {}};
                    }

                    když constexpr (COMPLETION_MODE == CompletionMode::Schema) {
                        parserContext.m_suggestions.emplace(Completion{parseString + "/"});
                    } jinak {
                        parserContext.m_suggestions.emplace(Completion{parseString, "[", Completion::WhenToAdd::IfFullMatch});
                    }
                    rozbij;
                případ yang::NodeTypes::Rpc:
                    out.m_suffix = rpcNode_{child.second};
                    parserContext.m_suggestions.emplace(Completion{parseString + "/"});
                    rozbij;
                případ yang::NodeTypes::Action:
                    out.m_suffix = actionNode_{child.second};
                    parserContext.m_suggestions.emplace(Completion{parseString + "/"});
                    rozbij;
                případ yang::NodeTypes::AnyXml:
                případ yang::NodeTypes::Notification:
                    pokračuj;
            }
            table.add(parseString, out);
            když (!child.first) {
                auto topLevelModule = parserContext.currentSchemaPath().m_nodes.begin()->m_prefix;
                out.m_prefix = topLevelModule;
                table.add(topLevelModule->m_name + ":" + parseString, out);
            }
        }
        table.add("..", attribute_type{nodeup_{}});
        parserContext.m_completionIterator = begin;

        když constexpr (PARSER_MODE == NodeParserMode::CompletionsOnly) {
            vrať true;
        } jinak {
            It saveIter = begin;

            auto res = table.parse(begin, end, ctx, rctx, attr);

            když (std::holds_alternative<leaf_>(attr.m_suffix)) {
                parserContext.m_tmpListKeyLeafPath.m_location = parserContext.currentSchemaPath();
                ModuleNodePair node{attr.m_prefix.flat_map([](neměnné auto& it) {
                                        vrať boost::optional<std::string>{it.m_name};
                                    }),
                                    std::get<leaf_>(attr.m_suffix).m_name};
                parserContext.m_tmpListKeyLeafPath.m_node = node;
            }

            když constexpr (std::is_same<attribute_type, dataNode_>()) {
                když (std::holds_alternative<listElement_>(attr.m_suffix)) {
                    parserContext.m_tmpListPath = parserContext.currentDataPath();
                    auto tmpList = list_{std::get<listElement_>(attr.m_suffix).m_name};
                    parserContext.m_tmpListPath.m_nodes.emplace_back(attr.m_prefix, tmpList);

                    res = listSuffix.parse(begin, end, ctx, rctx, std::get<listElement_>(attr.m_suffix).m_keys);

                    // FIXME: think of a better way to do this, that is, get rid of manual iterator reverting
                    když (!res) {
                        // If listSuffix didn't succeed, we check, if we allow incomplete nodes. If we do, then we replace listElement_ with list_.
                        // If we don't, we fail the whole symbol table.
                        když constexpr (PARSER_MODE == NodeParserMode::IncompleteDataNode) {
                            res = true;
                            attr.m_suffix = list_{std::get<listElement_>(attr.m_suffix).m_name};
                        } jinak {
                            begin = saveIter;
                        }
                    }
                }

                když (std::holds_alternative<leafListElement_>(attr.m_suffix)) {
                    parserContext.m_tmpListKeyLeafPath.m_location = parserContext.currentSchemaPath();
                    ModuleNodePair node{attr.m_prefix.flat_map([](neměnné auto& it) {
                                            vrať boost::optional<std::string>{it.m_name};
                                        }),
                                        std::get<leafListElement_>(attr.m_suffix).m_name};
                    parserContext.m_tmpListKeyLeafPath.m_node = node;
                    res = leafListValue.parse(begin, end, ctx, rctx, std::get<leafListElement_>(attr.m_suffix).m_value);

                    když (!res) {
                        když constexpr (PARSER_MODE == NodeParserMode::IncompleteDataNode) {
                            res = true;
                            attr.m_suffix = leafList_{std::get<leafListElement_>(attr.m_suffix).m_name};
                        } jinak {
                            begin = saveIter;
                        }
                    }
                }
            }

            když (res) {
                // After a path fragment, there can only be a slash or a "pathEnd". If this is not the case
                // then that means there are other unparsed characters after the fragment. In that case the parsing
                // needs to fail.
                res = (pathEnd | &char_('/')).parse(begin, end, ctx, rctx, x3::unused);
                když (!res) {
                    begin = saveIter;
                } jinak {
                    parserContext.pushPathFragment(attr);
                }
            }

            vrať res;
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
    std::function<pravdivost(neměnné Schema&, neměnné std::string& path)> m_filterFunction;

    PathParser(neměnné std::function<pravdivost(neměnné Schema&, neměnné std::string& path)>& filterFunction = [](neměnné auto&, neměnné auto&) { vrať true; })
        : m_filterFunction(filterFunction)
    {
    }

    template <typename It, typename Ctx, typename RCtx, typename Attr>
    pravdivost parse(It& begin, It end, Ctx neměnné& ctx, RCtx& rctx, Attr& attr) neměnné
    {
        initializePath.parse(begin, end, ctx, rctx, x3::unused);
        dataPath_ attrData;

        // absoluteStart has to be separate from the dataPath parser,
        // otherwise, if the "dataNode % '/'" parser fails, the begin iterator
        // gets reverted to before the starting slash.
        auto res = (-absoluteStart).parse(begin, end, ctx, rctx, attrData.m_scope);
        auto dataPath = x3::attr(attrData.m_scope)
            >> (dataNode<COMPLETION_MODE>{m_filterFunction} % '/' | pathEnd >> x3::attr(std::vector<dataNode_>{}))
            >> -trailingSlash;
        res = dataPath.parse(begin, end, ctx, rctx, attrData);

        // If we allow data paths with a list at the end, we just try to parse that separately.
        když constexpr (PARSER_MODE == PathParserMode::DataPathListEnd || PARSER_MODE == PathParserMode::AnyPath) {
            když (!res || !pathEnd.parse(begin, end, ctx, rctx, x3::unused)) {
                dataNode_ attrNodeList;
                auto hasListEnd = incompleteDataNode<COMPLETION_MODE>{m_filterFunction}.parse(begin, end, ctx, rctx, attrNodeList);
                když (hasListEnd) {
                    attrData.m_nodes.emplace_back(attrNodeList);
                    // If the trailing slash matches, no more nodes are parsed. That means no more completion. So, I
                    // generate them manually, but only if we're in AnyPath mode, so, for example, inside an `ls`
                    // command. If we're in DataPathListEnd it doesn't make sense to parse put any more nodes after the
                    // final list.
                    když constexpr (PARSER_MODE == PathParserMode::AnyPath) {
                        res = (-(trailingSlash >> x3::omit[pathCompletions<COMPLETION_MODE>{m_filterFunction}])).parse(begin, end, ctx, rctx, x3::unused);
                    } jinak {
                        res = (-trailingSlash).parse(begin, end, ctx, rctx, x3::unused);
                    }
                }
            }
        }

        attr = attrData;
        když constexpr (PARSER_MODE == PathParserMode::AnyPath) {
            // If our data path already has some listElement_ fragments, we can't parse rest of the path as a schema path
            auto hasLists = std::any_of(attrData.m_nodes.begin(), attrData.m_nodes.end(),
                [] (neměnné auto& node) { vrať std::holds_alternative<listElement_>(node.m_suffix); });
            // If parsing failed, or if there's more input we try parsing schema nodes.
            když (!hasLists) {
                když (!res || !pathEnd.parse(begin, end, ctx, rctx, x3::unused)) {
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
        vrať res;
    }
};

// Need to use these wrappers so that my PathParser class gets the proper
// attribute. Otherwise, Spirit injects the attribute of the outer parser that
// uses my PathParser.
// Example grammar: anyPath | module.
// The PathParser class would get a boost::variant as the attribute, but I
// don't want to deal with that, so I use these wrappers to ensure the
// attribute I want (and let Spirit deal with boost::variant).
auto neměnné anyPath = x3::rule<class anyPath_class, AnyPath>{"anyPath"} = PathParser<PathParserMode::AnyPath, CompletionMode::Schema>{};
auto neměnné dataPath = x3::rule<class dataPath_class, dataPath_>{"dataPath"} = PathParser<PathParserMode::DataPath, CompletionMode::Data>{};
auto neměnné dataPathListEnd = x3::rule<class dataPath_class, dataPath_>{"dataPath"} = PathParser<PathParserMode::DataPathListEnd, CompletionMode::Data>{};

#if __clang__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Woverloaded-shift-op-parentheses"
#endif

struct SuggestLeafListEnd : x3::parser<SuggestLeafListEnd> {
    using attribute_type = x3::unused_type;
    template <typename It, typename Ctx, typename RCtx, typename Attr>
    pravdivost parse(It& begin, It, Ctx neměnné& ctx, RCtx&, Attr&) neměnné
    {
        auto& parserContext = x3::get<parser_context_tag>(ctx);
        parserContext.m_completionIterator = begin;
        parserContext.m_suggestions = {Completion{"]"}};

        vrať true;
    }
} neměnné suggestLeafListEnd;

auto neměnné leafListValue_def =
    '[' >> leaf_data >> suggestLeafListEnd >> ']';

auto neměnné rest =
    x3::omit[x3::no_skip[+(x3::char_ - '/' - space_separator)]];

auto neměnné key_identifier_def =
    x3::lexeme[
        ((x3::alpha | char_("_")) >> *(x3::alnum | char_("_") | char_("-") | char_(".")))
    ];

auto neměnné createKeySuggestions_def =
    x3::eps;

auto neměnné createValueSuggestions_def =
    x3::eps;

auto neměnné suggestKeysEnd_def =
    x3::eps;

auto neměnné keyValue_def =
    key_identifier > '=' > createValueSuggestions > leaf_data;

auto neměnné keyValueWrapper =
    x3::no_skip['[' > createKeySuggestions > keyValue > suggestKeysEnd > ']'];

// even though we don't allow no keys to be supplied, the star allows me to check which keys are missing
auto neměnné listSuffix_def =
    *keyValueWrapper;

auto neměnné list_def =
    node_identifier >> !char_('[');

auto neměnné absoluteStart_def =
    x3::omit['/'] >> x3::attr(Scope::Absolute);

auto neměnné trailingSlash_def =
    x3::omit['/'];

auto neměnné filterConfigFalse = [](neměnné Schema& schema, neměnné std::string& path) {
    vrať schema.isConfig(path);
};

// A WritableOps value is injected through the `x3::with` with this tag (see usage of the tag). It controls whether
// `config: false` data can be set with the `set` command. This is used by yang-cli because that tool needs modeling of
// the full datastore, including the "read-only" data.
struct writableOps_tag;

PathParser<PathParserMode::DataPath, CompletionMode::Data> neměnné dataPathFilterConfigFalse{filterConfigFalse};

struct WritableLeafPath : x3::parser<WritableLeafPath> {
    using attribute_type = dataPath_;
    template <typename It, typename Ctx, typename RCtx, typename Attr>
    stálé pravdivost parse(It& begin, It end, Ctx neměnné& ctx, RCtx& rctx, Attr& attr)
    {
        pravdivost res;
        když (x3::get<writableOps_tag>(ctx) == WritableOps::Yes) {
            res = dataPath.parse(begin, end, ctx, rctx, attr);
        } jinak {
            res = dataPathFilterConfigFalse.parse(begin, end, ctx, rctx, attr);
        }
        když (!res) {
            vrať false;
        }

        když (attr.m_nodes.empty() || !std::holds_alternative<leaf_>(attr.m_nodes.back().m_suffix)) {
            auto& parserContext = x3::get<parser_context_tag>(ctx);
            parserContext.m_errorMsg = "This is not a path to leaf.";
            vrať false;
        }

        vrať true;
    }

} neměnné writableLeafPath;

enum class AllowInput {
    Yes,
    No
};

template <AllowInput ALLOW_INPUT>
struct RpcActionPath : x3::parser<RpcActionPath<ALLOW_INPUT>> {
    using attribute_type = dataPath_;
    template <typename It, typename Ctx, typename RCtx, typename Attr>
    stálé pravdivost parse(It& begin, It end, Ctx neměnné& ctx, RCtx& rctx, Attr& attr)
    {
        auto grammar = PathParser<PathParserMode::DataPath, CompletionMode::Data>{[] (neměnné Schema& schema, neměnné std::string& path) {
            když constexpr (ALLOW_INPUT == AllowInput::No) {
                auto nodeType = schema.nodeType(path);
                když (nodeType == yang::NodeTypes::Rpc || nodeType == yang::NodeTypes::Action) {
                    vrať !schema.hasInputNodes(path);
                }
            }

            vrať true;
        }};
        pravdivost res = grammar.parse(begin, end, ctx, rctx, attr);
        když (!res) {
            vrať false;
        }

        když (attr.m_nodes.empty()
            || (!std::holds_alternative<rpcNode_>(attr.m_nodes.back().m_suffix) && !std::holds_alternative<actionNode_>(attr.m_nodes.back().m_suffix))) {
            auto& parserContext = x3::get<parser_context_tag>(ctx);
            parserContext.m_errorMsg = "This is not a path to an RPC/action.";
            vrať false;
        }

        vrať true;
    }
};

auto neměnné noRpcOrAction = [](neměnné Schema& schema, neměnné std::string& path) {
    auto nodeType = schema.nodeType(path);
    vrať nodeType != yang::NodeTypes::Rpc && nodeType != yang::NodeTypes::Action;
};

auto neměnné getPath_def =
    PathParser<PathParserMode::DataPathListEnd, CompletionMode::Data>{noRpcOrAction} |
    (module >> "*");

auto neměnné cdPath_def =
    PathParser<PathParserMode::DataPath, CompletionMode::Data>{[] (neměnné Schema& schema, neměnné std::string& path) {
        vrať noRpcOrAction(schema, path) && schema.nodeType(path) != yang::NodeTypes::Leaf;
    }};

auto neměnné presenceContainerPath_def =
    dataPath;

auto neměnné listInstancePath_def =
    dataPath;

auto neměnné leafListElementPath_def =
    dataPath;

// A "nothing" parser, which is used to indicate we tried to parse a path
auto neměnné initializePath_def =
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
