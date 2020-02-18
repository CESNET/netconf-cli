
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

using ModuleValuePair = std::pair<boost::optional<std::string>, std::string>;

namespace yang {
enum class LeafDataTypes {
    String,
    Decimal,
    Bool,
    Int8,
    Uint8,
    Int16,
    Uint16,
    Int32,
    Uint32,
    Int64,
    Uint64,
    Enum,
    Binary,
    IdentityRef,
    LeafRef,
};

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
    virtual bool leafEnumHasValue(const schemaPath_& location, const ModuleNodePair& node, const std::string& value) const = 0;
    virtual bool leafIdentityIsValid(const schemaPath_& location, const ModuleNodePair& node, const ModuleValuePair& value) const = 0;
    virtual bool listHasKey(const schemaPath_& location, const ModuleNodePair& node, const std::string& key) const = 0;
    virtual const std::set<std::string> listKeys(const schemaPath_& location, const ModuleNodePair& node) const = 0;
    virtual yang::LeafDataTypes leafType(const schemaPath_& location, const ModuleNodePair& node) const = 0;
    virtual yang::LeafDataTypes leafrefBase(const schemaPath_& location, const ModuleNodePair& node) const = 0;

    virtual const std::set<std::string> validIdentities(const schemaPath_& location, const ModuleNodePair& node, const Prefixes prefixes) const = 0;
    virtual const std::set<std::string> enumValues(const schemaPath_& location, const ModuleNodePair& node) const = 0;
    virtual std::set<std::string> childNodes(const schemaPath_& path, const Recursion recursion) const = 0;
    virtual std::set<std::string> moduleNodes(const module_& module, const Recursion recursion) const = 0;
};
