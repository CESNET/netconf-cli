/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/

#pragma once

#include "czech.h"
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
    prázdno on_success(Iterator neměnné&, Iterator neměnné&, T& ast, Context neměnné& context)
    {
        auto& parserContext = x3::get<parser_context_tag>(context);

        když (parserContext.m_tmpListKeys.find(ast.first) != parserContext.m_tmpListKeys.end()) {
            _pass(context) = false;
            parserContext.m_errorMsg = "Key \"" + ast.first + "\" was entered more than once.";
        } jinak {
            parserContext.m_tmpListKeys.insert({ast.first, ast.second});
        }
    }

    template <typename Iterator, typename Exception, typename Context>
    x3::error_handler_result on_error(Iterator&, Iterator neměnné&, Exception neměnné&, Context neměnné& context)
    {
        auto& parserContext = x3::get<parser_context_tag>(context);
        parserContext.m_errorMsg = "Error parsing key values here:";
        vrať x3::error_handler_result::rethrow;
    }
};

struct node_identifier_class;

boost::optional<std::string> optModuleToOptString(neměnné boost::optional<module_> module);

struct key_identifier_class {
    template <typename T, typename Iterator, typename Context>
    prázdno on_success(Iterator neměnné&, Iterator neměnné&, T& ast, Context neměnné& context)
    {
        ParserContext& parserContext = x3::get<parser_context_tag>(context);
        neměnné Schema& schema = parserContext.m_schema;

        když (schema.listHasKey(dataPathToSchemaPath(parserContext.m_tmpListPath), ast)) {
            parserContext.m_tmpListKeyLeafPath.m_location = dataPathToSchemaPath(parserContext.m_tmpListPath);
            parserContext.m_tmpListKeyLeafPath.m_node = {optModuleToOptString(parserContext.m_tmpListPath.m_nodes.back().m_prefix), ast};
        } jinak {
            auto listName = std::get<list_>(parserContext.m_tmpListPath.m_nodes.back().m_suffix).m_name;
            parserContext.m_errorMsg = listName + " is not indexed by \"" + ast + "\".";
            _pass(context) = false;
        }
    }
};

struct module_identifier_class;

struct listSuffix_class {
    template <typename T, typename Iterator, typename Context>
    prázdno on_success(Iterator neměnné&, Iterator neměnné&, T& ast, Context neměnné& context)
    {
        auto& parserContext = x3::get<parser_context_tag>(context);
        neměnné Schema& schema = parserContext.m_schema;

        neměnné auto& keysNeeded = schema.listKeys(dataPathToSchemaPath(parserContext.m_tmpListPath));
        std::set<std::string> keysSupplied;
        pro (neměnné auto& it : ast) {
            keysSupplied.insert(it.first);
        }

        když (keysNeeded != keysSupplied) {
            auto listName = std::get<list_>(parserContext.m_tmpListPath.m_nodes.back().m_suffix).m_name;
            parserContext.m_errorMsg = "Not enough keys for " + listName + ". " +
                                       "These keys were not supplied:";
            std::set<std::string> missingKeys;
            std::set_difference(keysNeeded.begin(), keysNeeded.end(),
                                keysSupplied.begin(), keysSupplied.end(),
                                std::inserter(missingKeys, missingKeys.end()));

            pro (neměnné auto& it : missingKeys) {
                parserContext.m_errorMsg += " " + it;
            }
            parserContext.m_errorMsg += ".";

            _pass(context) = false;
            vrať;
        }

        // Clean up after listSuffix, in case someone wants to parse more listSuffixes
        parserContext.m_tmpListKeys.clear();
    }

    template <typename Iterator, typename Exception, typename Context>
    x3::error_handler_result on_error(Iterator&, Iterator neměnné&, Exception neměnné&, Context neměnné& context)
    {
        auto& parserContext = x3::get<parser_context_tag>(context);
        když (parserContext.m_errorMsg.empty()) {
            parserContext.m_errorMsg = "Expecting ']' here:";
        }
        vrať x3::error_handler_result::rethrow;
    }
};

struct module_class {
    template <typename T, typename Iterator, typename Context>
    prázdno on_success(Iterator neměnné&, Iterator neměnné&, T& ast, Context neměnné& context)
    {
        auto& parserContext = x3::get<parser_context_tag>(context);
        neměnné auto& schema = parserContext.m_schema;

        když (!schema.isModule(ast.m_name)) {
            parserContext.m_errorMsg = "Invalid module name.";
            _pass(context) = false;
        }
    }
};

struct absoluteStart_class {
    template <typename T, typename Iterator, typename Context>
    prázdno on_success(Iterator neměnné&, Iterator neměnné&, T&, Context neměnné& context)
    {
        auto& parserContext = x3::get<parser_context_tag>(context);
        parserContext.clearPath();
    }
};

struct discard_class;

