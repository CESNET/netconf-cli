/*
 * Copyright (C) 2020 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
*/

#pragma once

#include <boost/spirit/home/x3.hpp>
#include "ast_handlers.hpp"
#include "common_parsers.hpp"
#include "leaf_data_type.hpp"
#include "schema.hpp"
namespace x3 = boost::spirit::x3;

template <typename TYPE>
struct leaf_data_class;

template <typename TYPE>
struct createSetSuggestions_class {
    std::set<std::string> getSuggestions(ParserContext& ctx) const;

    template <typename T, typename Iterator, typename Context>
    void on_success(Iterator const& begin, Iterator const&, T&, Context const& context)
    {
        auto& parserContext = x3::get<parser_context_tag>(context);

        // Only generate completions if the type is correct so that we don't
        // overwrite some other completions.
        if (std::holds_alternative<TYPE>(parserContext.m_leafType)) {
            parserContext.m_completionIterator = begin;
            auto suggestions = getSuggestions(parserContext);
            std::set<Completion> res;
            std::transform(suggestions.begin(), suggestions.end(), std::inserter(res, res.end()), [](auto it) { return Completion{it}; });
            parserContext.m_suggestions = res;
        }
    }
};

template <>
struct leaf_data_class<yang::Enum> {
    template <typename T, typename Iterator, typename Context>
    void on_success(Iterator const&, Iterator const&, T& ast, Context const& context)
    {
        auto& parserContext = x3::get<parser_context_tag>(context);

        if (std::get<yang::Enum>(parserContext.m_leafType).m_allowedValues.count(ast) == 0) {
            _pass(context) = false;
            parserContext.m_errorMsg = "leaf data type mismatch: Expected an enum here. Allowed values:";
            for (const auto& it : std::get<yang::Enum>(parserContext.m_leafType).m_allowedValues) {
                parserContext.m_errorMsg += " " + it.m_value;
            }
        }
    }
};

template <>
struct leaf_data_class<yang::IdentityRef> {
    template <typename T, typename Iterator, typename Context>
    void on_success(Iterator const&, Iterator const&, T& ast, Context const& context)
    {
        ParserContext& parserContext = x3::get<parser_context_tag>(context);
        identityRef_ pair{ast};
        if (!pair.m_prefix) {
            pair.m_prefix = module_{parserContext.currentSchemaPath().m_nodes.front().m_prefix.get().m_name};
        }

        if (std::get<yang::IdentityRef>(parserContext.m_leafType).m_allowedValues.count(pair) == 0) {
            _pass(context) = false;
        }
    }
};

x3::rule<leaf_data_class<yang::Enum>, enum_> const leaf_data_enum = "leaf_data_enum";
x3::rule<leaf_data_class<yang::IdentityRef>, identityRef_> const leaf_data_identityRef = "leaf_data_identityRef";
x3::rule<struct leaf_data_class<yang::Binary>, binary_> const leaf_data_binary = "leaf_data_binary";
x3::rule<struct leaf_data_class<yang::Decimal>, double> const leaf_data_decimal = "leaf_data_decimal";
x3::rule<struct leaf_data_class<yang::String>, std::string> const leaf_data_string = "leaf_data_string";
x3::rule<struct leaf_data_class_binary, std::string> const leaf_data_binary_data = "leaf_data_binary_data";
x3::rule<struct leaf_data_identityRef_data_class, identityRef_> const leaf_data_identityRef_data = "leaf_data_identityRef_data";

x3::rule<createSetSuggestions_class<yang::Enum>, x3::unused_type> const createEnumSuggestions = "createEnumSuggestions";
x3::rule<createSetSuggestions_class<yang::IdentityRef>, x3::unused_type> const createIdentitySuggestions = "createIdentitySuggestions";

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

template <typename It, typename Ctx, typename RCtx, typename Attr>
struct impl_LeafData {
    It& first;
    It last;
    Ctx const& ctx;
    RCtx& rctx;
    Attr& attr;
    ParserContext& parserContext;

    bool operator()(const yang::Binary&) const
    {
        return leaf_data_binary.parse(first, last, ctx, rctx, attr);
    }
    bool operator()(const yang::Bool&) const
    {
        return bool_symbols.parse(first, last, ctx, rctx, attr);
    }
    bool operator()(const yang::Decimal&) const
    {
        return x3::double_.parse(first, last, ctx, rctx, attr);
    }
    bool operator()(const yang::Uint8&) const
    {
        return x3::uint8.parse(first, last, ctx, rctx, attr);
    }
    bool operator()(const yang::Uint16&) const
    {
        return x3::uint16.parse(first, last, ctx, rctx, attr);
    }
    bool operator()(const yang::Uint32&) const
    {
        return x3::uint32.parse(first, last, ctx, rctx, attr);
    }
    bool operator()(const yang::Uint64&) const
    {
        return x3::uint64.parse(first, last, ctx, rctx, attr);
    }
    bool operator()(const yang::Int8&) const
    {
        return x3::int8.parse(first, last, ctx, rctx, attr);
    }
    bool operator()(const yang::Int16&) const
    {
        return x3::int16.parse(first, last, ctx, rctx, attr);
    }
    bool operator()(const yang::Int32&) const
    {
        return x3::int32.parse(first, last, ctx, rctx, attr);
    }
    bool operator()(const yang::Int64&) const
    {
        return x3::int64.parse(first, last, ctx, rctx, attr);
    }
    bool operator()(const yang::String&) const
    {
        return leaf_data_string.parse(first, last, ctx, rctx, attr);
    }
    bool operator()(const yang::Enum& type) const
    {
        parserContext.m_leafType = type;
        return leaf_data_enum.parse(first, last, ctx, rctx, attr);
    }
    bool operator()(const yang::IdentityRef& type) const
    {
        parserContext.m_leafType = type;
        return leaf_data_identityRef.parse(first, last, ctx, rctx, attr);
    }
    bool operator()(const yang::LeafRef& leafRef) const
    {
        return std::visit(*this, parserContext.m_schema.leafType(leafRef.m_pointsTo));
    }
    bool operator()(const yang::Union& unionInfo) const
    {
        return std::any_of(unionInfo.m_unionTypes.begin(), unionInfo.m_unionTypes.end(), [this](const auto& type) {
            return std::visit(*this, type);
        });
    }
};

struct LeafData : x3::parser<LeafData> {
    using attribute_type = leaf_data_;

    // TODO: Can this be placed in a .cpp file?
    template <typename It, typename Ctx, typename RCtx, typename Attr>
    bool parse(It& first, It last, Ctx const& ctx, RCtx& rctx, Attr& attr) const
    {
        ParserContext& parserContext = x3::get<parser_context_tag>(ctx);
        const Schema& schema = parserContext.m_schema;
        auto type = schema.leafType(parserContext.m_tmpListKeyLeafPath.m_location, parserContext.m_tmpListKeyLeafPath.m_node);

        auto pass = std::visit(impl_LeafData<It, Ctx, RCtx, Attr>{first, last, ctx, rctx, attr, parserContext}, type);

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
