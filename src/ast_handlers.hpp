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
#include "parser_context.hpp"
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
            listNode.m_prefix = parserContext.m_curModule.flat_map([] (auto mod) { return boost::optional<module_>{{mod}}; });;
            listNode.m_suffix = list_{parserContext.m_tmpListName};
            location.m_nodes.push_back(listNode);
            parserContext.m_tmpListKeyLeafPath.m_location = location;
            parserContext.m_tmpListKeyLeafPath.m_node = { parserContext.m_curModule, ast };
        } else {
            parserContext.m_errorMsg = parserContext.m_tmpListName + " is not indexed by \"" + ast + "\".";
            _pass(context) = false;
        }
    }
};

struct module_identifier_class;

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

struct absoluteStart_class {
    template <typename T, typename Iterator, typename Context>
    void on_success(Iterator const&, Iterator const&, T&, Context const& context)
    {
        auto& parserContext = x3::get<parser_context_tag>(context);
        parserContext.clearPath();
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
        if (ast.m_nodes.empty()) {
            parserContext.m_errorMsg = "This container is not a presence container.";
            _pass(context) = false;
            return;
        }

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
        if (ast.m_nodes.empty()) {
            parserContext.m_errorMsg = "This is not a list instance.";
            _pass(context) = false;
            return;
        }

        if (ast.m_nodes.back().m_suffix.type() != typeid(listElement_)) {
            parserContext.m_errorMsg = "This is not a list instance.";
            _pass(context) = false;
        }
    }
};

struct leafListElementPath_class {
    template <typename T, typename Iterator, typename Context>
    void on_success(Iterator const&, Iterator const&, T& ast, Context const& context)
    {
        auto& parserContext = x3::get<parser_context_tag>(context);
        if (ast.m_nodes.empty() || ast.m_nodes.back().m_suffix.type() != typeid(leafListElement_)) {
            parserContext.m_errorMsg = "This is not a leaf list element.";
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

struct writable_leaf_path_class {
    template <typename T, typename Iterator, typename Context>
    void on_success(Iterator const&, Iterator const&, T&, Context const& context)
    {
        auto& parserContext = x3::get<parser_context_tag>(context);
        if (parserContext.currentSchemaPath().m_nodes.empty()) {
            parserContext.m_errorMsg = "This is not a path to leaf.";
            _pass(context) = false;
            return;
        }

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

struct describe_class;

struct help_class;

struct get_class;

struct copy_class;

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

std::set<Completion> generateMissingKeyCompletionSet(std::set<std::string> keysNeeded, std::map<std::string, leaf_data_> currentSet);

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
        if (!parserContext.m_completing) {
            return;
        }
        const auto& dataQuery = parserContext.m_dataquery;

        parserContext.m_completionIterator = begin;
        auto listInstances = dataQuery->listKeys(parserContext.currentDataPath(), {parserContext.m_curModule, parserContext.m_tmpListName});

        decltype(listInstances) filteredInstances;
        //This filters out instances, which don't correspond to the partial instance we have.
        const auto partialFitsComplete = [&parserContext] (const auto& complete) {
            const auto& partial = parserContext.m_tmpListKeys;
            return std::all_of(partial.begin(), partial.end(), [&complete] (const auto& oneKV) {
                    const auto& [k, v] = oneKV;
                    return complete.at(k) == v;
                    });
        };
        std::copy_if(listInstances.begin(), listInstances.end(), std::inserter(filteredInstances, filteredInstances.end()), partialFitsComplete);

        std::set<Completion> validValues;

        std::transform(filteredInstances.begin(), filteredInstances.end(), std::inserter(validValues, validValues.end()), [&parserContext](const auto& instance) {
            return Completion{leafDataToCompletion(instance.at(parserContext.m_tmpListKeyLeafPath.m_node.second))};
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
            parserContext.m_suggestions = {Completion{"]/"}};
        } else {
            parserContext.m_suggestions = {Completion{"]["}};
        }
    }
};

struct commandNamesVisitor {
    template <typename T>
    std::string operator()(boost::type<T>)
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
            parserContext.m_suggestions.insert({commandNamesVisitor()(cmd), " "});
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
