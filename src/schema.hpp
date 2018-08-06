
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
};

struct module {
};
}


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

    virtual bool isContainer(const path_& location, const ModuleNodePair& node) const = 0;
    virtual bool isLeaf(const path_& location, const ModuleNodePair& node) const = 0;
    virtual bool isModule(const path_& location, const std::string& name) const = 0;
    virtual bool isList(const path_& location, const ModuleNodePair& node) const = 0;
    virtual bool isPresenceContainer(const path_& location, const ModuleNodePair& node) const = 0;
    virtual bool leafEnumHasValue(const path_& location, const ModuleNodePair& node, const std::string& value) const = 0;
    virtual bool listHasKey(const path_& location, const ModuleNodePair& node, const std::string& key) const = 0;
    virtual bool nodeExists(const std::string& location, const std::string& node) const = 0;
    virtual const std::set<std::string> listKeys(const path_& location, const ModuleNodePair& node) const = 0;
    virtual yang::LeafDataTypes leafType(const path_& location, const ModuleNodePair& node) const = 0;
    virtual std::set<std::string> childNodes(const path_& path) const = 0;

private:
    const std::unordered_map<std::string, NodeType>& children(const std::string& name) const;

    std::unordered_map<std::string, std::unordered_map<std::string, NodeType>> m_nodes;
};
