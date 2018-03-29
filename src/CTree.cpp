/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/

#include "CTree.hpp"

InvalidNodeException::~InvalidNodeException() = default;

inline std::string joinPaths(const std::string& prefix, const std::string& suffix)
{
    if (prefix.empty() || suffix.empty())
        return prefix + suffix;
    else
        return prefix + '/' + suffix;
}

bool TreeNode::operator<(const TreeNode& b) const
{
    return this->m_name < b.m_name;
}

CTree::CTree()
{
    m_nodes.emplace("", std::unordered_map<std::string, NODE_TYPE>());
}

const std::unordered_map<std::string, NODE_TYPE>& CTree::children(const std::string& name) const
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
    return children(location).at(name) == TYPE_CONTAINER;
}

void CTree::addContainer(const std::string& location, const std::string& name)
{
    m_nodes.at(location).emplace(name, TYPE_CONTAINER);

    //create a new set of children for the new node
    std::string key = joinPaths(location, name);
    m_nodes.emplace(key, std::unordered_map<std::string, NODE_TYPE>());
}

bool CTree::isListElement(const std::string& location, const std::string& name, const std::string& key) const
{
    if (!nodeExists(location, name + "[" + key + "]"))
        return false;
    return children(location).at(name + "[" + key + "]") == TYPE_LIST_ELEMENT;
}

void CTree::addListElement(const std::string& location, const std::string& name, const std::string& key)
{
    std::string nodeName = name + "[" + key + "]";
    m_nodes.at(location).emplace(nodeName, TYPE_LIST_ELEMENT);

    std::string mapKey = joinPaths(location, nodeName);
    m_nodes.emplace(mapKey, std::unordered_map<std::string, NODE_TYPE>());
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
