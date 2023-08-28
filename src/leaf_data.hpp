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

using x3::char_;

struct bool_symbol_table : x3::symbols<bool> {
    bool_symbol_table()
    {
        add
            ("true", true)
            ("false", false);
    }
} const bool_symbols;

auto const leaf_data_string = x3::rule<struct leaf_data_class<yang::String>, std::string>{"leaf_data_string"} =
    '\'' >> *(char_-'\'') >> '\'' |
    '\"' >> *(char_-'\"') >> '\"';

auto const leaf_data_binary = x3::rule<struct leaf_data_class<yang::Binary>, binary_>{"leaf_data_binary"} =
    as<std::string>[+(x3::alnum | char_('+') | char_('/')) >> -char_('=') >> -char_('=')];

auto const leaf_data_identityRef = x3::rule<struct leaf_data_class<yang::IdentityRef>, identityRef_>{"leaf_data_identityRef"} =
    -module >> node_identifier;

template <typename It, typename Ctx, typename RCtx, typename Attr>
bool leaf_data_parse_data_path(It& first, It last, Ctx const& ctx, RCtx& rctx, Attr& attr);

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
    bool operator()(const yang::InstanceIdentifier&) const
    {
        return leaf_data_parse_data_path(first, last, ctx, rctx, attr);
    }
    bool operator()(const yang::Empty) const
    {
        return x3::attr(empty_{}).parse(first, last, ctx, rctx, attr);
    }
    template <typename Type>
    void createSetSuggestions(const Type& type) const
    {
        parserContext.m_suggestions.clear();
        std::transform(type.m_allowedValues.begin(),
                type.m_allowedValues.end(),
                std::inserter(parserContext.m_suggestions, parserContext.m_suggestions.end()),
                [](auto it) {
            std::string res;
            if constexpr (std::is_same<Type, yang::IdentityRef>()) {
                res = it.m_prefix ? it.m_prefix->m_name + ":" : "";
            }
            res += it.m_value;
            return Completion{res};
        });
        parserContext.m_completionIterator = first;
    }
    bool operator()(const yang::Enum& type) const
    {
        createSetSuggestions(type);
        x3::symbols<enum_> parser;
        for (const auto& value : type.m_allowedValues) {
            parser.add(value.m_value, value);
        }
        auto res = parser.parse(first, last, ctx, rctx, attr);
        if (!res) {
            parserContext.m_errorMsg = "leaf data type mismatch: Expected an enum here. Allowed values:";
            for (const auto& it : type.m_allowedValues) {
                parserContext.m_errorMsg += " " + it.m_value;
            }
        }
        return res;
    }
    bool operator()(const yang::IdentityRef& type) const
    {
        createSetSuggestions(type);
        auto checkValidIdentity = [this, type](auto& ctx) {
            identityRef_ pair{boost::get<identityRef_>(_attr(ctx))};
            if (!pair.m_prefix) {
                pair.m_prefix = module_{parserContext.currentSchemaPath().m_nodes.front().m_prefix.get().m_name};
            }
            _pass(ctx) = type.m_allowedValues.count(pair) != 0;
        };

        return leaf_data_identityRef[checkValidIdentity].parse(first, last, ctx, rctx, attr);
    }
    bool operator()(const yang::LeafRef& leafRef) const
    {
        return std::visit(*this, leafRef.m_targetType->m_type);
    }
    bool operator()(const yang::Bits& bits) const
    {
        parserContext.m_suggestions.clear();
        x3::symbols<std::string> parser;
        for (const auto& bit : bits.m_allowedValues) {
            parser.add(bit, bit);
            parserContext.m_suggestions.insert(Completion{bit});
        }
        parserContext.m_completionIterator = first;

        std::vector<std::string> bitsRes;

        do {
            std::string bit;
            auto pass = parser.parse(first, last, ctx, rctx, bit);
            if (pass) {
                bitsRes.push_back(bit);
                parser.remove(bit);
                parserContext.m_suggestions.erase(Completion{bit});
            }
        } while (space_separator.parse(first, last, ctx, rctx, x3::unused));

        attr = bits_{bitsRes};

        return true;
    }
    bool operator()(const yang::Union& unionInfo) const
    {
        return std::any_of(unionInfo.m_unionTypes.begin(), unionInfo.m_unionTypes.end(), [this](const auto& type) {
            return std::visit(*this, type.m_type);
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
        auto type = schema.leafType(parserContext.m_tmpListKeyLeafPath.m_location, parserContext.m_tmpListKeyLeafPath.m_node).m_type;

        auto pass = std::visit(impl_LeafData<It, Ctx, RCtx, Attr>{first, last, ctx, rctx, attr, parserContext}, type);

        if (!pass) {
            if (parserContext.m_errorMsg.empty()) {
                parserContext.m_errorMsg = "leaf data type mismatch: Expected " + leafDataTypeToString(type) + " here:";
            }
        }
        return pass;
    }
};

auto const leaf_data = LeafData();
