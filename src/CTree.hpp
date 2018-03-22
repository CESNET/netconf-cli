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
    bool checkNode(const std::string& location, const std::string& node) const;
    void changeNode(const std::string& node);
    void addNode(const std::string& location, const std::string& node);
    void initDefault();
    std::string currentNode() const;

private:
    const std::unordered_set<std::string>& children(const std::string& node) const;

    std::unordered_map<std::string, std::unordered_set<std::string>> m_nodes;
    std::string m_curDir;
};
