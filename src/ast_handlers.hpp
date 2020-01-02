/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/

#pragma once

#include <boost/mpl/for_each.hpp>
#include <boost/spirit/home/x3.hpp>
#include <boost/spirit/home/x3/support/utility/error_reporting.hpp>


#include "ast_commands.hpp"
#include "schema.hpp"
#include "utils.hpp"
namespace x3 = boost::spirit::x3;

struct parser_context_tag;

struct keyValue_class {
    template <typename T, typename Iterator, typename Context>
    void on_success(Iterator const&, Iterator const&, T& ast, Context const& context)
    {
        auto& parserContext = x3::get<parser_context_tag>(context);

        if (parserContext.m_tmpListKeys.find(ast.first) != parserContext.m_tmpListKeys.end()) {
            _pass(context) = false;
            parserContext.m_errorMsg = "Key \"" + ast.first + "\" was entered more than once.";
        } else {
            parserContext.m_tmpListKeys.insert({ast.first, ast.second});
        }
    }

    template <typename Iterator, typename Exception, typename Context>
    x3::error_handler_result on_error(Iterator&, Iterator const&, Exception const&, Context const& context)
    {
        auto& parserContext = x3::get<parser_context_tag>(context);
        parserContext.m_errorMsg = "Error parsing key values here:";
        return x3::error_handler_result::rethrow;
    }
};

struct node_identifier_class {
    template <typename T, typename Iterator, typename Context>
    void on_success(Iterator const&, Iterator const&, T&, Context const& context)
    {
        auto& parserContext = x3::get<parser_context_tag>(context);

        if (!parserContext.m_topLevelModulePresent) {
            if (parserContext.m_errorMsg.empty())
                parserContext.m_errorMsg = "You have to specify a top level module.";
            _pass(context) = false;
        }
    }
};

struct key_identifier_class {
    template <typename T, typename Iterator, typename Context>
    void on_success(Iterator const&, Iterator const&, T& ast, Context const& context)
    {
        auto& parserContext = x3::get<parser_context_tag>(context);
        const Schema& schema = parserContext.m_schema;
        schemaPath_ location = parserContext.currentSchemaPath();
        ModuleNodePair list{parserContext.m_curModule, parserContext.m_tmpListName};

        if (schema.listHasKey(location, list, ast)) {
            schemaNode_ listNode;
            listNode.m_prefix = parserContext.m_curModule ? boost::optional<module_>{{*parserContext.m_curModule}} : boost::none;
            listNode.m_suffix = list_{parserContext.m_tmpListName};
            location.m_nodes.push_back(listNode);
            parserContext.m_tmpListKeyLeafPath.m_location = location;
            parserContext.m_tmpListKeyLeafPath.m_node = {boost::none, ast};
        } else {
            parserContext.m_errorMsg = parserContext.m_tmpListName + " is not indexed by \"" + ast + "\".";
            _pass(context) = false;
        }
    }
};

struct module_identifier_class;

struct listPrefix_class {
    template <typename T, typename Iterator, typename Context>
    void on_success(Iterator const&, Iterator const&, T& ast, Context const& context)
    {
        auto& parserContext = x3::get<parser_context_tag>(context);
        const Schema& schema = parserContext.m_schema;

        if (schema.isList(parserContext.currentSchemaPath(), {parserContext.m_curModule, ast})) {
            parserContext.m_tmpListName = ast;
        } else {
            _pass(context) = false;
        }
    }
};

struct listSuffix_class {
    template <typename T, typename Iterator, typename Context>
    void on_success(Iterator const&, Iterator const&, T& ast, Context const& context)
    {
        auto& parserContext = x3::get<parser_context_tag>(context);
        const Schema& schema = parserContext.m_schema;

        const auto& keysNeeded = schema.listKeys(parserContext.currentSchemaPath(), {parserContext.m_curModule, parserContext.m_tmpListName});
        std::set<std::string> keysSupplied;
        for (const auto& it : ast)
            keysSupplied.insert(it.first);

        if (keysNeeded != keysSupplied) {
            parserContext.m_errorMsg = "Not enough keys for " + parserContext.m_tmpListName + ". " +
                                       "These keys were not supplied:";
            std::set<std::string> missingKeys;
            std::set_difference(keysNeeded.begin(), keysNeeded.end(),
                                keysSupplied.begin(), keysSupplied.end(),
                                std::inserter(missingKeys, missingKeys.end()));

            for (const auto& it : missingKeys)
                parserContext.m_errorMsg += " " + it;
            parserContext.m_errorMsg += ".";

            _pass(context) = false;
        }
    }

