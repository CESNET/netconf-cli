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
#include "utils.hpp"

struct keyValue_class {
    template <typename T, typename Iterator, typename Context>
    void on_success(Iterator const&, Iterator const&, T& ast, Context const& context)
    {
        auto& parserContext = x3::get<parser_context_tag>(context);
        const Schema& schema = parserContext.m_schema;

        if (parserContext.m_tmpListKeys.find(ast.first) != parserContext.m_tmpListKeys.end()) {
            _pass(context) = false;
            parserContext.m_errorMsg = "Key \"" + ast.first + "\" was entered more than once.";
        } else if (!schema.listHasKey(parserContext.m_curPath, parserContext.m_tmpListName, ast.first)) {
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
    template <typename T, typename Iterator, typename Context>
    void on_success(Iterator const&, Iterator const&, T& ast, Context const& context)
    {
        auto& parserContext = x3::get<parser_context_tag>(context);
        parserContext.m_curPath.m_nodes.push_back(ast);
    }

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

struct cd_class {
    template <typename T, typename Iterator, typename Context>
    void on_success(Iterator const&, Iterator const&, T&, Context const&)
    {
    }

    template <typename Iterator, typename Exception, typename Context>
    x3::error_handler_result on_error(Iterator&, Iterator const&, Exception const& x, Context const& context)
    {
        auto& parserContext = x3::get<parser_context_tag>(context);
        if (parserContext.m_errorMsg.empty())
            parserContext.m_errorMsg = "Expected " + x.which() + " here:";
        return x3::error_handler_result::rethrow;
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
            path_ location = pathWithoutLastNode(parserContext.m_curPath);

            if (!schema.isPresenceContainer(location, cont.m_name)) {
                parserContext.m_errorMsg = "This container is not a presence container.";
                _pass(context) = false;
            }
        } catch (boost::bad_get&) {
            parserContext.m_errorMsg = "This is not a container.";
            _pass(context) = false;
        }
    }

    template <typename Iterator, typename Exception, typename Context>
    x3::error_handler_result on_error(Iterator&, Iterator const&, Exception const&, Context const& context)
    {
        auto& parserContext = x3::get<parser_context_tag>(context);
        if (parserContext.m_errorMsg.empty())
            parserContext.m_errorMsg = "Couldn't parse create/delete command.";
        return x3::error_handler_result::rethrow;
    }
};

struct create_class : public presenceContainerPathHandler {
};

struct delete_class : public presenceContainerPathHandler {
};

struct leaf_path_class {
    template <typename T, typename Iterator, typename Context>
    void on_success(Iterator const&, Iterator const&, T& ast, Context const& context)
    {
        auto& parserContext = x3::get<parser_context_tag>(context);
        try {
            auto leaf = boost::get<leaf_>(ast.m_nodes.back());
        } catch (boost::bad_get&) {
            parserContext.m_errorMsg = "This is not a path to leaf.";
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
            leaf_ leaf = boost::get<leaf_>(parserContext.m_curPath.m_nodes.back());
            path_ location = pathWithoutLastNode(parserContext.m_curPath);
            parserContext.m_errorMsg = "Expected " + leafDataTypeToString(schema.leafType(location, leaf.m_name)) + " here:";
            return x3::error_handler_result::fail;
        }
        return x3::error_handler_result::rethrow;
    }
};

struct leaf_data_base_class {
    yang::LeafDataTypes m_type;

    leaf_data_base_class(yang::LeafDataTypes type)
        : m_type(type)
    {
    }

    template <typename T, typename Iterator, typename Context>
    void on_success(Iterator const&, Iterator const&, T&, Context const& context)
    {
        auto& parserContext = x3::get<parser_context_tag>(context);
        auto& schema = parserContext.m_schema;

        leaf_ leaf = boost::get<leaf_>(parserContext.m_curPath.m_nodes.back());
        path_ location = pathWithoutLastNode(parserContext.m_curPath);

        if (schema.leafType(location, leaf.m_name) != m_type) {
            _pass(context) = false;
        }
    }
};

struct leaf_data_enum_class : leaf_data_base_class {
    leaf_data_enum_class()
        : leaf_data_base_class(yang::LeafDataTypes::Enum)
    {
    }

    template <typename T, typename Iterator, typename Context>
    void on_success(Iterator const& start, Iterator const& end, T& ast, Context const& context)
    {
        leaf_data_base_class::on_success(start, end, ast, context);
        auto& parserContext = x3::get<parser_context_tag>(context);
        auto& schema = parserContext.m_schema;

        leaf_ leaf = boost::get<leaf_>(parserContext.m_curPath.m_nodes.back());
        path_ location = pathWithoutLastNode(parserContext.m_curPath);

        if (!schema.leafEnumHasValue(location, leaf.m_name, ast.m_value)) {
            _pass(context) = false;
        }
    }
};

struct leaf_data_decimal_class : leaf_data_base_class {
    leaf_data_decimal_class()
        : leaf_data_base_class(yang::LeafDataTypes::Decimal)
    {
    }
};

struct leaf_data_bool_class : leaf_data_base_class {
    leaf_data_bool_class()
        : leaf_data_base_class(yang::LeafDataTypes::Bool)
    {
    }
};

struct leaf_data_int_class : leaf_data_base_class {
    leaf_data_int_class()
        : leaf_data_base_class(yang::LeafDataTypes::Int)
    {
    }
};

struct leaf_data_uint_class : leaf_data_base_class {
    leaf_data_uint_class()
        : leaf_data_base_class(yang::LeafDataTypes::Uint)
    {
    }
};

struct leaf_data_string_class : leaf_data_base_class {
    leaf_data_string_class()
        : leaf_data_base_class(yang::LeafDataTypes::String)
    {
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
