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

struct nodeup_ {
    bool operator==(const nodeup_&) const
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

struct leaf_ {
    leaf_() = default;
    leaf_(const std::string& name);

    bool operator==(const leaf_& b) const;

    std::string m_name;
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
    std::vector<boost::variant<container_, listElement_, nodeup_, leaf_>> m_nodes;
};


std::string pathToDataString(const path_& path);
std::string pathToSchemaString(const path_& path);

BOOST_FUSION_ADAPT_STRUCT(container_, m_name)
BOOST_FUSION_ADAPT_STRUCT(listElement_, m_name, m_keys)
BOOST_FUSION_ADAPT_STRUCT(path_, m_nodes)