    template <typename Iterator, typename Exception, typename Context>
    x3::error_handler_result on_error(Iterator&, Iterator const&, Exception const&, Context const& context)
    {
        auto& parserContext = x3::get<parser_context_tag>(context);
        if (parserContext.m_errorMsg.empty())
            parserContext.m_errorMsg = "Expecting ']' here:";
        return x3::error_handler_result::rethrow;
    }
};
struct listElement_class {
    template <typename Iterator, typename Exception, typename Context>
    x3::error_handler_result on_error(Iterator&, Iterator const&, Exception const&, Context const& context)
    {
        auto& parserContext = x3::get<parser_context_tag>(context);
        if (parserContext.m_errorMsg.empty()) {
            return x3::error_handler_result::fail;
        } else {
            return x3::error_handler_result::rethrow;
        }
    }
};
struct list_class {
    template <typename T, typename Iterator, typename Context>
    void on_success(Iterator const&, Iterator const&, T& ast, Context const& context)
    {
        auto& parserContext = x3::get<parser_context_tag>(context);
        const Schema& schema = parserContext.m_schema;

        if (!schema.isList(parserContext.currentSchemaPath(), {parserContext.m_curModule, ast.m_name})) {
            _pass(context) = false;
        }
    }
};
struct nodeup_class {
    template <typename T, typename Iterator, typename Context>
    void on_success(Iterator const&, Iterator const&, T&, Context const& context)
    {
        auto& parserContext = x3::get<parser_context_tag>(context);

        if (parserContext.currentSchemaPath().m_nodes.empty())
            _pass(context) = false;
    }
};

struct container_class {
    template <typename T, typename Iterator, typename Context>
    void on_success(Iterator const&, Iterator const&, T& ast, Context const& context)
    {
        auto& parserContext = x3::get<parser_context_tag>(context);
        const auto& schema = parserContext.m_schema;

        if (!schema.isContainer(parserContext.currentSchemaPath(), {parserContext.m_curModule, ast.m_name}))
            _pass(context) = false;
    }
};

struct leaf_class {
    template <typename T, typename Iterator, typename Context>
    void on_success(Iterator const&, Iterator const&, T& ast, Context const& context)
    {
        auto& parserContext = x3::get<parser_context_tag>(context);
        const auto& schema = parserContext.m_schema;

        if (!schema.isLeaf(parserContext.currentSchemaPath(), {parserContext.m_curModule, ast.m_name}))
            _pass(context) = false;
    }
};

struct module_class {
    template <typename T, typename Iterator, typename Context>
    void on_success(Iterator const&, Iterator const&, T& ast, Context const& context)
    {
        auto& parserContext = x3::get<parser_context_tag>(context);
        const auto& schema = parserContext.m_schema;

        if (schema.isModule(ast.m_name)) {
            parserContext.m_curModule = ast.m_name;
            parserContext.m_topLevelModulePresent = true;
        } else {
            parserContext.m_errorMsg = "Invalid module name.";
            _pass(context) = false;
        }
    }
};

struct schemaNode_class {
    template <typename T, typename Iterator, typename Context>
    void on_success(Iterator const&, Iterator const&, T& ast, Context const& context)
    {
        auto& parserContext = x3::get<parser_context_tag>(context);
        parserContext.pushPathFragment(ast);
        parserContext.m_curModule = boost::none;
    }
};

struct dataNodeList_class {
    template <typename T, typename Iterator, typename Context>
    void on_success(Iterator const&, Iterator const&, T& ast, Context const& context)
    {
        auto& parserContext = x3::get<parser_context_tag>(context);
        parserContext.pushPathFragment(ast);
    }
};

struct dataNode_class {
    template <typename T, typename Iterator, typename Context>
    void on_success(Iterator const&, Iterator const&, T& ast, Context const& context)
    {
        auto& parserContext = x3::get<parser_context_tag>(context);
        parserContext.pushPathFragment(ast);
        parserContext.m_curModule = boost::none;
    }
};

struct absoluteStart_class {
    template <typename T, typename Iterator, typename Context>
    void on_success(Iterator const&, Iterator const&, T&, Context const& context)
    {
        auto& parserContext = x3::get<parser_context_tag>(context);
        parserContext.clearPath();
    }
};

struct dataNodesListEnd_class;

struct dataPathListEnd_class;

