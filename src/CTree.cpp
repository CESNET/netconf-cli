/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/

#include "CTree.hpp"

InvalidNodeException::~InvalidNodeException() = default;

const std::unordered_set<std::string>& CTree::children(const std::string& node) const
{
    return m_nodes.at(node);
}

bool CTree::checkNode(const std::string& location, const std::string& node) const
{
    std::cout << "checkNode(" << node << ")" << std::endl;
    if (node == ".." || node.empty())
        return true;
    const auto& childrenRef = children(location); //first, get a reference to all children
    if (childrenRef.find(node) == childrenRef.end()) { //find the desired node, if it isn't present throw an exception
        std::cout << "cant find " << node << " in " << location << std::endl;
        throw InvalidNodeException(node);
    }
    return true;
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
    m_nodes.emplace(location + (location == "" ? "" : "/") + name, std::unordered_set<std::string>());
}
void CTree::initDefault()
{
    m_nodes.emplace("", std::unordered_set<std::string>());
    addNode("", "aaa");
    addNode("", "bbb");
    addNode("", "ccc");
    addNode("aaa", "aaabbb");
    addNode("aaa", "aaauuu");
    addNode("bbb", "bbbuuu");
}
