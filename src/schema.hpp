/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/

#pragma once

#include <boost/variant/variant.hpp>
#include <set>
#include <stdexcept>
#include <unordered_map>
#include "ast_path.hpp"
#include "leaf_data_type.hpp"

using ModuleValuePair = std::pair<boost::optional<std::string>, std::string>;

namespace yang {
enum class NodeTypes {
    Container,
    PresenceContainer,
    List,
    Leaf
};
}

enum class Recursion {
    NonRecursive,
    Recursive
};


class InvalidNodeException {
};

/*! \class Schema
 *     \brief A base schema class for schemas
 *         */

using ModuleNodePair = std::pair<boost::optional<std::string>, std::string>;

class Schema {
public:
    virtual ~Schema();

    bool isContainer(const schemaPath_& location, const ModuleNodePair& node) const;
    bool isLeaf(const schemaPath_& location, const ModuleNodePair& node) const;
    bool isList(const schemaPath_& location, const ModuleNodePair& node) const;
    bool isPresenceContainer(const schemaPath_& location, const ModuleNodePair& node) const;
    virtual yang::NodeTypes nodeType(const std::string& path) const = 0;
    virtual yang::NodeTypes nodeType(const schemaPath_& location, const ModuleNodePair& node) const = 0;
    virtual bool isModule(const std::string& name) const = 0;
    virtual bool listHasKey(const schemaPath_& location, const ModuleNodePair& node, const std::string& key) const = 0;
    virtual bool leafIsKey(const std::string& leafPath) const = 0;
    virtual const std::set<std::string> listKeys(const schemaPath_& location, const ModuleNodePair& node) const = 0;
    virtual yang::LeafDataType leafType(const schemaPath_& location, const ModuleNodePair& node) const = 0;
    virtual yang::LeafDataType leafType(const std::string& path) const = 0;
    virtual std::optional<std::string> leafTypeName(const std::string& path) const = 0;
    virtual std::string leafrefPath(const std::string& leafrefPath) const = 0;
    virtual std::optional<std::string> description(const std::string& location) const = 0;
    virtual std::optional<std::string> units(const std::string& location) const = 0;

    virtual std::set<std::string> childNodes(const schemaPath_& path, const Recursion recursion) const = 0;
    virtual std::set<std::string> moduleNodes(const module_& module, const Recursion recursion) const = 0;
};