struct dataPath_class {
    template <typename Iterator, typename Exception, typename Context>
    x3::error_handler_result on_error(Iterator&, Iterator const&, Exception const&, Context const& context)
    {
        auto& parserContext = x3::get<parser_context_tag>(context);
        if (parserContext.m_errorMsg.empty()) {
            parserContext.m_errorMsg = "Expected path.";
            return x3::error_handler_result::fail;
        } else {
            return x3::error_handler_result::rethrow;
        }
    }
};

struct schemaPath_class {
    template <typename Iterator, typename Exception, typename Context>
    x3::error_handler_result on_error(Iterator&, Iterator const&, Exception const&, Context const& context)
    {
        auto& parserContext = x3::get<parser_context_tag>(context);
        if (parserContext.m_errorMsg.empty()) {
            parserContext.m_errorMsg = "Expected path.";
            return x3::error_handler_result::fail;
        } else {
            return x3::error_handler_result::rethrow;
        }
    }
};

struct discard_class;

struct ls_class;

struct cd_class {
    template <typename Iterator, typename Exception, typename Context>
    x3::error_handler_result on_error(Iterator&, Iterator const&, Exception const& x, Context const& context)
    {
        auto& parserContext = x3::get<parser_context_tag>(context);
        if (parserContext.m_errorMsg.empty())
            parserContext.m_errorMsg = "Expected " + x.which() + " here:";
        return x3::error_handler_result::rethrow;
    }
};

struct presenceContainerPath_class {
    template <typename T, typename Iterator, typename Context>
    void on_success(Iterator const&, Iterator const&, T& ast, Context const& context)
    {
        auto& parserContext = x3::get<parser_context_tag>(context);
        const auto& schema = parserContext.m_schema;
        try {
            boost::optional<std::string> module;
            if (ast.m_nodes.back().m_prefix)
                module = ast.m_nodes.back().m_prefix.value().m_name;
            container_ cont = boost::get<container_>(ast.m_nodes.back().m_suffix);
            auto location = pathWithoutLastNode(parserContext.currentSchemaPath());

            if (!schema.isPresenceContainer(location, {module, cont.m_name})) {
                parserContext.m_errorMsg = "This container is not a presence container.";
                _pass(context) = false;
            }
        } catch (boost::bad_get&) {
            parserContext.m_errorMsg = "This is not a container.";
            _pass(context) = false;
        }
    }
};

struct listInstancePath_class {
    template <typename T, typename Iterator, typename Context>
    void on_success(Iterator const&, Iterator const&, T& ast, Context const& context)
    {
        auto& parserContext = x3::get<parser_context_tag>(context);
        if (ast.m_nodes.back().m_suffix.type() != typeid(listElement_)) {
            parserContext.m_errorMsg = "This is not a list instance.";
            _pass(context) = false;
        }
    }
};

struct space_separator_class {
    template <typename T, typename Iterator, typename Context>
    void on_success(Iterator const&, Iterator const&, T&, Context const& context)
    {
        auto& parserContext = x3::get<parser_context_tag>(context);
        parserContext.m_suggestions.clear();
        parserContext.m_completionIterator = boost::none;
        parserContext.m_completionSuffix.clear();
    }
};

struct create_class {
    template <typename Iterator, typename Exception, typename Context>
    x3::error_handler_result on_error(Iterator&, Iterator const&, Exception const&, Context const& context)
    {
        auto& parserContext = x3::get<parser_context_tag>(context);
        if (parserContext.m_errorMsg.empty())
            parserContext.m_errorMsg = "Couldn't parse create/delete command.";
        return x3::error_handler_result::rethrow;
    }
};

struct delete_class {
    template <typename Iterator, typename Exception, typename Context>
    x3::error_handler_result on_error(Iterator&, Iterator const&, Exception const&, Context const& context)
    {
        auto& parserContext = x3::get<parser_context_tag>(context);
        if (parserContext.m_errorMsg.empty())
            parserContext.m_errorMsg = "Couldn't parse create/delete command.";
        return x3::error_handler_result::rethrow;
    }
};

struct leaf_path_class {
    template <typename T, typename Iterator, typename Context>
    void on_success(Iterator const&, Iterator const&, T&, Context const& context)
    {
        auto& parserContext = x3::get<parser_context_tag>(context);
        try {
            auto lastNode = parserContext.currentSchemaPath().m_nodes.back();
            auto leaf = boost::get<leaf_>(lastNode.m_suffix);
            auto location = pathWithoutLastNode(parserContext.currentSchemaPath());
            ModuleNodePair node{lastNode.m_prefix.flat_map([](const auto& it) { return boost::optional<std::string>{it.m_name}; }), leaf.m_name};

            parserContext.m_tmpListKeyLeafPath.m_location = location;
            parserContext.m_tmpListKeyLeafPath.m_node = node;

        } catch (boost::bad_get&) {
            parserContext.m_errorMsg = "This is not a path to leaf.";
            _pass(context) = false;
        }
    }
};

