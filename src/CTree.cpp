/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/

#include <iostream>
#include "CTree.hpp"

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

Result CTree::isContainer(const std::string& location, const std::string& name) const
{
    if (!nodeExists(location, name))
        return std::make_pair(false, "");

    return std::make_pair(children(location).at(name).type() == typeid(schema::container), "");
}

void CTree::addContainer(const std::string& location, const std::string& name)
{
    m_nodes.at(location).emplace(name, schema::container{});

    //create a new set of children for the new node
    std::string key = joinPaths(location, name);
    m_nodes.emplace(key, std::unordered_map<std::string, NodeType>());
}

Result CTree::isList(const std::string& location, const std::string& name, const std::set<std::string>& keys) const
{
    if (!nodeExists(location, name))
        return std::make_pair(false, "");
    auto &child = children(location).at(name);
    if(!(child.type() == typeid(schema::list)))
        return std::make_pair(false, "");

    auto &list = boost::get<schema::list>(child);
    if (list.m_keys != keys)
    {
        std::set<std::string> bad_keys;
        std::set_difference(list.m_keys.begin(), list.m_keys.end(), keys.begin(), keys.end(), std::inserter(bad_keys, bad_keys.end()));

        std::string error = "bad keys for " + name + ":";
        for (const auto& it : bad_keys)
        {
            error += " " + it;
        }
        return std::make_pair(false, error);
    }
    return std::make_pair(true, "");
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
