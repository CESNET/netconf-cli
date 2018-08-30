/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/
#pragma once

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

struct ls_ : x3::position_tagged {
    bool operator==(const ls_& b) const;
    boost::optional<path_> m_path;
};

struct cd_ : x3::position_tagged {
    bool operator==(const cd_& b) const;
    path_ m_path;
};

struct create_ : x3::position_tagged {
    bool operator==(const create_& b) const;
    path_ m_path;
};

struct delete_ : x3::position_tagged {
    bool operator==(const delete_& b) const;
    path_ m_path;
};

struct set_ : x3::position_tagged {
    bool operator==(const set_& b) const;
    path_ m_path;
    leaf_data_ m_data;
};

struct get_ : x3::position_tagged {
    bool operator==(const get_& b) const;
    path_ m_path;
};

using command_ = boost::variant<ls_, cd_, create_, delete_, set_, get_>;

BOOST_FUSION_ADAPT_STRUCT(ls_, m_path)
BOOST_FUSION_ADAPT_STRUCT(cd_, m_path)
BOOST_FUSION_ADAPT_STRUCT(create_, m_path)
BOOST_FUSION_ADAPT_STRUCT(delete_, m_path)
BOOST_FUSION_ADAPT_STRUCT(enum_, m_value)
BOOST_FUSION_ADAPT_STRUCT(set_, m_path, m_data)
BOOST_FUSION_ADAPT_STRUCT(get_, m_path)
