/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/

#include "CTree.hpp"

InvalidNodeException::~InvalidNodeException() = default;

bool TreeNode::operator<(const TreeNode& b) const
{
    return this->m_name < b.m_name;
}

CTree::CTree()
{
    m_nodes.emplace("", std::set<TreeNode>());
}
const std::set<TreeNode>& CTree::children(const std::string& node) const
{
    return m_nodes.at(node);
}

bool CTree::nodeExists(const std::string& location, const std::string& node) const
{
    if (node.empty())
        return true;
    const auto& childrenRef = children(location);
    for (const auto it : childrenRef) {
        if (it.m_name == node)
            return true;
    }

    return false;
}

bool CTree::isContainer(const std::string& location, const std::string& node) const
{
    if (!nodeExists(location, node))
        return false;
    for (const auto it : children(location)) {
        if (it.m_name == node) {
            return it.m_type == TYPE_CONTAINER;
        }
    }
}

void CTree::addContainer(const std::string& location, const std::string& name)
{
    TreeNode newContainer;
    newContainer.m_name = name;
    newContainer.m_type = TYPE_CONTAINER;

    m_nodes.at(location).insert(newContainer);

    //create a new set of children for the new node
    std::string key;
    if (location.empty())
        key = name;
    else
        key = location + "/" + name;
    m_nodes.emplace(key, std::set<TreeNode>());
}


void CTree::changeNode(const std::string& node)
{
    if (node.empty()) {
        m_curDir = "";
        std::cout << "cd to " << node << std::endl;
        return;
    }
    m_curDir += (m_curDir.empty() ? "" : "/") + node;
    std::cout << "cd to " << node << std::endl;
}
std::string CTree::currentNode() const
{
    return m_curDir;
}