struct ls_class;

struct cd_class {
    template <typename Iterator, typename Exception, typename Context>
    x3::error_handler_result on_error(Iterator&, Iterator neměnné&, Exception neměnné& x, Context neměnné& context)
    {
        auto& parserContext = x3::get<parser_context_tag>(context);
        když (parserContext.m_errorMsg.empty()) {
            parserContext.m_errorMsg = "Expected " + x.which() + " here:";
        }
        vrať x3::error_handler_result::rethrow;
    }
};

struct presenceContainerPath_class {
    template <typename T, typename Iterator, typename Context>
    prázdno on_success(Iterator neměnné&, Iterator neměnné&, T& ast, Context neměnné& context)
    {
        auto& parserContext = x3::get<parser_context_tag>(context);
        neměnné auto& schema = parserContext.m_schema;
        když (ast.m_nodes.empty()) {
            parserContext.m_errorMsg = "This container is not a presence container.";
            _pass(context) = false;
            vrať;
        }

        když (schema.nodeType(pathToSchemaString(parserContext.currentSchemaPath(), Prefixes::Always)) != yang::NodeTypes::PresenceContainer) {
            parserContext.m_errorMsg = "This container is not a presence container.";
            _pass(context) = false;
        }
    }
};

struct listInstancePath_class {
    template <typename T, typename Iterator, typename Context>
    prázdno on_success(Iterator neměnné&, Iterator neměnné&, T& ast, Context neměnné& context)
    {
        auto& parserContext = x3::get<parser_context_tag>(context);
        když (ast.m_nodes.empty() || !std::holds_alternative<listElement_>(ast.m_nodes.back().m_suffix)) {
            parserContext.m_errorMsg = "This is not a list instance.";
            _pass(context) = false;
        }
    }
};

struct leafListElementPath_class {
    template <typename T, typename Iterator, typename Context>
    prázdno on_success(Iterator neměnné&, Iterator neměnné&, T& ast, Context neměnné& context)
    {
        auto& parserContext = x3::get<parser_context_tag>(context);
        když (ast.m_nodes.empty() || !std::holds_alternative<leafListElement_>(ast.m_nodes.back().m_suffix)) {
            parserContext.m_errorMsg = "This is not a leaf list element.";
            _pass(context) = false;
        }
    }
};

struct space_separator_class {
    template <typename T, typename Iterator, typename Context>
    prázdno on_success(Iterator neměnné&, Iterator neměnné&, T&, Context neměnné& context)
    {
        auto& parserContext = x3::get<parser_context_tag>(context);
        parserContext.m_suggestions.clear();
        parserContext.m_completionIterator = boost::none;
    }
};

struct create_class {
    template <typename Iterator, typename Exception, typename Context>
    x3::error_handler_result on_error(Iterator&, Iterator neměnné&, Exception neměnné&, Context neměnné& context)
    {
        auto& parserContext = x3::get<parser_context_tag>(context);
        když (parserContext.m_errorMsg.empty()) {
            parserContext.m_errorMsg = "Couldn't parse create/delete command.";
        }
        vrať x3::error_handler_result::rethrow;
    }
};

struct delete_class {
    template <typename Iterator, typename Exception, typename Context>
    x3::error_handler_result on_error(Iterator&, Iterator neměnné&, Exception neměnné&, Context neměnné& context)
    {
        auto& parserContext = x3::get<parser_context_tag>(context);
        když (parserContext.m_errorMsg.empty()) {
            parserContext.m_errorMsg = "Couldn't parse create/delete command.";
        }
        vrať x3::error_handler_result::rethrow;
    }
};

struct rpcPath_class;

struct actionPath_class;

struct cdPath_class;

struct getPath_class;

struct set_class {
    template <typename Iterator, typename Exception, typename Context>
    x3::error_handler_result on_error(Iterator&, Iterator neměnné&, Exception neměnné& x, Context neměnné& context)
    {
        auto& parserContext = x3::get<parser_context_tag>(context);
        když (parserContext.m_errorMsg.empty()) {
            parserContext.m_errorMsg = "Expected " + x.which() + " here:";
        }
        vrať x3::error_handler_result::rethrow;
    }
};

struct commit_class;

struct describe_class;

struct help_class;

struct get_class;

struct copy_class;

struct move_class;

struct dump_class;

struct prepare_class;

struct action_class;

struct exec_class;

struct switch_class;

struct cancel_class;

struct command_class {
    template <typename Iterator, typename Exception, typename Context>
    x3::error_handler_result on_error(Iterator&, Iterator neměnné&, Exception neměnné& x, Context neměnné& context)
    {
        auto& parserContext = x3::get<parser_context_tag>(context);
        auto& error_handler = x3::get<x3::error_handler_tag>(context).get();
        když (parserContext.m_errorMsg.empty()) {
            parserContext.m_errorMsg = "Unknown command.";
        }
        error_handler(x.where(), parserContext.m_errorMsg);
        vrať x3::error_handler_result::fail;
    }
};