// This handler only checks if the module exists
// It doesn't set any ParserContext flags (except the error message)
struct data_module_prefix_class {
    template <typename T, typename Iterator, typename Context>
    void on_success(Iterator const&, Iterator const&, T& ast, Context const& context)
    {
        auto& parserContext = x3::get<parser_context_tag>(context);
        const auto& schema = parserContext.m_schema;

        if (!schema.isModule(parserContext.currentSchemaPath(), ast.m_name)) {
            parserContext.m_errorMsg = "Invalid module name.";
            _pass(context) = false;
        }
    }
};

struct leaf_data_class {
    template <typename Iterator, typename Exception, typename Context>
    x3::error_handler_result on_error(Iterator&, Iterator const&, Exception const&, Context const& context)
    {
        auto& parserContext = x3::get<parser_context_tag>(context);
        auto& schema = parserContext.m_schema;
        if (parserContext.m_errorMsg.empty()) {
            parserContext.m_errorMsg = "leaf data type mismatch: Expected " +
                leafDataTypeToString(schema.leafType(parserContext.m_tmpListKeyLeafPath.m_location, parserContext.m_tmpListKeyLeafPath.m_node)) + " here:";
            return x3::error_handler_result::fail;
        }
        return x3::error_handler_result::rethrow;
    }
};

template <yang::LeafDataTypes TYPE>
struct leaf_data_base_class {
    yang::LeafDataTypes m_type;

    leaf_data_base_class()
        : m_type(TYPE)
    {
    }

    template <typename T, typename Iterator, typename Context>
    void on_success(Iterator const&, Iterator const&, T&, Context const& context)
    {
        auto& parserContext = x3::get<parser_context_tag>(context);
        auto& schema = parserContext.m_schema;

        auto type = schema.leafType(parserContext.m_tmpListKeyLeafPath.m_location, parserContext.m_tmpListKeyLeafPath.m_node);
        if (type == yang::LeafDataTypes::LeafRef) {
            type = schema.leafrefBase(parserContext.m_tmpListKeyLeafPath.m_location, parserContext.m_tmpListKeyLeafPath.m_node);
        }

        if (type != m_type) {
            _pass(context) = false;
        }
    }
};

struct leaf_data_binary_data_class;

struct leaf_data_enum_class : leaf_data_base_class<yang::LeafDataTypes::Enum> {
    using leaf_data_base_class::leaf_data_base_class;

    template <typename T, typename Iterator, typename Context>
    void on_success(Iterator const& start, Iterator const& end, T& ast, Context const& context)
    {
        leaf_data_base_class::on_success(start, end, ast, context);
        // Base class on_success cannot return for us, so we check if it failed the parser.
        if (_pass(context) == false)
            return;
        auto& parserContext = x3::get<parser_context_tag>(context);
        auto& schema = parserContext.m_schema;

        if (!schema.leafEnumHasValue(parserContext.m_tmpListKeyLeafPath.m_location, parserContext.m_tmpListKeyLeafPath.m_node, ast.m_value)) {
            _pass(context) = false;
            parserContext.m_errorMsg = "leaf data type mismatch: Expected an enum here. Allowed values:";
            for (const auto& it : schema.enumValues(parserContext.m_tmpListKeyLeafPath.m_location, parserContext.m_tmpListKeyLeafPath.m_node)) {
                parserContext.m_errorMsg += " " + it;
            }
        }
    }
};

struct leaf_data_identityRef_data_class;

struct leaf_data_identityRef_class : leaf_data_base_class<yang::LeafDataTypes::IdentityRef> {
    using leaf_data_base_class::leaf_data_base_class;

    template <typename T, typename Iterator, typename Context>
    void on_success(Iterator const& start, Iterator const& end, T& ast, Context const& context)
    {
        // FIXME: can I reuse leaf_data_enum_class somehow..?
        leaf_data_base_class::on_success(start, end, ast, context);
        // Base class on_success cannot return for us, so we check if it failed the parser.
        if (_pass(context) == false)
            return;
        auto& parserContext = x3::get<parser_context_tag>(context);
        auto& schema = parserContext.m_schema;

        ModuleValuePair pair;
        if (ast.m_prefix) {
            pair.first = ast.m_prefix.get().m_name;
        }
        pair.second = ast.m_value;

        if (!schema.leafIdentityIsValid(parserContext.m_tmpListKeyLeafPath.m_location, parserContext.m_tmpListKeyLeafPath.m_node, pair)) {
            _pass(context) = false;
        }
    }
};

