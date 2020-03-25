#pragma once

#include <boost/spirit/home/x3.hpp>
#include "ast_values.hpp"
#include "ast_handlers.hpp"
#include "common_parsers.hpp"
#include "schema.hpp"
namespace x3 = boost::spirit::x3;

using x3::char_;
using x3::double_;
using x3::int8;
using x3::int16;
using x3::int32;
using x3::int64;
using x3::uint8;
using x3::uint16;
using x3::uint32;
using x3::uint64;

x3::rule<leaf_data_class, leaf_data_> const leaf_data = "leaf_data";
x3::rule<leaf_data_enum_class, enum_> const leaf_data_enum = "leaf_data_enum";
x3::rule<leaf_data_base_class<yang::LeafDataTypes::Decimal>, double> const leaf_data_decimal = "leaf_data_decimal";
x3::rule<leaf_data_base_class<yang::LeafDataTypes::Bool>, bool> const leaf_data_bool = "leaf_data_bool";
x3::rule<leaf_data_base_class<yang::LeafDataTypes::Int8>, int8_t> const leaf_data_int8 = "leaf_data_int8";
x3::rule<leaf_data_base_class<yang::LeafDataTypes::Uint8>, uint8_t> const leaf_data_uint8 = "leaf_data_uint8";
x3::rule<leaf_data_base_class<yang::LeafDataTypes::Int16>, int16_t> const leaf_data_int16 = "leaf_data_int16";
x3::rule<leaf_data_base_class<yang::LeafDataTypes::Uint16>, uint16_t> const leaf_data_uint16 = "leaf_data_uint16";
x3::rule<leaf_data_base_class<yang::LeafDataTypes::Int32>, int32_t> const leaf_data_int32 = "leaf_data_int32";
x3::rule<leaf_data_base_class<yang::LeafDataTypes::Uint32>, uint32_t> const leaf_data_uint32 = "leaf_data_uint32";
x3::rule<leaf_data_base_class<yang::LeafDataTypes::Int64>, int64_t> const leaf_data_int64 = "leaf_data_int64";
x3::rule<leaf_data_base_class<yang::LeafDataTypes::Uint64>, uint64_t> const leaf_data_uint64 = "leaf_data_uint64";
x3::rule<leaf_data_base_class<yang::LeafDataTypes::String>, std::string> const leaf_data_string = "leaf_data_string";
x3::rule<leaf_data_binary_data_class, std::string> const leaf_data_binary_data = "leaf_data_binary_data";
x3::rule<leaf_data_base_class<yang::LeafDataTypes::Binary>, binary_> const leaf_data_binary = "leaf_data_binary";
x3::rule<leaf_data_identityRef_data_class, identityRef_> const leaf_data_identityRef_data = "leaf_data_identityRef_data";
x3::rule<leaf_data_identityRef_class, identityRef_> const leaf_data_identityRef = "leaf_data_identityRef";

x3::rule<createSetSuggestions_class<yang::LeafDataTypes::Enum>, x3::unused_type> const createEnumSuggestions = "createEnumSuggestions";
x3::rule<createSetSuggestions_class<yang::LeafDataTypes::IdentityRef>, x3::unused_type> const createIdentitySuggestions = "createIdentitySuggestions";

auto const createEnumSuggestions_def =
    x3::eps;

auto const leaf_data_enum_def =
    createEnumSuggestions >> +char_;

auto const leaf_data_decimal_def =
    double_;

struct bool_symbol_table : x3::symbols<bool> {
    bool_symbol_table()
    {
    add
        ("true", true)
        ("false", false);
    }
} const bool_rule;

auto const leaf_data_bool_def =
    bool_rule;
auto const leaf_data_int8_def =
    int8;
auto const leaf_data_int16_def =
    int16;
auto const leaf_data_int32_def =
    int32;
auto const leaf_data_int64_def =
    int64;
auto const leaf_data_uint8_def =
    uint8;
auto const leaf_data_uint16_def =
    uint16;
auto const leaf_data_uint32_def =
    uint32;
auto const leaf_data_uint64_def =
    uint64;
auto const leaf_data_string_def =
    '\'' >> *(char_-'\'') >> '\'' |
    '\"' >> *(char_-'\"') >> '\"';

// This intermediate rule is neccessary for coercing to std::string.
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

auto const leaf_data_def =
x3::no_skip[x3::expect[
    leaf_data_enum |
    leaf_data_decimal |
    leaf_data_bool |
    leaf_data_int8 |
    leaf_data_int16 |
    leaf_data_int32 |
    leaf_data_int64 |
    leaf_data_uint8 |
    leaf_data_uint16 |
    leaf_data_uint32 |
    leaf_data_uint64 |
    leaf_data_binary |
    leaf_data_identityRef |
    leaf_data_string]];

BOOST_SPIRIT_DEFINE(leaf_data)
BOOST_SPIRIT_DEFINE(leaf_data_enum)
BOOST_SPIRIT_DEFINE(leaf_data_decimal)
BOOST_SPIRIT_DEFINE(leaf_data_bool)
BOOST_SPIRIT_DEFINE(leaf_data_int8)
BOOST_SPIRIT_DEFINE(leaf_data_int16)
BOOST_SPIRIT_DEFINE(leaf_data_int32)
BOOST_SPIRIT_DEFINE(leaf_data_int64)
BOOST_SPIRIT_DEFINE(leaf_data_uint8)
BOOST_SPIRIT_DEFINE(leaf_data_uint16)
BOOST_SPIRIT_DEFINE(leaf_data_uint32)
BOOST_SPIRIT_DEFINE(leaf_data_uint64)
BOOST_SPIRIT_DEFINE(leaf_data_string)
BOOST_SPIRIT_DEFINE(leaf_data_binary_data)
BOOST_SPIRIT_DEFINE(leaf_data_binary)
BOOST_SPIRIT_DEFINE(leaf_data_identityRef_data)
BOOST_SPIRIT_DEFINE(leaf_data_identityRef)
BOOST_SPIRIT_DEFINE(createEnumSuggestions)
BOOST_SPIRIT_DEFINE(createIdentitySuggestions)
