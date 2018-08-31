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

struct module_ {
    bool operator==(const module_& b) const;
    std::string m_name;
};

struct node_ {
    boost::optional<module_> m_prefix;
    boost::variant<container_, listElement_, nodeup_, leaf_> m_suffix;

    node_();
    node_(decltype(m_suffix) node);
    node_(module_ module, decltype(m_suffix) node);
    bool operator==(const node_& b) const;
};

enum class Scope
{
    Absolute,
    Relative
};

struct path_ {
    bool operator==(const path_& b) const;
    Scope m_scope = Scope::Relative;
    std::vector<node_> m_nodes;
};

std::string nodeToSchemaString(decltype(path_::m_nodes)::value_type node);

std::string pathToAbsoluteSchemaString(const path_& path);
std::string pathToDataString(const path_& path);
std::string pathToSchemaString(const path_& path);

BOOST_FUSION_ADAPT_STRUCT(container_, m_name)
BOOST_FUSION_ADAPT_STRUCT(listElement_, m_name, m_keys)
BOOST_FUSION_ADAPT_STRUCT(module_, m_name)
BOOST_FUSION_ADAPT_STRUCT(node_, m_prefix, m_suffix)
BOOST_FUSION_ADAPT_STRUCT(path_, m_scope, m_nodes)
