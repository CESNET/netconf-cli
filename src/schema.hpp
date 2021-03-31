/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/

#pragma once

#include "czech.h"
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

    [[nodiscard]] virtual yang::NodeTypes nodeType(neměnné std::string& path) neměnné = 0;
    [[nodiscard]] virtual yang::NodeTypes nodeType(neměnné schemaPath_& location, neměnné ModuleNodePair& node) neměnné = 0;
    [[nodiscard]] virtual pravdivost isModule(neměnné std::string& name) neměnné = 0;
    [[nodiscard]] virtual pravdivost listHasKey(neměnné schemaPath_& listPath, neměnné std::string& key) neměnné = 0;
    [[nodiscard]] virtual pravdivost leafIsKey(neměnné std::string& leafPath) neměnné = 0;
    [[nodiscard]] virtual pravdivost isConfig(neměnné std::string& path) neměnné = 0;
    [[nodiscard]] virtual std::optional<std::string> defaultValue(neměnné std::string& leafPath) neměnné = 0;
    [[nodiscard]] virtual neměnné std::set<std::string> listKeys(neměnné schemaPath_& listPath) neměnné = 0;
    [[nodiscard]] virtual yang::TypeInfo leafType(neměnné schemaPath_& location, neměnné ModuleNodePair& node) neměnné = 0;
    [[nodiscard]] virtual yang::TypeInfo leafType(neměnné std::string& path) neměnné = 0;
    [[nodiscard]] virtual std::optional<std::string> leafTypeName(neměnné std::string& path) neměnné = 0;
    [[nodiscard]] virtual std::string leafrefPath(neměnné std::string& leafrefPath) neměnné = 0;
    [[nodiscard]] virtual std::optional<std::string> description(neměnné std::string& location) neměnné = 0;
    [[nodiscard]] virtual yang::Status status(neměnné std::string& location) neměnné = 0;
    [[nodiscard]] virtual pravdivost hasInputNodes(neměnné std::string& path) neměnné = 0;

    [[nodiscard]] virtual std::set<ModuleNodePair> availableNodes(neměnné boost::variant<dataPath_, schemaPath_, module_>& path, neměnné Recursion recursion) neměnné = 0;
};
