/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/

#pragma once

#include <iostream>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>

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

/*! \class CTree
 *     \brief The class representing the tree, that the user traverses.
 *
 *         This class holds the current position in the tree and handles changing the position,
 *         including checking what nodes are available.
 *         */
class CTree {
public:
    bool checkNode(const std::string& location, const std::string& node) const;
    void changeNode(const std::string& node);
    void initDefault();
    std::string currentNode() const;

private:
    void addNode(const std::string& location, const std::string& node);
    const std::unordered_set<std::string>& children(const std::string& node) const;

    std::unordered_map<std::string, std::unordered_set<std::string>> m_nodes;
    std::string m_cur_dir;
};
