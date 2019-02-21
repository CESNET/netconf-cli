/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/
#pragma once

#include <boost/mpl/vector.hpp>
#include "ast_path.hpp"
#include "ast_values.hpp"

namespace x3 = boost::spirit::x3;
namespace ascii = boost::spirit::x3::ascii;

using ascii::space;
using x3::_attr;
using x3::alnum;
using x3::alpha;
using x3::char_;
using x3::double_;
using x3::expect;
using x3::int_;
using x3::lexeme;
using x3::lit;
using x3::uint_;

struct parser_context_tag;

using keyValue_ = std::pair<std::string, std::string>;

enum class LsOption {
    Recursive
};

struct discard_ : x3::position_tagged {
    static constexpr auto name = "discard";
    static constexpr auto shortHelp = "discard - Short help for discard.";
    static constexpr auto longHelp = "Long help for \"discard\".\n\n such long. very help. wow.";
    bool operator==(const discard_& b) const;
};

struct ls_ : x3::position_tagged {
    static constexpr auto name = "ls";
    static constexpr auto shortHelp = "ls - Short help for ls.";
    static constexpr auto longHelp = "Long help for \"ls\".\n\n such long. very help. wow.";
    bool operator==(const ls_& b) const;
    std::vector<LsOption> m_options;
    boost::optional<boost::variant<dataPath_, schemaPath_>> m_path;
};

struct cd_ : x3::position_tagged {
    static constexpr auto name = "cd";
    static constexpr auto shortHelp = "cd - Short help for cd.";
    static constexpr auto longHelp = "Long help for \"cd\".\n\n such long. very help. wow.";
    bool operator==(const cd_& b) const;
    dataPath_ m_path;
};

struct create_ : x3::position_tagged {
    static constexpr auto name = "create";
    static constexpr auto shortHelp = "create - Short help for create.";
    static constexpr auto longHelp = "Long help for \"create\".\n\n such long. very help. wow.";
    bool operator==(const create_& b) const;
    dataPath_ m_path;
};

struct delete_ : x3::position_tagged {
    static constexpr auto name = "delete";
    static constexpr auto shortHelp = "delete - Short help for delete.";
    static constexpr auto longHelp = "Long help for \"delete\".\n\n such long. very help. wow.";
    bool operator==(const delete_& b) const;
    dataPath_ m_path;
};

struct set_ : x3::position_tagged {
    static constexpr auto name = "set";
    static constexpr auto shortHelp = "set - Short help for set.";
    static constexpr auto longHelp = "Long help for \"set\".\n\n such long. very help. wow.";
    bool operator==(const set_& b) const;
    dataPath_ m_path;
    leaf_data_ m_data;
};

struct commit_ : x3::position_tagged {
    static constexpr auto name = "commit";
    static constexpr auto shortHelp = "commit - Short help for commit.";
    static constexpr auto longHelp = "Long help for \"commit\".\n\n such long. very help. wow.";
    bool operator==(const set_& b) const;
};

struct get_ : x3::position_tagged {
    static constexpr auto name = "get";
    static constexpr auto shortHelp = "get - Short help for get.";
    static constexpr auto longHelp = "Long help for \"get\".\n\n such long. very help. wow.";
    bool operator==(const get_& b) const;
    boost::optional<boost::variant<dataPath_, schemaPath_>> m_path;
};

struct help_;
using CommandTypes = boost::mpl::vector<discard_, ls_, cd_, create_, delete_, set_, commit_, get_, help_>;
struct help_ : x3::position_tagged {
    static constexpr auto name = "help";
    static constexpr auto shortHelp = "help - Short help for help.";
    static constexpr auto longHelp = "Long help for \"help\".\n\n such long. very help. wow.";
    bool operator==(const help_& b) const;

    // This is complete garbage
    boost::optional<boost::make_variant_over<boost::mpl::transform<CommandTypes, boost::type<boost::mpl::_>>::type>::type> m_cmd;
};

// TODO: The usage of MPL won't be necessary after std::variant support is added to Spirit
// https://github.com/boostorg/spirit/issues/270
using command_ = boost::make_variant_over<CommandTypes>::type;

BOOST_FUSION_ADAPT_STRUCT(ls_, m_options, m_path)
BOOST_FUSION_ADAPT_STRUCT(cd_, m_path)
BOOST_FUSION_ADAPT_STRUCT(create_, m_path)
BOOST_FUSION_ADAPT_STRUCT(delete_, m_path)
BOOST_FUSION_ADAPT_STRUCT(enum_, m_value)
BOOST_FUSION_ADAPT_STRUCT(set_, m_path, m_data)
BOOST_FUSION_ADAPT_STRUCT(commit_)
BOOST_FUSION_ADAPT_STRUCT(help_, m_cmd)
BOOST_FUSION_ADAPT_STRUCT(discard_)
BOOST_FUSION_ADAPT_STRUCT(get_, m_path)
