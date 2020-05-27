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
#include <variant>

// From https://github.com/boostorg/spirit/issues/270#issuecomment-450593508
#include <boost/mpl/vector.hpp>
#include <boost/spirit/home/x3/support/traits/is_variant.hpp>
#include <boost/spirit/home/x3/support/traits/tuple_traits.hpp>
#include <boost/spirit/home/x3/support/traits/variant_find_substitute.hpp>
#include <boost/spirit/home/x3/support/traits/variant_has_substitute.hpp>

// Based on: boost/spirit/home/x3/support/traits/variant_find_substitute.hpp
namespace boost::spirit::x3::traits
{
  template<typename... Ts> struct is_variant<std::variant<Ts...>> : mpl::true_ {};

  template<typename... Ts, typename Attribute>
  struct variant_find_substitute<std::variant<Ts...>, Attribute>
  {
    using variant_type = std::variant<Ts...>;
    using types = mpl::vector<Ts...>;
    using end = typename mpl::end<types>::type;

    using iter_1 = typename mpl::find_if<types, is_same<mpl::_1, Attribute>>::type;

    using iter = typename mpl::eval_if<is_same<iter_1, end>,
                                       mpl::find_if<types, traits::is_substitute<mpl::_1, Attribute>>,
                                       mpl::identity<iter_1>>::type;

    using type = typename mpl::eval_if<is_same<iter, end>,
                                       mpl::identity<Attribute>,
                                       mpl::deref<iter>>::type;
  };

  template<typename... Ts>
  struct variant_find_substitute<std::variant<Ts...>, std::variant<Ts...>>
      : mpl::identity<std::variant<Ts...>> {};

  template<typename... Ts, typename Attribute>
  struct variant_has_substitute_impl<std::variant<Ts...>, Attribute>
  {
    // Find a type from the variant that can be a substitute for Attribute.
    // return true_ if one is found, else false_

    using types = mpl::vector<Ts...>;

    using end = typename mpl::end<types>::type;

    using iter_1 = typename mpl::find_if<types, is_same<mpl::_1, Attribute>>::type;

    using iter = typename mpl::eval_if<is_same<iter_1, end>,
                                       mpl::find_if<types, traits::is_substitute<mpl::_1, Attribute>>,
                                       mpl::identity<iter_1>>::type;

    using type = mpl::not_<is_same<iter, end>>;
  };
}
namespace x3 = boost::spirit::x3;

template <typename TYPE>
struct leaf_data_class;

x3::rule<struct leaf_data_class<yang::Enum>, enum_> const leaf_data_enum = "leaf_data_enum";
x3::rule<struct leaf_data_class<yang::IdentityRef>, identityRef_> const leaf_data_identityRef = "leaf_data_identityRef";
x3::rule<struct leaf_data_class<yang::Binary>, binary_> const leaf_data_binary = "leaf_data_binary";
x3::rule<struct leaf_data_class<yang::Decimal>, double> const leaf_data_decimal = "leaf_data_decimal";
x3::rule<struct leaf_data_class<yang::String>, std::string> const leaf_data_string = "leaf_data_string";

using x3::char_;

auto const leaf_data_enum_def =
    +char_;

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

auto const leaf_data_binary_def =
    as<std::string>[+(x3::alnum | char_('+') | char_('/')) >> -char_('=') >> -char_('=')];

auto const leaf_data_identityRef_def =
    -module >> node_identifier;

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
        auto checkValidEnum = [this, type] (auto& ctx) {
            if (type.m_allowedValues.count(std::get<enum_>(attr)) == 0) {
                _pass(ctx) = false;
                parserContext.m_errorMsg = "leaf data type mismatch: Expected an enum here. Allowed values:";
                for (const auto& it : type.m_allowedValues) {
                    parserContext.m_errorMsg += " " + it.m_value;
                }
            }
        };
        return leaf_data_enum[checkValidEnum].parse(first, last, ctx, rctx, attr);
    }
    bool operator()(const yang::IdentityRef& type) const
    {
        createSetSuggestions(type);
        auto checkValidIdentity = [this, type] (auto& ctx) {
            identityRef_ pair{std::get<identityRef_>(_attr(ctx))};
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

auto const leaf_data = x3::no_skip[std::move(LeafData())];

BOOST_SPIRIT_DEFINE(leaf_data_enum)
BOOST_SPIRIT_DEFINE(leaf_data_string)
BOOST_SPIRIT_DEFINE(leaf_data_binary)
BOOST_SPIRIT_DEFINE(leaf_data_identityRef)
