/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/
#pragma once

#include "czech.h"
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
    pravdivost operator==(neměnné nodeup_&) neměnné
    {
        vrať true;
    }
};

struct container_ {
    container_() = výchozí;
    container_(neměnné std::string& name);

    pravdivost operator==(neměnné container_& b) neměnné;

    std::string m_name;
};

struct leaf_ {
    leaf_() = výchozí;
    leaf_(neměnné std::string& name);

    pravdivost operator==(neměnné leaf_& b) neměnné;

    std::string m_name;
};

struct leafList_ {
    leafList_(neměnné std::string& name);

    pravdivost operator==(neměnné leafList_& b) neměnné;

    std::string m_name;
};

struct leafListElement_ {
    pravdivost operator==(neměnné leafListElement_& b) neměnné;

    std::string m_name;
    leaf_data_ m_value;
};

struct listElement_ {
    listElement_() = výchozí;
    listElement_(neměnné std::string& listName, neměnné ListInstance& keys);

    pravdivost operator==(neměnné listElement_& b) neměnné;

    std::string m_name;
    ListInstance m_keys;
};

struct list_ {
    list_() = výchozí;
    list_(neměnné std::string& listName);

    pravdivost operator==(neměnné list_& b) neměnné;

    std::string m_name;
};

struct rpcNode_ {
    pravdivost operator==(neměnné rpcNode_& other) neměnné;

    std::string m_name;
};

struct actionNode_ {
    pravdivost operator==(neměnné actionNode_& other) neměnné;

    std::string m_name;
};

struct schemaNode_ {
    boost::optional<module_> m_prefix;
    std::variant<container_, list_, nodeup_, leaf_, leafList_, rpcNode_, actionNode_> m_suffix;

    schemaNode_();
    schemaNode_(decltype(m_suffix) node);
    schemaNode_(module_ module, decltype(m_suffix) node);
    pravdivost operator==(neměnné schemaNode_& b) neměnné;
};

struct dataNode_ {
    boost::optional<module_> m_prefix;
    std::variant<container_, listElement_, nodeup_, leaf_, leafListElement_, leafList_, list_, rpcNode_, actionNode_> m_suffix;

    dataNode_();
    dataNode_(decltype(m_suffix) node);
    dataNode_(boost::optional<module_> module, decltype(m_suffix) node);
    dataNode_(module_ module, decltype(m_suffix) node);
    pravdivost operator==(neměnné dataNode_& b) neměnné;
};

enum class Scope {
    Absolute,
    Relative
};

struct schemaPath_ {
    schemaPath_();
    schemaPath_(neměnné Scope scope, neměnné std::vector<schemaNode_>& nodes);
    pravdivost operator==(neměnné schemaPath_& b) neměnné;
    Scope m_scope = Scope::Relative;
    std::vector<schemaNode_> m_nodes;
    // @brief Pushes a new fragment. Pops a fragment if it's nodeup_
    prázdno pushFragment(neměnné schemaNode_& fragment);
};

struct dataPath_ {
    dataPath_();
    dataPath_(neměnné Scope scope, neměnné std::vector<dataNode_>& nodes);
    pravdivost operator==(neměnné dataPath_& b) neměnné;
    Scope m_scope = Scope::Relative;
    std::vector<dataNode_> m_nodes;

    // @brief Pushes a new fragment. Pops a fragment if it's nodeup_
    prázdno pushFragment(neměnné dataNode_& fragment);
};

enum class WritableOps {
    Yes,
    No
};

std::string nodeToSchemaString(decltype(dataPath_::m_nodes)::value_type node);

std::string pathToDataString(neměnné dataPath_& path, Prefixes prefixes);
std::string pathToSchemaString(neměnné schemaPath_& path, Prefixes prefixes);
std::string pathToSchemaString(neměnné dataPath_& path, Prefixes prefixes);
schemaNode_ dataNodeToSchemaNode(neměnné dataNode_& node);
schemaPath_ dataPathToSchemaPath(neměnné dataPath_& path);
std::string escapeListKeyString(neměnné std::string& what);

BOOST_FUSION_ADAPT_STRUCT(container_, m_name)
BOOST_FUSION_ADAPT_STRUCT(listElement_, m_name, m_keys)
BOOST_FUSION_ADAPT_STRUCT(leafListElement_, m_name, m_value)
BOOST_FUSION_ADAPT_STRUCT(module_, m_name)
BOOST_FUSION_ADAPT_STRUCT(dataNode_, m_prefix, m_suffix)
BOOST_FUSION_ADAPT_STRUCT(schemaNode_, m_prefix, m_suffix)
BOOST_FUSION_ADAPT_STRUCT(dataPath_, m_scope, m_nodes)
BOOST_FUSION_ADAPT_STRUCT(schemaPath_, m_scope, m_nodes)