struct set_class {
    template <typename Iterator, typename Exception, typename Context>
    x3::error_handler_result on_error(Iterator&, Iterator const&, Exception const& x, Context const& context)
    {
        auto& parserContext = x3::get<parser_context_tag>(context);
        if (parserContext.m_errorMsg.empty())
            parserContext.m_errorMsg = "Expected " + x.which() + " here:";
        return x3::error_handler_result::rethrow;
    }
};

struct commit_class;

struct help_class;

struct get_class;

struct command_class {
    template <typename Iterator, typename Exception, typename Context>
    x3::error_handler_result on_error(Iterator&, Iterator const&, Exception const& x, Context const& context)
    {
        auto& parserContext = x3::get<parser_context_tag>(context);
        auto& error_handler = x3::get<x3::error_handler_tag>(context).get();
        if (parserContext.m_errorMsg.empty()) {
            parserContext.m_errorMsg = "Unknown command.";
        }
        error_handler(x.where(), parserContext.m_errorMsg);
        return x3::error_handler_result::fail;
    }
};

struct initializePath_class {
    template <typename T, typename Iterator, typename Context>
    void on_success(Iterator const&, Iterator const&, T&, Context const& context)
    {
        auto& parserContext = x3::get<parser_context_tag>(context);
        parserContext.resetPath();
        parserContext.m_tmpListKeys.clear();
        parserContext.m_tmpListName.clear();
        parserContext.m_suggestions.clear();
    }
};

struct trailingSlash_class;

struct createPathSuggestions_class {
    template <typename T, typename Iterator, typename Context>
    void on_success(Iterator const& begin, Iterator const&, T&, Context const& context)
    {
        auto& parserContext = x3::get<parser_context_tag>(context);
        const auto& schema = parserContext.m_schema;

        parserContext.m_completionIterator = begin;
        auto suggestions = schema.childNodes(parserContext.currentSchemaPath(), Recursion::NonRecursive);
        std::set<std::string> suffixesAdded;
        std::transform(suggestions.begin(), suggestions.end(),
            std::inserter(suffixesAdded, suffixesAdded.end()),
            [&parserContext, &schema] (auto it) {
                ModuleNodePair node;
                if (auto colonPos = it.find(":"); colonPos != it.npos) {
                    node.first = it.substr(0, colonPos);
                    node.second = it.substr(colonPos + 1, node.second.npos);
                } else {
                    node.first = boost::none;
                    node.second = it;
                }

                if (schema.isLeaf(parserContext.currentSchemaPath(), node)) {
                    return it + " ";
                }
                if (schema.isContainer(parserContext.currentSchemaPath(), node)) {
                    return it + "/";
                }
                if (schema.isList(parserContext.currentSchemaPath(), node)) {
                    return it + "[";
                }
                return it;
        });
        parserContext.m_suggestions = suffixesAdded;
    }
};

std::set<std::string> generateMissingKeyCompletionSet(std::set<std::string> keysNeeded, std::map<std::string, leaf_data_> currentSet);

struct createKeySuggestions_class {
    template <typename T, typename Iterator, typename Context>
    void on_success(Iterator const& begin, Iterator const&, T&, Context const& context)
    {
        auto& parserContext = x3::get<parser_context_tag>(context);
        const auto& schema = parserContext.m_schema;

        parserContext.m_completionIterator = begin;

        const auto& keysNeeded = schema.listKeys(parserContext.currentSchemaPath(), {parserContext.m_curModule, parserContext.m_tmpListName});
        parserContext.m_suggestions = generateMissingKeyCompletionSet(keysNeeded, parserContext.m_tmpListKeys);
    }
};

std::string leafDataToCompletion(const leaf_data_& value);

