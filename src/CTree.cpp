/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/

#include "CTree.hpp"

class InvalidNodeException : std::invalid_argument {
public:
    InvalidNodeException(const std::string& node_name)
        : std::invalid_argument(node_name)
    {
        m_msg = "Invalid node name: ";
        m_msg += std::invalid_argument::what();
    }
    const char* what() const noexcept
    {
        return m_msg.c_str();
    }

private:
    std::string m_msg;
};

const std::unordered_set<std::string>& CTree::getChildren(const std::string& node) const
{
    return m_nodes.at(node);
}

bool CTree::checkNode(const std::string& node) const
{
    if (node == "..")
        return true;
    const auto& children = getChildren(m_cur_dir); //first, get a reference to all children
    if (children.find(node) == children.end()) { //find the desired node, if it isn't present throw an exception
        throw InvalidNodeException(node);
    }
    return true;
}
void CTree::changeNode(const std::string& node)
{
    checkNode(node);
    m_cur_dir += node;
}
void CTree::addNode(const std::string& location, const std::string& name)
{
    m_nodes.at(location).insert(name);

    //create a new set of children for the new node
    m_nodes.emplace(location + "/" + name, std::unordered_set<std::string>());
}
void CTree::initDefault()
{
    m_nodes.emplace("", std::unordered_set<std::string>());
}
