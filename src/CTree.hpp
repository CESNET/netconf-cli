/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/

#pragma once

#include <boost/variant.hpp>
#include <stdexcept>
#include <unordered_map>
#include <set>

#include "utils.hpp"

namespace schema
{
    struct container {
    };
    struct list {
        std::set<std::string> m_keys;
    };
}

using Result = std::pair< bool, std::string >;

using NodeType = boost::variant<schema::container, schema::list>;


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

    Result isContainer(const std::string& location, const std::string& name) const;
    void addContainer(const std::string& location, const std::string& name);
    Result isList(const std::string& location, const std::string& name, const std::set<std::string>& keys) const;
    void addList(const std::string& location, const std::string& name, const std::set<std::string>& keys);
    void changeNode(const std::string& name);
    std::string currentNode() const;

private:
    const std::unordered_map<std::string, NodeType>& children(const std::string& name) const;

    std::unordered_map<std::string, std::unordered_map<std::string, NodeType>> m_nodes;
    std::string m_curDir;
};