struct createValueSuggestions_class {
    template <typename T, typename Iterator, typename Context>
    void on_success(Iterator const& begin, Iterator const&, T&, Context const& context)
    {
        auto& parserContext = x3::get<parser_context_tag>(context);
        const auto& dataQuery = parserContext.m_dataquery;

        parserContext.m_completionIterator = begin;
        auto listInstances = dataQuery.listKeys(parserContext.currentDataPath(), {parserContext.m_curModule, parserContext.m_tmpListName});

        // Filter out instances, which correspond to the key-values already present m_tmpListKeys
        decltype(listInstances) filteredInstances;
        std::copy_if(listInstances.begin(), listInstances.end(), std::inserter(filteredInstances, filteredInstances.end()), [&parserContext] (const auto& instance) {
            return std::all_of(parserContext.m_tmpListKeys.begin(),
                               parserContext.m_tmpListKeys.end(),
                               [&instance] (const auto& keyValue) {
                                   return instance.find(keyValue.first) != instance.end() && instance.at(keyValue.first) == keyValue.second;
                               });
        });
        std::set<std::string> validValues;

        std::transform(filteredInstances.begin(), filteredInstances.end(), std::inserter(validValues, validValues.end()), [&parserContext] (const auto& instance) {
            return leafDataToCompletion(instance.at(parserContext.m_tmpListKeyLeafPath.m_node.second));
        });

        parserContext.m_suggestions = validValues;
    }
};

struct suggestKeysEnd_class {
    template <typename T, typename Iterator, typename Context>
    void on_success(Iterator const& begin, Iterator const&, T&, Context const& context)
    {
        auto& parserContext = x3::get<parser_context_tag>(context);
        const auto& schema = parserContext.m_schema;

        parserContext.m_completionIterator = begin;
        const auto& keysNeeded = schema.listKeys(parserContext.currentSchemaPath(), {parserContext.m_curModule, parserContext.m_tmpListName});
        if (generateMissingKeyCompletionSet(keysNeeded, parserContext.m_tmpListKeys).empty()) {
            parserContext.m_suggestions = {"]/"};
        } else {
            parserContext.m_suggestions = {"]["};
        }
    }
};

struct commandNamesVisitor {
    template <typename T>
    auto operator()(boost::type<T>)
    {
        return T::name;
    }
};

struct createCommandSuggestions_class {
    template <typename T, typename Iterator, typename Context>
    void on_success(Iterator const& begin, Iterator const&, T&, Context const& context)
    {
        auto& parserContext = x3::get<parser_context_tag>(context);
        parserContext.m_completionIterator = begin;

        parserContext.m_suggestions.clear();
        boost::mpl::for_each<CommandTypes, boost::type<boost::mpl::_>>([&parserContext](auto cmd) {
            parserContext.m_suggestions.emplace(commandNamesVisitor()(cmd));
        });
    }
};

struct completing_class {
    template <typename T, typename Iterator, typename Context>
    void on_success(Iterator const&, Iterator const&, T&, Context const& context)
    {
        auto& parserContext = x3::get<parser_context_tag>(context);

        if (!parserContext.m_completing)
            _pass(context) = false;
    }
};

struct createEnumSuggestions_class {
    template <typename T, typename Iterator, typename Context>
    void on_success(Iterator const& begin, Iterator const&, T&, Context const& context)
    {
        auto& parserContext = x3::get<parser_context_tag>(context);
        const Schema& schema = parserContext.m_schema;

        // Only generate completions if the type is enum so that we don't
        // overwrite some other completions.
        if (schema.leafType(parserContext.m_tmpListKeyLeafPath.m_location, parserContext.m_tmpListKeyLeafPath.m_node) == yang::LeafDataTypes::Enum) {
            parserContext.m_completionIterator = begin;
            parserContext.m_suggestions = schema.enumValues(parserContext.m_tmpListKeyLeafPath.m_location, parserContext.m_tmpListKeyLeafPath.m_node);
        }
    }
};

// FIXME: can I reuse createEnumSuggestions?
struct createIdentitySuggestions_class {
    template <typename T, typename Iterator, typename Context>
    void on_success(Iterator const& begin, Iterator const&, T&, Context const& context)
    {
        auto& parserContext = x3::get<parser_context_tag>(context);
        const Schema& schema = parserContext.m_schema;

        // Only generate completions if the type is identityref so that we
        // don't overwrite some other completions.
        if (schema.leafType(parserContext.m_tmpListKeyLeafPath.m_location, parserContext.m_tmpListKeyLeafPath.m_node) == yang::LeafDataTypes::IdentityRef) {
            parserContext.m_completionIterator = begin;
            parserContext.m_suggestions = schema.validIdentities(parserContext.m_tmpListKeyLeafPath.m_location, parserContext.m_tmpListKeyLeafPath.m_node, Prefixes::WhenNeeded);
        }
    }
};
