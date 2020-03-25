/*
 * Copyright (C) 2020 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
*/

#pragma once

#include <boost/spirit/home/x3.hpp>
#include "ast_values.hpp"
#include "ast_handlers.hpp"
#include "common_parsers.hpp"
#include "schema.hpp"
namespace x3 = boost::spirit::x3;

template <yang::LeafDataTypes TYPE>
struct leaf_data_class;

template <yang::LeafDataTypes TYPE>
struct createSetSuggestions_class {
    std::set<std::string> getSuggestions(const ParserContext& ctx, const Schema& schema) const;

    template <typename T, typename Iterator, typename Context>
    void on_success(Iterator const& begin, Iterator const&, T&, Context const& context)
    {
        auto& parserContext = x3::get<parser_context_tag>(context);
        const Schema& schema = parserContext.m_schema;

        // Only generate completions if the type is correct so that we don't
        // overwrite some other completions.
        if (schema.leafType(parserContext.m_tmpListKeyLeafPath.m_location, parserContext.m_tmpListKeyLeafPath.m_node) == TYPE) {
            parserContext.m_completionIterator = begin;
            auto suggestions = getSuggestions(parserContext, schema);
            std::set<Completion> res;
            std::transform(suggestions.begin(), suggestions.end(), std::inserter(res, res.end()), [](auto it) { return Completion{it}; });
            parserContext.m_suggestions = res;
        }
    }
};

