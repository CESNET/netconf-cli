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

#include "ast_values.hpp"

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

struct list_ {
    list_() {}
    list_(const std::string& listName);

    bool operator==(const list_& b) const;

    std::string m_name;
};

struct schemaNode_ {
    boost::optional<module_> m_prefix;
    boost::variant<container_, list_, nodeup_, leaf_> m_suffix;

    schemaNode_();
    schemaNode_(decltype(m_suffix) node);
    schemaNode_(module_ module, decltype(m_suffix) node);
    bool operator==(const schemaNode_& b) const;
};

struct dataNode_ {
    boost::optional<module_> m_prefix;
    boost::variant<container_, listElement_, nodeup_, leaf_, list_> m_suffix;

    dataNode_();
    dataNode_(decltype(m_suffix) node);
    dataNode_(module_ module, decltype(m_suffix) node);
    bool operator==(const dataNode_& b) const;
};

enum class TrailingSlash {
    Present,
    NonPresent
};

enum class Scope {
    Absolute,
    Relative
};

struct schemaPath_ {
    bool operator==(const schemaPath_& b) const;
    Scope m_scope = Scope::Relative;
    std::vector<schemaNode_> m_nodes;
    TrailingSlash m_trailingSlash = TrailingSlash::NonPresent;
};

struct dataPath_ {
    bool operator==(const dataPath_& b) const;
    Scope m_scope = Scope::Relative;
    std::vector<dataNode_> m_nodes;
    TrailingSlash m_trailingSlash = TrailingSlash::NonPresent;
};

std::string nodeToSchemaString(decltype(dataPath_::m_nodes)::value_type node);

std::string pathToAbsoluteSchemaString(const dataPath_& path);
std::string pathToAbsoluteSchemaString(const schemaPath_& path);
std::string pathToDataString(const dataPath_& path);
std::string pathToSchemaString(const schemaPath_& path);
schemaNode_ dataNodeToSchemaNode(const dataNode_& node);
schemaPath_ dataPathToSchemaPath(const dataPath_& path);

BOOST_FUSION_ADAPT_STRUCT(container_, m_name)
BOOST_FUSION_ADAPT_STRUCT(listElement_, m_name, m_keys)
BOOST_FUSION_ADAPT_STRUCT(module_, m_name)
BOOST_FUSION_ADAPT_STRUCT(dataNode_, m_prefix, m_suffix)
BOOST_FUSION_ADAPT_STRUCT(schemaNode_, m_prefix, m_suffix)
BOOST_FUSION_ADAPT_STRUCT(dataPath_, m_scope, m_nodes, m_trailingSlash)
BOOST_FUSION_ADAPT_STRUCT(schemaPath_, m_scope, m_nodes, m_trailingSlash)
