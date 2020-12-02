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

namespace yang {
enum class NodeTypes {
    Container,
    PresenceContainer,
    List,
    Leaf,
    Rpc,
    Action,
    Notification,
    AnyXml,
    LeafList
};

enum class Status {
    Current,
    Deprecated,
    Obsolete
};
}

enum class Recursion {
    NonRecursive,
    Recursive
};


class InvalidNodeException : std::exception {
};

/*! \class Schema
 *     \brief A base schema class for schemas
 *         */

using ModuleNodePair = std::pair<boost::optional<std::string>, std::string>;

class Schema {
public:
    virtual ~Schema();

    [[nodiscard]] virtual yang::NodeTypes nodeType(const std::string& path) const = 0;
    [[nodiscard]] virtual yang::NodeTypes nodeType(const schemaPath_& location, const ModuleNodePair& node) const = 0;
    [[nodiscard]] virtual bool isModule(const std::string& name) const = 0;
    [[nodiscard]] virtual bool listHasKey(const schemaPath_& listPath, const std::string& key) const = 0;
    [[nodiscard]] virtual bool leafIsKey(const std::string& leafPath) const = 0;
    [[nodiscard]] virtual bool isConfig(const std::string& path) const = 0;
    [[nodiscard]] virtual std::optional<std::string> defaultValue(const std::string& leafPath) const = 0;
    [[nodiscard]] virtual const std::set<std::string> listKeys(const schemaPath_& listPath) const = 0;
    [[nodiscard]] virtual yang::TypeInfo leafType(const schemaPath_& location, const ModuleNodePair& node) const = 0;
    [[nodiscard]] virtual yang::TypeInfo leafType(const std::string& path) const = 0;
    [[nodiscard]] virtual std::optional<std::string> leafTypeName(const std::string& path) const = 0;
    [[nodiscard]] virtual std::string leafrefPath(const std::string& leafrefPath) const = 0;
    [[nodiscard]] virtual std::optional<std::string> description(const std::string& location) const = 0;
    [[nodiscard]] virtual yang::Status status(const std::string& location) const = 0;
    [[nodiscard]] virtual bool hasInputNodes(const std::string& path) const = 0;

    [[nodiscard]] virtual std::set<ModuleNodePair> availableNodes(const boost::variant<dataPath_, schemaPath_, module_>& path, const Recursion recursion) const = 0;
};
