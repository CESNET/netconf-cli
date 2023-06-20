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

// Up to Boost 1.77, calling `_pass(context) = false` was enough.
// Since 1.78 it became necessary to explicitly reset the input iterator, but unfortunately the function signature
// was wrong and the whole handler would just be ignored.
// That was fixed in 1.80.
// FIXME: change the function signature and remove the const_cast once we're unconditionally on 1.80+
#define REJECT_PARSING do { const_cast<Iterator&>(after) = before; _pass(context) = false; } while (0)
#include "utils.hpp"
namespace x3 = boost::spirit::x3;

struct parser_context_tag;

struct keyValue_class {
    template <typename T, typename Iterator, typename Context>
    void on_success(Iterator const& before, Iterator const& after, T& ast, Context const& context)
    {
        auto& parserContext = x3::get<parser_context_tag>(context);

        if (parserContext.m_tmpListKeys.find(ast.first) != parserContext.m_tmpListKeys.end()) {
            REJECT_PARSING;
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

boost::optional<std::string> optModuleToOptString(const boost::optional<module_> module);

struct key_identifier_class {
    template <typename T, typename Iterator, typename Context>
    void on_success(Iterator const& before, Iterator const& after, T& ast, Context const& context)
    {
        ParserContext& parserContext = x3::get<parser_context_tag>(context);
        const Schema& schema = parserContext.m_schema;

        if (schema.listHasKey(dataPathToSchemaPath(parserContext.m_tmpListPath), ast)) {
            parserContext.m_tmpListKeyLeafPath.m_location = dataPathToSchemaPath(parserContext.m_tmpListPath);
            parserContext.m_tmpListKeyLeafPath.m_node = {optModuleToOptString(parserContext.m_tmpListPath.m_nodes.back().m_prefix), ast};
        } else {
            auto listName = std::get<list_>(parserContext.m_tmpListPath.m_nodes.back().m_suffix).m_name;
            parserContext.m_errorMsg = listName + " is not indexed by \"" + ast + "\".";
            REJECT_PARSING;
        }
    }
};

struct listSuffix_class {
    template <typename T, typename Iterator, typename Context>
    void on_success(Iterator const& before, Iterator const& after, T& ast, Context const& context)
    {
        auto& parserContext = x3::get<parser_context_tag>(context);
        const Schema& schema = parserContext.m_schema;

        const auto& keysNeeded = schema.listKeys(dataPathToSchemaPath(parserContext.m_tmpListPath));
        std::set<std::string> keysSupplied;
        for (const auto& it : ast) {
            keysSupplied.insert(it.first);
        }

        if (keysNeeded != keysSupplied) {
            auto listName = std::get<list_>(parserContext.m_tmpListPath.m_nodes.back().m_suffix).m_name;
            parserContext.m_errorMsg = "Not enough keys for " + listName + ". " +
                                       "These keys were not supplied:";
            std::set<std::string> missingKeys;
            std::set_difference(keysNeeded.begin(), keysNeeded.end(),
                                keysSupplied.begin(), keysSupplied.end(),
                                std::inserter(missingKeys, missingKeys.end()));

            for (const auto& it : missingKeys) {
                parserContext.m_errorMsg += " " + it;
            }
            parserContext.m_errorMsg += ".";

            REJECT_PARSING;
            return;
        }

        // Clean up after listSuffix, in case someone wants to parse more listSuffixes
        parserContext.m_tmpListKeys.clear();
    }

    template <typename Iterator, typename Exception, typename Context>
    x3::error_handler_result on_error(Iterator&, Iterator const&, Exception const&, Context const& context)
    {
        auto& parserContext = x3::get<parser_context_tag>(context);
        if (parserContext.m_errorMsg.empty()) {
            parserContext.m_errorMsg = "Expecting ']' here:";
        }
        return x3::error_handler_result::rethrow;
    }
};

struct module_class {
    template <typename T, typename Iterator, typename Context>
    void on_success(Iterator const& before, Iterator const& after, T& ast, Context const& context)
    {
        auto& parserContext = x3::get<parser_context_tag>(context);
        const auto& schema = parserContext.m_schema;

        if (!schema.isModule(ast.m_name)) {
            parserContext.m_errorMsg = "Invalid module name.";
            REJECT_PARSING;
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

struct cd_class {
    template <typename Iterator, typename Exception, typename Context>
    x3::error_handler_result on_error(Iterator&, Iterator const&, Exception const& x, Context const& context)
    {
        auto& parserContext = x3::get<parser_context_tag>(context);
        if (parserContext.m_errorMsg.empty()) {
            parserContext.m_errorMsg = "Expected " + x.which() + " here:";
        }
        return x3::error_handler_result::rethrow;
    }
};

struct presenceContainerPath_class {
    template <typename T, typename Iterator, typename Context>
    void on_success(Iterator const& before, Iterator const& after, T& ast, Context const& context)
    {
        auto& parserContext = x3::get<parser_context_tag>(context);
        const auto& schema = parserContext.m_schema;
        if (ast.m_nodes.empty()) {
            parserContext.m_errorMsg = "This container is not a presence container.";
            REJECT_PARSING;
            return;
        }

        if (schema.nodeType(pathToSchemaString(parserContext.currentSchemaPath(), Prefixes::Always)) != yang::NodeTypes::PresenceContainer) {
            parserContext.m_errorMsg = "This container is not a presence container.";
            REJECT_PARSING;
        }
    }
};

struct listInstancePath_class {
    template <typename T, typename Iterator, typename Context>
    void on_success(Iterator const& before, Iterator const& after, T& ast, Context const& context)
    {
        auto& parserContext = x3::get<parser_context_tag>(context);
        if (ast.m_nodes.empty() || !std::holds_alternative<listElement_>(ast.m_nodes.back().m_suffix)) {
            parserContext.m_errorMsg = "This is not a list instance.";
            REJECT_PARSING;
        }
    }
};

struct leafListElementPath_class {
    template <typename T, typename Iterator, typename Context>
    void on_success(Iterator const& before, Iterator const& after, T& ast, Context const& context)
    {
        auto& parserContext = x3::get<parser_context_tag>(context);
        if (ast.m_nodes.empty() || !std::holds_alternative<leafListElement_>(ast.m_nodes.back().m_suffix)) {
            parserContext.m_errorMsg = "This is not a leaf list element.";
            REJECT_PARSING;
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
        if (parserContext.m_errorMsg.empty()) {
            parserContext.m_errorMsg = "Couldn't parse create/delete command.";
        }
        return x3::error_handler_result::rethrow;
    }
};

struct delete_class {
    template <typename Iterator, typename Exception, typename Context>
    x3::error_handler_result on_error(Iterator&, Iterator const&, Exception const&, Context const& context)
    {
        auto& parserContext = x3::get<parser_context_tag>(context);
        if (parserContext.m_errorMsg.empty()) {
            parserContext.m_errorMsg = "Couldn't parse create/delete command.";
        }
        return x3::error_handler_result::rethrow;
    }
};

struct set_class {
    template <typename Iterator, typename Exception, typename Context>
    x3::error_handler_result on_error(Iterator&, Iterator const&, Exception const& x, Context const& context)
    {
        auto& parserContext = x3::get<parser_context_tag>(context);
        if (parserContext.m_errorMsg.empty()) {
            parserContext.m_errorMsg = "Expected " + x.which() + " here:";
        }
        return x3::error_handler_result::rethrow;
    }
};

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
        parserContext.m_tmpListPath = dataPath_{};
        parserContext.m_tmpListKeys.clear();
        parserContext.m_suggestions.clear();
    }
};

std::set<Completion> generateMissingKeyCompletionSet(std::set<std::string> keysNeeded, ListInstance currentSet);

struct createKeySuggestions_class {
    template <typename T, typename Iterator, typename Context>
    void on_success(Iterator const& before, Iterator const&, T&, Context const& context)
    {
        auto& parserContext = x3::get<parser_context_tag>(context);
        const auto& schema = parserContext.m_schema;

        parserContext.m_completionIterator = before;

        const auto& keysNeeded = schema.listKeys(dataPathToSchemaPath(parserContext.m_tmpListPath));
        parserContext.m_suggestions = generateMissingKeyCompletionSet(keysNeeded, parserContext.m_tmpListKeys);
    }
};

std::string leafDataToCompletion(const leaf_data_& value);

struct createValueSuggestions_class {
    template <typename T, typename Iterator, typename Context>
    void on_success(Iterator const& before, Iterator const&, T&, Context const& context)
    {
        auto& parserContext = x3::get<parser_context_tag>(context);
        if (!parserContext.m_completing) {
            return;
        }
        const auto& dataQuery = parserContext.m_dataquery;

        parserContext.m_completionIterator = before;
        auto listInstances = dataQuery->listKeys(parserContext.m_tmpListPath);

        decltype(listInstances) filteredInstances;
        //This filters out instances, which don't correspond to the partial instance we have.
        const auto partialFitsComplete = [&parserContext](const auto& complete) {
            const auto& partial = parserContext.m_tmpListKeys;
            return std::all_of(partial.begin(), partial.end(), [&complete](const auto& oneKV) {
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
    void on_success(Iterator const& before, Iterator const&, T&, Context const& context)
    {
        auto& parserContext = x3::get<parser_context_tag>(context);
        const auto& schema = parserContext.m_schema;

        parserContext.m_completionIterator = before;
        const auto& keysNeeded = schema.listKeys(dataPathToSchemaPath(parserContext.m_tmpListPath));
        if (generateMissingKeyCompletionSet(keysNeeded, parserContext.m_tmpListKeys).empty()) {
            parserContext.m_suggestions = {Completion{"]/"}};
        } else {
            parserContext.m_suggestions = {Completion{"]["}};
        }
    }
};

template <typename T>
std::string commandNamesVisitor(boost::type<T>)
{
    return T::name;
}

struct createCommandSuggestions_class {
    template <typename T, typename Iterator, typename Context>
    void on_success(Iterator const& before, Iterator const&, T&, Context const& context)
    {
        auto& parserContext = x3::get<parser_context_tag>(context);
        parserContext.m_completionIterator = before;

        parserContext.m_suggestions.clear();
        boost::mpl::for_each<CommandTypes, boost::type<boost::mpl::_>>([&parserContext](auto cmd) {
            parserContext.m_suggestions.insert({commandNamesVisitor(cmd), " "});
        });
    }
};

struct completing_class {
    template <typename T, typename Iterator, typename Context>
    void on_success(Iterator const& before, Iterator const& after, T&, Context const& context)
    {
        auto& parserContext = x3::get<parser_context_tag>(context);

        if (!parserContext.m_completing) {
            REJECT_PARSING;
        }
    }
};
