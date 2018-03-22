/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/

#include "CTree.hpp"

InvalidNodeException::~InvalidNodeException() = default;

CTree::CTree()
{
    m_nodes.emplace("", std::unordered_set<std::string>());
}
const std::unordered_set<std::string>& CTree::children(const std::string& node) const
{
    return m_nodes.at(node);
}

bool CTree::isContainer(const std::string& location, const std::string& node) const
{
    std::cout << "checkNode(" << location << "," << node << ")" << std::endl;
    if (node == ".." || node.empty())
        return true;
    const auto& childrenRef = children(location);
    return childrenRef.find(node) != childrenRef.end();
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

void CTree::addNode(const std::string& location, const std::string& name)
{
    m_nodes.at(location).insert(name);

    //create a new set of children for the new node
    std::string key;
    if (location.empty())
        key = name;
    else
        key = location + "/" + name;
    m_nodes.emplace(key, std::unordered_set<std::string>());
}
