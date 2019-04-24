
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
#include "ast_path.hpp"

using ModuleValuePair = std::pair<boost::optional<std::string>, std::string>;

namespace yang {
enum class ContainerTraits {
    Presence,
    None,
};

enum class LeafDataTypes {
    String,
    Decimal,
    Bool,
    Int,
    Uint,
    Enum,
    Binary,
    IdentityRef,
    LeafRef, // only used in StaticSchema
};

struct container {
    yang::ContainerTraits m_presence;
};
struct list {
    std::set<std::string> m_keys;
};

struct leaf {
    yang::LeafDataTypes m_type;
    std::set<std::string> m_enumValues;
    ModuleValuePair m_identBase;
    std::string m_leafRefSource;
};

struct module {
};
}

enum class Recursion {
    NonRecursive,
    Recursive
};

enum class Prefixes {
    Always,
    WhenNeeded
};

using NodeType = boost::variant<yang::container, yang::list, yang::leaf, yang::module>;


class InvalidNodeException : public std::invalid_argument {
public:
    using std::invalid_argument::invalid_argument;
    ~InvalidNodeException() override;
};

/*! \class Schema
 *     \brief A base schema class for schemas
 *         */

using ModuleNodePair = std::pair<boost::optional<std::string>, std::string>;

class Schema {
public:
    virtual ~Schema();

    virtual bool isContainer(const schemaPath_& location, const ModuleNodePair& node) const = 0;
    virtual bool isLeaf(const schemaPath_& location, const ModuleNodePair& node) const = 0;
    virtual bool isModule(const schemaPath_& location, const std::string& name) const = 0;
    virtual bool isList(const schemaPath_& location, const ModuleNodePair& node) const = 0;
    virtual bool isPresenceContainer(const schemaPath_& location, const ModuleNodePair& node) const = 0;
    virtual bool leafEnumHasValue(const schemaPath_& location, const ModuleNodePair& node, const std::string& value) const = 0;
    virtual bool leafIdentityIsValid(const schemaPath_& location, const ModuleNodePair& node, const ModuleValuePair& value) const = 0;
    virtual bool listHasKey(const schemaPath_& location, const ModuleNodePair& node, const std::string& key) const = 0;
    virtual const std::set<std::string> listKeys(const schemaPath_& location, const ModuleNodePair& node) const = 0;
    virtual yang::LeafDataTypes leafType(const schemaPath_& location, const ModuleNodePair& node) const = 0;
    virtual const std::set<std::string> validIdentities(const schemaPath_& location, const ModuleNodePair& node, const Prefixes prefixes) const = 0;
    virtual const std::set<std::string> enumValues(const schemaPath_& location, const ModuleNodePair& node) const = 0;
    virtual std::set<std::string> childNodes(const schemaPath_& path, const Recursion recursion) const = 0;

private:
    const std::unordered_map<std::string, NodeType>& children(const std::string& name) const;

    std::unordered_map<std::string, std::unordered_map<std::string, NodeType>> m_nodes;
};
