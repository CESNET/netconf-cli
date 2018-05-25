/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/
#pragma once
#include <boost/spirit/home/x3.hpp>
#include <boost/spirit/home/x3/support/ast/position_tagged.hpp>
#include <boost/spirit/home/x3/support/utility/error_reporting.hpp>

#include <boost/fusion/adapted/struct/adapt_struct.hpp>
#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/fusion/include/std_pair.hpp>
#include <boost/variant.hpp>
#include <map>
#include <vector>

#include "utils.hpp"
namespace x3 = boost::spirit::x3;
namespace ascii = boost::spirit::x3::ascii;

using x3::alpha;
using x3::alnum;
using x3::lit;
using x3::char_;
using x3::_attr;
using x3::lexeme;
using x3::expect;
using ascii::space;
using boost::fusion::operator<<;


using keyValue_ = std::pair<std::string, std::string>;


struct parser_context_tag;


struct nodeup_ {
    bool operator==(const nodeup_& b) const
    {
       return true;
    }
};

struct container_ {
    container_() = default;
    container_(const std::string& name);

    bool operator==(const container_& b) const;

    std::string m_name;
};


struct list_ {
    std::vector<std::string> m_keys;
};

struct listElement_ {
    listElement_() {}
    listElement_(const std::string& listName, const std::map<std::string, std::string>& keys);

    bool operator==(const listElement_& b) const;

    std::string m_name;
    std::map<std::string, std::string> m_keys;
};


struct path_ {
    bool operator==(const path_& b) const;
    std::vector<boost::variant<container_, listElement_, nodeup_>> m_nodes;
};


struct cd_ : x3::position_tagged {
    bool operator==(const cd_& b) const;
    path_ m_path;
};

BOOST_FUSION_ADAPT_STRUCT(nodeup_)
BOOST_FUSION_ADAPT_STRUCT(container_, m_name)
BOOST_FUSION_ADAPT_STRUCT(listElement_, m_name, m_keys)
BOOST_FUSION_ADAPT_STRUCT(path_, m_nodes)
BOOST_FUSION_ADAPT_STRUCT(cd_, m_path)
