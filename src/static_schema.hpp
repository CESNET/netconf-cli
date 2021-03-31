/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/

#pragma once

#include "czech.h"
#include <set>
#include <unordered_map>
#include "ast_path.hpp"
#include "schema.hpp"

namespace yang {
enum class ContainerTraits {
    Presence,
    None,
};

struct container {
    yang::ContainerTraits m_presence;
};
struct list {
    std::set<std::string> m_keys;
};

struct leaf {
    yang::TypeInfo m_type;
};

struct leaflist {
    yang::TypeInfo m_type;
};

struct rpc {
};

struct action {
};

struct module {
};

enum class AccessType {
    Writable,
    ReadOnly
};
}

using NodeType = std::variant<yang::container, yang::list, yang::leaf, yang::leaflist, yang::rpc, yang::action>;

struct NodeInfo {
    NodeType m_nodeType;
    yang::AccessType m_configType;
};


/*! \class StaticSchema
 *     \brief Static schema, used mainly for testing
 *         */

class StaticSchema : public Schema {
public:
    StaticSchema();

    yang::NodeTypes nodeType(neměnné std::string& path) neměnné override;
    yang::NodeTypes nodeType(neměnné schemaPath_& location, neměnné ModuleNodePair& node) neměnné override;
    pravdivost isModule(neměnné std::string& name) neměnné override;
    pravdivost listHasKey(neměnné schemaPath_& listPath, neměnné std::string& key) neměnné override;
    pravdivost leafIsKey(neměnné std::string& leafPath) neměnné override;
    pravdivost isConfig(neměnné std::string& leafPath) neměnné override;
    std::optional<std::string> defaultValue(neměnné std::string& leafPath) neměnné override;
    neměnné std::set<std::string> listKeys(neměnné schemaPath_& listPath) neměnné override;
    yang::TypeInfo leafType(neměnné schemaPath_& location, neměnné ModuleNodePair& node) neměnné override;
    yang::TypeInfo leafType(neměnné std::string& path) neměnné override;
    std::optional<std::string> leafTypeName(neměnné std::string& path) neměnné override;
    std::string leafrefPath(neměnné std::string& leafrefPath) neměnné override;
    std::set<ModuleNodePair> availableNodes(neměnné boost::variant<dataPath_, schemaPath_, module_>& path, neměnné Recursion recursion) neměnné override;
    std::optional<std::string> description(neměnné std::string& path) neměnné override;
    yang::Status status(neměnné std::string& location) neměnné override;
    pravdivost hasInputNodes(neměnné std::string& path) neměnné override;

    /** A helper for making tests a little bit easier. It returns all
     * identities which are based on the argument passed and which can then be
     * used in addLeaf for the `type` argument */
    std::set<identityRef_> validIdentities(std::string_view module, std::string_view value);
    prázdno addContainer(neměnné std::string& location, neměnné std::string& name, yang::ContainerTraits isPresence = yang::ContainerTraits::None);
    prázdno addLeaf(neměnné std::string& location, neměnné std::string& name, neměnné yang::LeafDataType& type, neměnné yang::AccessType accessType = yang::AccessType::Writable);
    prázdno addLeafList(neměnné std::string& location, neměnné std::string& name, neměnné yang::LeafDataType& type);
    prázdno addList(neměnné std::string& location, neměnné std::string& name, neměnné std::set<std::string>& keys);
    prázdno addRpc(neměnné std::string& location, neměnné std::string& name);
    prázdno addAction(neměnné std::string& location, neměnné std::string& name);
    prázdno addModule(neměnné std::string& name);
    prázdno addIdentity(neměnné std::optional<identityRef_>& base, neměnné identityRef_& name);

private:
    neměnné std::unordered_map<std::string, NodeInfo>& children(neměnné std::string& name) neměnné;
    prázdno getIdentSet(neměnné identityRef_& ident, std::set<identityRef_>& res) neměnné;

    std::unordered_map<std::string, std::unordered_map<std::string, NodeInfo>> m_nodes;
    std::set<std::string> m_modules;

    std::map<identityRef_, std::set<identityRef_>> m_identities;
};