template <>
struct leaf_data_class<yang::LeafDataTypes::Enum> {
    template <typename T, typename Iterator, typename Context>
    void on_success(Iterator const&, Iterator const&, T& ast, Context const& context)
    {
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

template <>
struct leaf_data_class<yang::LeafDataTypes::IdentityRef> {
    template <typename T, typename Iterator, typename Context>
    void on_success(Iterator const&, Iterator const&, T& ast, Context const& context)
    {
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

x3::rule<leaf_data_class<yang::LeafDataTypes::Enum>, enum_> const leaf_data_enum = "leaf_data_enum";
x3::rule<leaf_data_class<yang::LeafDataTypes::IdentityRef>, identityRef_> const leaf_data_identityRef = "leaf_data_identityRef";
x3::rule<struct leaf_data_class<yang::LeafDataTypes::Binary>, binary_> const leaf_data_binary = "leaf_data_binary";
x3::rule<struct leaf_data_class<yang::LeafDataTypes::Decimal>, double> const leaf_data_decimal = "leaf_data_decimal";
x3::rule<struct leaf_data_class<yang::LeafDataTypes::String>, std::string> const leaf_data_string = "leaf_data_string";
x3::rule<struct leaf_data_class_binary, std::string> const leaf_data_binary_data = "leaf_data_binary_data";
x3::rule<struct leaf_data_identityRef_data_class, identityRef_> const leaf_data_identityRef_data = "leaf_data_identityRef_data";

x3::rule<createSetSuggestions_class<yang::LeafDataTypes::Enum>, x3::unused_type> const createEnumSuggestions = "createEnumSuggestions";
x3::rule<createSetSuggestions_class<yang::LeafDataTypes::IdentityRef>, x3::unused_type> const createIdentitySuggestions = "createIdentitySuggestions";

using x3::char_;

auto const createEnumSuggestions_def =
    x3::eps;

auto const leaf_data_enum_def =
    createEnumSuggestions >> +char_;

struct bool_symbol_table : x3::symbols<bool> {
    bool_symbol_table()
    {
    add
        ("true", true)
        ("false", false);
    }
} const bool_symbols;

auto const leaf_data_string_def =
    '\'' >> *(char_-'\'') >> '\'' |
    '\"' >> *(char_-'\"') >> '\"';

// This intermediate rule is neccessary for coercing to std::string.
// TODO: check if I can do the coercing right in the grammar with `as{}` from
// https://github.com/boostorg/spirit/issues/530#issuecomment-584836532
// This would shave off some more lines.
auto const leaf_data_binary_data_def =
    +(x3::alnum | char_('+') | char_('/')) >> -char_('=') >> -char_('=');

auto const leaf_data_binary_def =
    leaf_data_binary_data;

auto const leaf_data_identityRef_data_def =
    -module  >> node_identifier;

auto const createIdentitySuggestions_def =
    x3::eps;

auto const leaf_data_identityRef_def =
    createIdentitySuggestions >> leaf_data_identityRef_data;

struct LeafData : x3::parser<LeafData> {
    using attribute_type = leaf_data_;

    // TODO: Can this be placed in a .cpp file?
    template <typename It, typename Ctx, typename RCtx, typename Attr>
    bool parse(It& first, It last, Ctx const& ctx, RCtx& rctx, Attr& attr) const
    {
        ParserContext& parserContext = x3::get<parser_context_tag>(ctx);
        const Schema& schema = parserContext.m_schema;
        auto type = schema.leafType(parserContext.m_tmpListKeyLeafPath.m_location, parserContext.m_tmpListKeyLeafPath.m_node);

        std::function<bool(yang::LeafDataTypes)> parse_impl = [&](auto type) {
            switch (type) {
            case yang::LeafDataTypes::Binary:
                return leaf_data_binary.parse(first, last, ctx, rctx, attr);
            case yang::LeafDataTypes::Bool:
                return bool_symbols.parse(first, last, ctx, rctx, attr);
            case yang::LeafDataTypes::Decimal:
                return x3::double_.parse(first, last, ctx, rctx, attr);
            case yang::LeafDataTypes::Uint8:
                return x3::uint8.parse(first, last, ctx, rctx, attr);
            case yang::LeafDataTypes::Uint16:
                return x3::uint16.parse(first, last, ctx, rctx, attr);
            case yang::LeafDataTypes::Uint32:
                return x3::uint32.parse(first, last, ctx, rctx, attr);
            case yang::LeafDataTypes::Uint64:
                return x3::uint64.parse(first, last, ctx, rctx, attr);
            case yang::LeafDataTypes::Int8:
                return x3::int8.parse(first, last, ctx, rctx, attr);
            case yang::LeafDataTypes::Int16:
                return x3::int16.parse(first, last, ctx, rctx, attr);
            case yang::LeafDataTypes::Int32:
                return x3::int32.parse(first, last, ctx, rctx, attr);
            case yang::LeafDataTypes::Int64:
                return x3::int64.parse(first, last, ctx, rctx, attr);
            case yang::LeafDataTypes::String:
                return leaf_data_string.parse(first, last, ctx, rctx, attr);
            case yang::LeafDataTypes::Enum:
                return leaf_data_enum.parse(first, last, ctx, rctx, attr);
            case yang::LeafDataTypes::IdentityRef:
                return leaf_data_identityRef.parse(first, last, ctx, rctx, attr);
            case yang::LeafDataTypes::LeafRef:
                auto actualType = schema.leafrefBaseType(parserContext.m_tmpListKeyLeafPath.m_location, parserContext.m_tmpListKeyLeafPath.m_node);
                return parse_impl(actualType);
            }
            __builtin_unreachable();
        };
        auto pass = parse_impl(type);

        if (!pass) {
            if (parserContext.m_errorMsg.empty()) {
                parserContext.m_errorMsg = "leaf data type mismatch: Expected " +
                    leafDataTypeToString(schema.leafType(parserContext.m_tmpListKeyLeafPath.m_location, parserContext.m_tmpListKeyLeafPath.m_node)) + " here:";
            }
        }
        return pass;
    }
};

auto const leaf_data = x3::no_skip[std::move(LeafData())];

BOOST_SPIRIT_DEFINE(leaf_data_enum)
BOOST_SPIRIT_DEFINE(leaf_data_string)
BOOST_SPIRIT_DEFINE(leaf_data_binary_data)
BOOST_SPIRIT_DEFINE(leaf_data_binary)
BOOST_SPIRIT_DEFINE(leaf_data_identityRef_data)
BOOST_SPIRIT_DEFINE(leaf_data_identityRef)
BOOST_SPIRIT_DEFINE(createEnumSuggestions)
BOOST_SPIRIT_DEFINE(createIdentitySuggestions)
