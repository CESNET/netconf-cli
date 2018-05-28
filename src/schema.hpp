/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/

#pragma once

#include <boost/variant.hpp>
#include <set>
#include <stdexcept>
#include <unordered_map>
#include "ast.hpp"

namespace yang {
struct container {
};
struct list {
    std::set<std::string> m_keys;
};
}



using NodeType = boost::variant<yang::container, yang::list>;


class InvalidNodeException : public std::invalid_argument {
public:
    using std::invalid_argument::invalid_argument;
    ~InvalidNodeException() override;
};

/*! \class Schema
 *     \brief The class representing the schema, that the user traverses.
 *         */
class Schema {
public:
    Schema();
    bool nodeExists(const std::string& location, const std::string& name) const;

    bool isContainer(const path_& location, const std::string& name) const;
    void addContainer(const std::string& location, const std::string& name);
    const std::set<std::string>& listKeys(const path_& location, const std::string& name) const;
    bool listHasKey(const path_& location, const std::string& name, const std::string& key) const;
    bool isList(const path_& location, const std::string& name) const;
    void addList(const std::string& location, const std::string& name, const std::set<std::string>& keys);

private:
    const std::unordered_map<std::string, NodeType>& children(const std::string& name) const;

    std::unordered_map<std::string, std::unordered_map<std::string, NodeType>> m_nodes;
};
