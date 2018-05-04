/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/

#include "CTree.hpp"
#include "utils.hpp"

InvalidNodeException::~InvalidNodeException() = default;


bool TreeNode::operator<(const TreeNode& b) const
{
    return this->m_name < b.m_name;
}

CTree::CTree()
{
    m_nodes.emplace("", std::unordered_map<std::string, NODE_TYPE>());
}

const std::unordered_map<std::string, NODE_TYPE>& CTree::children(const std::string& node) const
{
    return m_nodes.at(node);
}

bool CTree::nodeExists(const std::string& location, const std::string& node) const
{
    if (node.empty())
        return true;
    const auto& childrenRef = children(location);

    return childrenRef.find(node) != childrenRef.end();
}

bool CTree::isContainer(const std::string& location, const std::string& node) const
{
    if (!nodeExists(location, node))
        return false;
    return children(location).at(node) == TYPE_CONTAINER;
}

void CTree::addContainer(const std::string& location, const std::string& name)
{
    m_nodes.at(location).emplace(name, TYPE_CONTAINER);

    //create a new set of children for the new node
    std::string key = joinPaths(location, name);
    m_nodes.emplace(key, std::unordered_map<std::string, NODE_TYPE>());
}


void CTree::changeNode(const std::string& node)
{
    if (node.empty()) {
        m_curDir = "";
        return;
    }
    m_curDir += joinPaths(m_curDir, node);
}
std::string CTree::currentNode() const
{
    return m_curDir;
}
