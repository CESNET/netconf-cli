/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/

#pragma once
#include "CTree.hpp"

class InvalidKeyException : public std::invalid_argument {
public:
    using std::invalid_argument::invalid_argument;
    ~InvalidKeyException() override;
};

struct ParserContext {
    ParserContext(const CTree& tree);
    const CTree& m_tree;
    std::string m_curPath;
    std::string m_errorMsg;
    std::string m_tmpListName;
    std::set<std::string> m_tmpListKeys;
    bool m_errorHandled = false;
};

struct keyValue_class {
    template <typename T, typename Iterator, typename Context>
    void on_success(Iterator const&, Iterator const&, T& ast, Context const& context)
    {
        auto& parserContext = x3::get<parser_context_tag>(context);
        const CTree& tree = parserContext.m_tree;

        if (parserContext.m_tmpListKeys.find(ast.first) != parserContext.m_tmpListKeys.end()) {
            _pass(context) = false;
            parserContext.m_errorMsg = "Key \"" + ast.first + "\" was entered more than once.";
        } else if (tree.listHasKey(parserContext.m_curPath, parserContext.m_tmpListName, ast.first)) {
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
        const CTree& tree = parserContext.m_tree;

        if (tree.isList(parserContext.m_curPath, ast)) {
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
        const CTree& tree = parserContext.m_tree;

        const auto& keysNeeded = tree.listKeys(parserContext.m_curPath, parserContext.m_tmpListName);
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
        parserContext.m_curPath = joinPaths(parserContext.m_curPath, ast.m_name);
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

struct container_class {
    template <typename T, typename Iterator, typename Context>
    void on_success(Iterator const&, Iterator const&, T& ast, Context const& context)
    {
        auto& parserContext = x3::get<parser_context_tag>(context);
        const auto& tree = parserContext.m_tree;

        if (tree.isContainer(parserContext.m_curPath, ast.m_name)) {
            parserContext.m_curPath = joinPaths(parserContext.m_curPath, ast.m_name);
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
