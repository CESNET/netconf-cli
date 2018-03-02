/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/

#include "CTree.hpp"

InvalidNodeException::~InvalidNodeException() = default;

std::string joinPaths(const std::string& prefix, const std::string& suffix)
{
    if (prefix.empty() || suffix.empty())
        return prefix + suffix;
    else
        return prefix + '/' + suffix;
}

const std::unordered_set<std::string>& CTree::children(const std::string& node) const
{
    return m_nodes.at(node);
}

bool CTree::checkNode(const std::string& location, const std::string& node) const
{
    if (node == ".." || node.empty())
        return true;
    const auto& childrenRef = children(location); //first, get a reference to all children
    if (childrenRef.find(node) == childrenRef.end()) { //find the desired node, if it isn't present throw an exception
        throw InvalidNodeException(node);
    }
    return true;
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

void CTree::addNode(const std::string& location, const std::string& name)
{
    m_nodes.at(location).insert(name);

    //create a new set of children for the new node
    m_nodes.emplace(joinPaths(location, name), std::unordered_set<std::string>());
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
