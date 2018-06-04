/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/

#pragma once

#include "parser_context.hpp"
#include "schema.hpp"

struct keyValue_class {
    template <typename T, typename Iterator, typename Context>
    void on_success(Iterator const&, Iterator const&, T& ast, Context const& context)
    {
        auto& parserContext = x3::get<parser_context_tag>(context);
        const Schema& schema = parserContext.m_schema;

        if (parserContext.m_tmpListKeys.find(ast.first) != parserContext.m_tmpListKeys.end()) {
            _pass(context) = false;
            parserContext.m_errorMsg = "Key \"" + ast.first + "\" was entered more than once.";
        } else if (schema.listHasKey(parserContext.m_curPath, parserContext.m_tmpListName, ast.first)) {
            parserContext.m_tmpListKeys.insert(ast.first);
        } else {
            _pass(context) = false;
            parserContext.m_errorMsg = parserContext.m_tmpListName + " is not indexed by \"" + ast.first + "\".";
        }
    }
};
struct identifier_class;

struct listPrefix_class {
    template <typename T, typename Iterator, typename Context>
    void on_success(Iterator const&, Iterator const&, T& ast, Context const& context)
    {
        auto& parserContext = x3::get<parser_context_tag>(context);
        const Schema& schema = parserContext.m_schema;

        if (schema.isList(parserContext.m_curPath, ast)) {
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

        const auto& keysNeeded = schema.listKeys(parserContext.m_curPath, parserContext.m_tmpListName);
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
};
struct listElement_class {
    template <typename T, typename Iterator, typename Context>
    void on_success(Iterator const&, Iterator const&, T& ast, Context const& context)
    {
        auto& parserContext = x3::get<parser_context_tag>(context);
        parserContext.m_curPath.m_nodes.push_back(ast);
    }

    template <typename Iterator, typename Exception, typename Context>
    x3::error_handler_result on_error(Iterator&, Iterator const&, Exception const& ex, Context const& context)
    {
        auto& parserContext = x3::get<parser_context_tag>(context);
        auto& error_handler = x3::get<x3::error_handler_tag>(context).get();
        if (parserContext.m_errorHandled) // someone already handled our error
            return x3::error_handler_result::fail;

        parserContext.m_errorHandled = true;

        std::string message = parserContext.m_errorMsg;
        error_handler(ex.where(), message);
        return x3::error_handler_result::fail;
    }
};

struct nodeup_class {
    template <typename T, typename Iterator, typename Context>
    void on_success(Iterator const&, Iterator const&, T&, Context const& context)
    {
        auto& parserContext = x3::get<parser_context_tag>(context);

        if (!parserContext.m_curPath.m_nodes.empty()) {
            parserContext.m_curPath.m_nodes.pop_back();
        } else {
            _pass(context) = false;
        }
    }
};

struct container_class {
    template <typename T, typename Iterator, typename Context>
    void on_success(Iterator const&, Iterator const&, T& ast, Context const& context)
    {
        auto& parserContext = x3::get<parser_context_tag>(context);
        const auto& schema = parserContext.m_schema;

        if (schema.isContainer(parserContext.m_curPath, ast.m_name)) {
            parserContext.m_curPath.m_nodes.push_back(ast);
        } else {
            _pass(context) = false;
        }
    }
};

struct leaf_class {
    template <typename T, typename Iterator, typename Context>
    void on_success(Iterator const&, Iterator const&, T& ast, Context const& context)
    {
        auto& parserContext = x3::get<parser_context_tag>(context);
        const auto& schema = parserContext.m_schema;

        if (schema.isLeaf(parserContext.m_curPath, ast.m_name)) {
            parserContext.m_curPath.m_nodes.push_back(ast);
        } else {
            _pass(context) = false;
        }
    }
};

struct path_class {
    template <typename T, typename Iterator, typename Context>
    void on_success(Iterator const&, Iterator const&, T&, Context const&)
    {
    }

    template <typename Iterator, typename Exception, typename Context>
    x3::error_handler_result on_error(Iterator&, Iterator const&, Exception const& x, Context const& context)
    {
        auto& parserContext = x3::get<parser_context_tag>(context);
        auto& error_handler = x3::get<x3::error_handler_tag>(context).get();
        std::string message = "invalid path.";
        if (parserContext.m_errorHandled) // someone already handled our error
            return x3::error_handler_result::fail;

        parserContext.m_errorHandled = true;
        error_handler(x.where(), message);
        return x3::error_handler_result::fail;
    }
};

struct data_string_class {
};

struct cd_class {
    template <typename T, typename Iterator, typename Context>
    void on_success(Iterator const&, Iterator const&, T&, Context const&)
    {
    }

    template <typename Iterator, typename Exception, typename Context>
    x3::error_handler_result on_error(Iterator&, Iterator const&, Exception const& x, Context const& context)
    {
        auto& parserContext = x3::get<parser_context_tag>(context);
        auto& error_handler = x3::get<x3::error_handler_tag>(context).get();
        std::string message = "This isn't a list or a container or nothing.";
        if (parserContext.m_errorHandled) // someone already handled our error
            return x3::error_handler_result::fail;

        parserContext.m_errorHandled = true;
        error_handler(x.where(), message);
        return x3::error_handler_result::fail;
    }
};

struct presenceContainerPathHandler {
    template <typename T, typename Iterator, typename Context>
    void on_success(Iterator const&, Iterator const&, T& ast, Context const& context)
    {
        auto& parserContext = x3::get<parser_context_tag>(context);
        const auto& schema = parserContext.m_schema;
        try {
            container_ cont = boost::get<container_>(ast.m_path.m_nodes.back());
            path_ location{decltype(path_::m_nodes)(parserContext.m_curPath.m_nodes.begin(),
                                                    parserContext.m_curPath.m_nodes.end() - 1)};

            if (!schema.isPresenceContainer(location, cont.m_name)) {
                _pass(context) = false;
                return;
            }
        } catch (boost::bad_get&) {
            _pass(context) = false;
            return;
        }
    }

    template <typename Iterator, typename Exception, typename Context>
    x3::error_handler_result on_error(Iterator&, Iterator const&, Exception const& x, Context const& context)
    {
        auto& parserContext = x3::get<parser_context_tag>(context);
        auto& error_handler = x3::get<x3::error_handler_tag>(context).get();
        std::string message = "This isn't a path to a presence container.";
        if (parserContext.m_errorHandled) // someone already handled our error
            return x3::error_handler_result::fail;

        parserContext.m_errorHandled = true;
        error_handler(x.where(), message);
        return x3::error_handler_result::fail;
    }
};

struct create_class : public presenceContainerPathHandler {
};

struct delete_class : public presenceContainerPathHandler {
};

struct set_class {
    template <typename T, typename Iterator, typename Context>
    void on_success(Iterator const&, Iterator const&, T& ast, Context const& context)
    {
        try {
            auto leaf = boost::get<leaf_>(ast.m_path.m_nodes.back());
        } catch (boost::bad_get&) {
            _pass(context) = false;
            return;
        }
    }

    template <typename Iterator, typename Exception, typename Context>
    x3::error_handler_result on_error(Iterator&, Iterator const&, Exception const& x, Context const& context)
    {
        auto& parserContext = x3::get<parser_context_tag>(context);
        auto& error_handler = x3::get<x3::error_handler_tag>(context).get();
        std::string message = "This isn't a path to leaf.";
        if (parserContext.m_errorHandled) // someone already handled our error
            return x3::error_handler_result::fail;

        parserContext.m_errorHandled = true;
        error_handler(x.where(), message);
        return x3::error_handler_result::fail;
    }
};

struct command_class {
    template <typename Iterator, typename Exception, typename Context>
    x3::error_handler_result on_error(Iterator&, Iterator const&, Exception const& x, Context const& context)
    {
        auto& parserContext = x3::get<parser_context_tag>(context);
        auto& error_handler = x3::get<x3::error_handler_tag>(context).get();
        std::string message = "Couldn't parse command.";
        if (parserContext.m_errorHandled) // someone already handled our error
            return x3::error_handler_result::fail;

        parserContext.m_errorHandled = true;
        error_handler(x.where(), message);
        return x3::error_handler_result::fail;
    }
};
