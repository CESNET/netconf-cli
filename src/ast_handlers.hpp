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
        const Schema& schema = parserContext.m_schema;

        if (parserContext.m_tmpListKeys.find(ast.first) != parserContext.m_tmpListKeys.end()) {
            _pass(context) = false;
            parserContext.m_errorMsg = "Key \"" + ast.first + "\" was entered more than once.";
        } else if (!schema.listHasKey(parserContext.curPathSchema(), {parserContext.m_curModule, parserContext.m_tmpListName}, ast.first)) {
            _pass(context) = false;
            parserContext.m_errorMsg = parserContext.m_tmpListName + " is not indexed by \"" + ast.first + "\".";
        } else {
            parserContext.m_tmpListKeys.insert(ast.first);
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
        parserContext.m_tmpListKeyName = ast;
    }
};

struct module_identifier_class;

struct listPrefix_class {
    template <typename T, typename Iterator, typename Context>
    void on_success(Iterator const&, Iterator const&, T& ast, Context const& context)
    {
        auto& parserContext = x3::get<parser_context_tag>(context);
        const Schema& schema = parserContext.m_schema;

        if (schema.isList(parserContext.curPathSchema(), {parserContext.m_curModule, ast})) {
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

        const auto& keysNeeded = schema.listKeys(parserContext.curPathSchema(), {parserContext.m_curModule, parserContext.m_tmpListName});
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

        if (!schema.isList(parserContext.curPathSchema(), {parserContext.m_curModule, ast.m_name})) {
            _pass(context) = false;
        }
    }
};
struct nodeup_class {
    template <typename T, typename Iterator, typename Context>
    void on_success(Iterator const&, Iterator const&, T&, Context const& context)
    {
        auto& parserContext = x3::get<parser_context_tag>(context);

        if (parserContext.curPathSchema().m_nodes.empty())
            _pass(context) = false;
    }
};

struct container_class {
    template <typename T, typename Iterator, typename Context>
    void on_success(Iterator const&, Iterator const&, T& ast, Context const& context)
    {
        auto& parserContext = x3::get<parser_context_tag>(context);
        const auto& schema = parserContext.m_schema;

        if (!schema.isContainer(parserContext.curPathSchema(), {parserContext.m_curModule, ast.m_name}))
            _pass(context) = false;
    }
};

struct leaf_class {
    template <typename T, typename Iterator, typename Context>
    void on_success(Iterator const&, Iterator const&, T& ast, Context const& context)
    {
        auto& parserContext = x3::get<parser_context_tag>(context);
        const auto& schema = parserContext.m_schema;

        if (!schema.isLeaf(parserContext.curPathSchema(), {parserContext.m_curModule, ast.m_name}))
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
        parserContext.pushCurPath(ast);
        parserContext.m_curModule = boost::none;
    }
};

struct dataNodeList_class {
    template <typename T, typename Iterator, typename Context>
    void on_success(Iterator const&, Iterator const&, T& ast, Context const& context)
    {
        auto& parserContext = x3::get<parser_context_tag>(context);
        parserContext.pushCurPath(ast);
    }
};

struct dataNode_class {
    template <typename T, typename Iterator, typename Context>
    void on_success(Iterator const&, Iterator const&, T& ast, Context const& context)
    {
        auto& parserContext = x3::get<parser_context_tag>(context);
        parserContext.pushCurPath(ast);
        parserContext.m_curModule = boost::none;
    }
};

struct absoluteStart_class {
    template <typename T, typename Iterator, typename Context>
    void on_success(Iterator const&, Iterator const&, T&, Context const& context)
    {
        auto& parserContext = x3::get<parser_context_tag>(context);
        parserContext.clearCurPath();
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
            auto location = pathWithoutLastNode(parserContext.curPathSchema());

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
    void on_success(Iterator const&, Iterator const&, T& ast, Context const& context)
    {
        auto& parserContext = x3::get<parser_context_tag>(context);
        try {
            auto leaf = boost::get<leaf_>(ast.m_nodes.back().m_suffix);
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

        if (!schema.isModule(parserContext.curPathSchema(), ast.m_name)) {
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
            leaf_ leaf = boost::get<leaf_>(parserContext.curPathSchema().m_nodes.back().m_suffix);
            schemaPath_ location = pathWithoutLastNode(parserContext.curPathSchema());
            boost::optional<std::string> module;
            if (parserContext.curPathSchema().m_nodes.back().m_prefix)
                module = parserContext.curPathSchema().m_nodes.back().m_prefix.value().m_name;
            parserContext.m_errorMsg = "Expected " + leafDataTypeToString(schema.leafType(location, {module, leaf.m_name})) + " here:";
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
        boost::optional<std::string> module;
        if (parserContext.curPathSchema().m_nodes.back().m_prefix)
            module = parserContext.curPathSchema().m_nodes.back().m_prefix.value().m_name;

        leaf_ leaf = boost::get<leaf_>(parserContext.curPathSchema().m_nodes.back().m_suffix);
        schemaPath_ location = pathWithoutLastNode(parserContext.curPathSchema());

        auto type = schema.leafType(location, {module, leaf.m_name});
        if (type == yang::LeafDataTypes::LeafRef) {
            type = schema.leafrefBase(location, {module, leaf.m_name});
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
        boost::optional<std::string> module;
        if (parserContext.curPathSchema().m_nodes.back().m_prefix)
            module = parserContext.curPathSchema().m_nodes.back().m_prefix.value().m_name;

        leaf_ leaf = boost::get<leaf_>(parserContext.curPathSchema().m_nodes.back().m_suffix);
        schemaPath_ location = pathWithoutLastNode(parserContext.curPathSchema());

        if (!schema.leafEnumHasValue(location, {module, leaf.m_name}, ast.m_value)) {
            _pass(context) = false;

            parserContext.m_errorMsg = "Expected an enum here. Allowed values:";
            for (const auto& it : schema.enumValues(location, {module, leaf.m_name})) {
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
        boost::optional<std::string> module;
        if (parserContext.curPathSchema().m_nodes.back().m_prefix)
            module = parserContext.curPathSchema().m_nodes.back().m_prefix.value().m_name;

        leaf_ leaf = boost::get<leaf_>(parserContext.curPathSchema().m_nodes.back().m_suffix);
        schemaPath_ location = pathWithoutLastNode(parserContext.curPathSchema());

        ModuleValuePair pair;
        if (ast.m_prefix) {
            pair.first = ast.m_prefix.get().m_name;
        }
        pair.second = ast.m_value;

        if (!schema.leafIdentityIsValid(location, {module, leaf.m_name}, pair)) {
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
        parserContext.resetCurPathToOrig();
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
        auto suggestions = schema.childNodes(parserContext.curPathSchema(), Recursion::NonRecursive);
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

                if (schema.isLeaf(parserContext.curPathSchema(), node)) {
                    return it + " ";
                }
                if (schema.isContainer(parserContext.curPathSchema(), node)) {
                    return it + "/";
                }
                if (schema.isList(parserContext.curPathSchema(), node)) {
                    return it + "[";
                }
                return it;
        });
        parserContext.m_suggestions = suffixesAdded;
    }
};

std::set<std::string> generateMissingKeyCompletionSet(std::set<std::string> keysNeeded, std::set<std::string> currentSet);

struct createKeySuggestions_class {
    template <typename T, typename Iterator, typename Context>
    void on_success(Iterator const& begin, Iterator const&, T&, Context const& context)
    {
        auto& parserContext = x3::get<parser_context_tag>(context);
        const auto& schema = parserContext.m_schema;

        parserContext.m_completionIterator = begin;

        const auto& keysNeeded = schema.listKeys(parserContext.curPathSchema(), {parserContext.m_curModule, parserContext.m_tmpListName});
        parserContext.m_suggestions = generateMissingKeyCompletionSet(keysNeeded, parserContext.m_tmpListKeys);
    }
};

struct createValueSuggestions_class {
    template <typename T, typename Iterator, typename Context>
    void on_success(Iterator const& begin, Iterator const&, T&, Context const& context)
    {
        auto& parserContext = x3::get<parser_context_tag>(context);
        const auto& dataQuery = parserContext.m_dataquery;

        parserContext.m_completionIterator = begin;
        auto keysAndValues = dataQuery.listKeys(parserContext.curPathData(), {parserContext.m_curModule, parserContext.m_tmpListName});
        parserContext.m_suggestions = keysAndValues.at(parserContext.m_tmpListKeyName);
    }
};

struct suggestKeysEnd_class {
    template <typename T, typename Iterator, typename Context>
    void on_success(Iterator const& begin, Iterator const&, T&, Context const& context)
    {
        auto& parserContext = x3::get<parser_context_tag>(context);
        const auto& schema = parserContext.m_schema;

        parserContext.m_completionIterator = begin;
        const auto& keysNeeded = schema.listKeys(parserContext.curPathSchema(), {parserContext.m_curModule, parserContext.m_tmpListName});
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
        parserContext.m_completionIterator = begin;
        const Schema& schema = parserContext.m_schema;

        boost::optional<std::string> module;
        if (parserContext.curPathSchema().m_nodes.back().m_prefix)
            module = parserContext.curPathSchema().m_nodes.back().m_prefix.value().m_name;

        leaf_ leaf = boost::get<leaf_>(parserContext.curPathSchema().m_nodes.back().m_suffix);
        schemaPath_ location = pathWithoutLastNode(parserContext.curPathSchema());

        // Only generate completions if the type is enum so that we don't
        // overwrite some other completions.
        if (schema.leafType(location, {module, leaf.m_name}) == yang::LeafDataTypes::Enum)
            parserContext.m_suggestions = schema.enumValues(location, {module, leaf.m_name});
    }
};

// FIXME: can I reuse createEnumSuggestions?
struct createIdentitySuggestions_class {
    template <typename T, typename Iterator, typename Context>
    void on_success(Iterator const& begin, Iterator const&, T&, Context const& context)
    {
        auto& parserContext = x3::get<parser_context_tag>(context);
        parserContext.m_completionIterator = begin;
        const Schema& schema = parserContext.m_schema;

        boost::optional<std::string> module;
        if (parserContext.curPathSchema().m_nodes.back().m_prefix)
            module = parserContext.curPathSchema().m_nodes.back().m_prefix.value().m_name;

        leaf_ leaf = boost::get<leaf_>(parserContext.curPathSchema().m_nodes.back().m_suffix);
        schemaPath_ location = pathWithoutLastNode(parserContext.curPathSchema());

        // Only generate completions if the type is identityref so that we
        // don't overwrite some other completions.
        if (schema.leafType(location, {module, leaf.m_name}) == yang::LeafDataTypes::IdentityRef)
            parserContext.m_suggestions = schema.validIdentities(location, {module, leaf.m_name}, Prefixes::WhenNeeded);
    }
};
