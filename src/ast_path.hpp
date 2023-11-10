/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/
#pragma once

#include <boost/fusion/adapted/struct/adapt_struct.hpp>
#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/fusion/include/std_pair.hpp>
#include <map>
#include <variant>
#include <vector>

#include "ast_values.hpp"
#include "list_instance.hpp"

enum class Prefixes {
    Always,
    WhenNeeded
};

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

struct leafList_ {
    leafList_(const std::string& name);

    bool operator==(const leafList_& b) const;

    std::string m_name;
};

struct leafListElement_ {
    bool operator==(const leafListElement_& b) const;

    std::string m_name;
    leaf_data_ m_value;
};

struct listElement_ {
    listElement_() = default;
    listElement_(const std::string& listName, const ListInstance& keys);

    bool operator==(const listElement_& b) const;

    std::string m_name;
    ListInstance m_keys;
};

struct list_ {
    list_() = default;
    list_(const std::string& listName);

    bool operator==(const list_& b) const;

    std::string m_name;
};

struct rpcNode_ {
    bool operator==(const rpcNode_& other) const;

    std::string m_name;
};

struct actionNode_ {
    bool operator==(const actionNode_& other) const;

    std::string m_name;
};

struct schemaNode_ {
    boost::optional<module_> m_prefix;
    std::variant<container_, list_, nodeup_, leaf_, leafList_, rpcNode_, actionNode_> m_suffix;

    schemaNode_();
    schemaNode_(decltype(m_suffix) node);
    schemaNode_(module_ module, decltype(m_suffix) node);
    bool operator==(const schemaNode_& b) const;
};

struct dataNode_ {
    boost::optional<module_> m_prefix;
    std::variant<container_, listElement_, nodeup_, leaf_, leafListElement_, leafList_, list_, rpcNode_, actionNode_> m_suffix;

    dataNode_();
    dataNode_(decltype(m_suffix) node);
    dataNode_(boost::optional<module_> module, decltype(m_suffix) node);
    dataNode_(module_ module, decltype(m_suffix) node);
    bool operator==(const dataNode_& b) const;
};

enum class Scope {
    Absolute,
    Relative
};

struct schemaPath_ {
    schemaPath_();
    schemaPath_(const Scope scope, const std::vector<schemaNode_>& nodes);
    bool operator==(const schemaPath_& b) const;
    Scope m_scope = Scope::Relative;
    std::vector<schemaNode_> m_nodes;
    // @brief Pushes a new fragment. Pops a fragment if it's nodeup_
    void pushFragment(const schemaNode_& fragment);
};

struct dataPath_ {
    dataPath_();
    dataPath_(const Scope scope, const std::vector<dataNode_>& nodes);
    bool operator==(const dataPath_& b) const;
    Scope m_scope = Scope::Relative;
    std::vector<dataNode_> m_nodes;

    // @brief Pushes a new fragment. Pops a fragment if it's nodeup_
    void pushFragment(const dataNode_& fragment);
};

enum class WritableOps {
    Yes,
    No
};

std::string nodeToSchemaString(decltype(dataPath_::m_nodes)::value_type node);

std::string pathToDataString(const dataPath_& path, Prefixes prefixes);
std::string pathToSchemaString(const schemaPath_& path, Prefixes prefixes);
std::string pathToSchemaString(const dataPath_& path, Prefixes prefixes);
schemaNode_ dataNodeToSchemaNode(const dataNode_& node);
schemaPath_ dataPathToSchemaPath(const dataPath_& path);
std::string escapeListKeyString(const std::string& what);

// @brief Combining the current working directory with a new path, this function returns the resulting absolute path
dataPath_ realPath(const dataPath_& cwd, const dataPath_& newPath);

BOOST_FUSION_ADAPT_STRUCT(container_, m_name)
BOOST_FUSION_ADAPT_STRUCT(listElement_, m_name, m_keys)
BOOST_FUSION_ADAPT_STRUCT(leafListElement_, m_name, m_value)
BOOST_FUSION_ADAPT_STRUCT(module_, m_name)
BOOST_FUSION_ADAPT_STRUCT(dataNode_, m_prefix, m_suffix)
BOOST_FUSION_ADAPT_STRUCT(schemaNode_, m_prefix, m_suffix)
BOOST_FUSION_ADAPT_STRUCT(dataPath_, m_scope, m_nodes)
BOOST_FUSION_ADAPT_STRUCT(schemaPath_, m_scope, m_nodes)