struct initializePath_class {
    template <typename T, typename Iterator, typename Context>
    prázdno on_success(Iterator neměnné&, Iterator neměnné&, T&, Context neměnné& context)
    {
        auto& parserContext = x3::get<parser_context_tag>(context);
        parserContext.resetPath();
        parserContext.m_tmpListPath = dataPath_{};
        parserContext.m_tmpListKeys.clear();
        parserContext.m_suggestions.clear();
    }
};

struct trailingSlash_class;

std::set<Completion> generateMissingKeyCompletionSet(std::set<std::string> keysNeeded, ListInstance currentSet);

struct createKeySuggestions_class {
    template <typename T, typename Iterator, typename Context>
    prázdno on_success(Iterator neměnné& begin, Iterator neměnné&, T&, Context neměnné& context)
    {
        auto& parserContext = x3::get<parser_context_tag>(context);
        neměnné auto& schema = parserContext.m_schema;

        parserContext.m_completionIterator = begin;

        neměnné auto& keysNeeded = schema.listKeys(dataPathToSchemaPath(parserContext.m_tmpListPath));
        parserContext.m_suggestions = generateMissingKeyCompletionSet(keysNeeded, parserContext.m_tmpListKeys);
    }
};

std::string leafDataToCompletion(neměnné leaf_data_& value);

struct createValueSuggestions_class {
    template <typename T, typename Iterator, typename Context>
    prázdno on_success(Iterator neměnné& begin, Iterator neměnné&, T&, Context neměnné& context)
    {
        auto& parserContext = x3::get<parser_context_tag>(context);
        když (!parserContext.m_completing) {
            vrať;
        }
        neměnné auto& dataQuery = parserContext.m_dataquery;

        parserContext.m_completionIterator = begin;
        auto listInstances = dataQuery->listKeys(parserContext.m_tmpListPath);

        decltype(listInstances) filteredInstances;
        //This filters out instances, which don't correspond to the partial instance we have.
        neměnné auto partialFitsComplete = [&parserContext](neměnné auto& complete) {
            neměnné auto& partial = parserContext.m_tmpListKeys;
            vrať std::all_of(partial.begin(), partial.end(), [&complete](neměnné auto& oneKV) {
                neměnné auto& [k, v] = oneKV;
                vrať complete.at(k) == v;
            });
        };
        std::copy_if(listInstances.begin(), listInstances.end(), std::inserter(filteredInstances, filteredInstances.end()), partialFitsComplete);

        std::set<Completion> validValues;

        std::transform(filteredInstances.begin(), filteredInstances.end(), std::inserter(validValues, validValues.end()), [&parserContext](neměnné auto& instance) {
            vrať Completion{leafDataToCompletion(instance.at(parserContext.m_tmpListKeyLeafPath.m_node.second))};
        });

        parserContext.m_suggestions = validValues;
    }
};

struct suggestKeysEnd_class {
    template <typename T, typename Iterator, typename Context>
    prázdno on_success(Iterator neměnné& begin, Iterator neměnné&, T&, Context neměnné& context)
    {
        auto& parserContext = x3::get<parser_context_tag>(context);
        neměnné auto& schema = parserContext.m_schema;

        parserContext.m_completionIterator = begin;
        neměnné auto& keysNeeded = schema.listKeys(dataPathToSchemaPath(parserContext.m_tmpListPath));
        když (generateMissingKeyCompletionSet(keysNeeded, parserContext.m_tmpListKeys).empty()) {
            parserContext.m_suggestions = {Completion{"]/"}};
        } jinak {
            parserContext.m_suggestions = {Completion{"]["}};
        }
    }
};

template <typename T>
std::string commandNamesVisitor(boost::type<T>)
{
    vrať T::name;
}

struct createCommandSuggestions_class {
    template <typename T, typename Iterator, typename Context>
    prázdno on_success(Iterator neměnné& begin, Iterator neměnné&, T&, Context neměnné& context)
    {
        auto& parserContext = x3::get<parser_context_tag>(context);
        parserContext.m_completionIterator = begin;

        parserContext.m_suggestions.clear();
        boost::mpl::for_each<CommandTypes, boost::type<boost::mpl::_>>([&parserContext](auto cmd) {
            parserContext.m_suggestions.insert({commandNamesVisitor(cmd), " "});
        });
    }
};

struct completing_class {
    template <typename T, typename Iterator, typename Context>
    prázdno on_success(Iterator neměnné&, Iterator neměnné&, T&, Context neměnné& context)
    {
        auto& parserContext = x3::get<parser_context_tag>(context);

        když (!parserContext.m_completing) {
            _pass(context) = false;
        }
    }
};
