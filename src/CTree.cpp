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


std::string pathToString(const path_& path)
{
   std::string res;
   for (const auto it : path.m_nodes)
       res = joinPaths(res, boost::apply_visitor(nodeToString(), it));
   return res;
}

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

bool CTree::isContainer(const path_& location, const std::string& name) const
{
    std::string locationString = pathToString(location);
    if (!nodeExists(locationString, name))
        return false;

    return children(locationString).at(name).type() == typeid(schema::container);
}

void CTree::addContainer(const std::string& location, const std::string& name)
{
    m_nodes.at(location).emplace(name, schema::container{});

    //create a new set of children for the new node
    std::string key = joinPaths(location, name);
    m_nodes.emplace(key, std::unordered_map<std::string, NodeType>());
}


bool CTree::listHasKey(const path_& location, const std::string& name, const std::string& key) const
{
    std::string locationString = pathToString(location);
    assert(isList(location, name));

    const auto& child = children(locationString).at(name);
    const auto& list = boost::get<schema::list>(child);
    return list.m_keys.find(key) != list.m_keys.end();
}

const std::set<std::string>& CTree::listKeys(const path_& location, const std::string& name) const
{
    std::string locationString = pathToString(location);
    assert(isList(location, name));

    const auto& child = children(locationString).at(name);
    const auto& list = boost::get<schema::list>(child);
    return list.m_keys;
}

bool CTree::isList(const path_& location, const std::string& name) const
{
    std::string locationString = pathToString(location);
    if (!nodeExists(locationString, name))
        return false;
    const auto& child = children(locationString).at(name);
    if (child.type() != typeid(schema::list))
        return false;

    return true;
}

void CTree::addList(const std::string& location, const std::string& name, const std::set<std::string>& keys)
{
    m_nodes.at(location).emplace(name, schema::list{keys});

    m_nodes.emplace(name, std::unordered_map<std::string, NodeType>());
}

