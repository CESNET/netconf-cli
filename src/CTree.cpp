/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/

#include <iostream>
#include "CTree.hpp"
#include "utils.hpp"

InvalidNodeException::~InvalidNodeException() = default;



CTree::CTree()
{
    m_nodes.emplace("", std::unordered_map<std::string, NodeType>());
}

const std::unordered_map<std::string, NodeType>& CTree::children(const std::string& name) const
{
    return m_nodes.at(name);
}

bool CTree::nodeExists(const std::string& location, const std::string& name) const
{
    if (name.empty())
        return true;
    const auto& childrenRef = children(location);

    return childrenRef.find(name) != childrenRef.end();
}

bool CTree::isContainer(const std::string& location, const std::string& name) const
{
    if (!nodeExists(location, name))
        return false;

    return children(location).at(name).type() == typeid(schema::container);
}

void CTree::addContainer(const std::string& location, const std::string& name)
{
    m_nodes.at(location).emplace(name, schema::container{});

    //create a new set of children for the new node
    std::string key = joinPaths(location, name);
    m_nodes.emplace(key, std::unordered_map<std::string, NodeType>());
}


bool CTree::listHasKey(const std::string& location, const std::string& name, const std::string& key) const
{
    assert(isList(location, name));

    const auto& child = children(location).at(name);
    const auto& list = boost::get<schema::list>(child);
    return list.m_keys.find(key) != list.m_keys.end();
}

const std::set<std::string>& CTree::listKeys(const std::string& location, const std::string& name) const
{
    assert(isList(location, name));

    const auto& child = children(location).at(name);
    const auto& list = boost::get<schema::list>(child);
    return list.m_keys;
}

bool CTree::isList(const std::string& location, const std::string& name) const
{
    if (!nodeExists(location, name))
        return false;
    const auto &child = children(location).at(name);
    if ((child.type() != typeid(schema::list)))
        return false;

    return true;
}

void CTree::addList(const std::string& location, const std::string& name, const std::set<std::string>& keys)
{
    m_nodes.at(location).emplace(name, schema::list{keys});

    m_nodes.emplace(name, std::unordered_map<std::string, NodeType>());
}


void CTree::changeNode(const std::string& name)
{
    if (name.empty()) {
        m_curDir = "";
        return;
    }
    m_curDir += joinPaths(m_curDir, name);
}
std::string CTree::currentNode() const
{
    return m_curDir;
}
