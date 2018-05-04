/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/

#pragma once

#include <stdexcept>
#include <unordered_map>

#include "utils.hpp"

enum NODE_TYPE {
    TYPE_CONTAINER,
    TYPE_LIST,
    TYPE_LIST_ELEMENT
};

struct TreeNode {
    bool operator<(const TreeNode& b) const;
    std::string m_name;
    NODE_TYPE m_type;
};


class InvalidNodeException : public std::invalid_argument {
public:
    using std::invalid_argument::invalid_argument;
    ~InvalidNodeException() override;
};

/*! \class CTree
 *     \brief The class representing the tree, that the user traverses.
 *
 *         This class holds the current position in the tree and handles changing the position,
 *         including checking what nodes are available.
 *         */
class CTree {
public:
    CTree();
    bool nodeExists(const std::string& location, const std::string& name) const;

    bool isContainer(const std::string& location, const std::string& name) const;
    void addContainer(const std::string& location, const std::string& name);
    bool isListElement(const std::string& location, const std::string& name, const std::string& key) const;
    void addListElement(const std::string& location, const std::string& name, const std::string& key);
    void changeNode(const std::string& name);
    std::string currentNode() const;

private:
    const std::unordered_map<std::string, NODE_TYPE>& children(const std::string& name) const;

    std::unordered_map<std::string, std::unordered_map<std::string, NODE_TYPE>> m_nodes;
    std::string m_curDir;
};
