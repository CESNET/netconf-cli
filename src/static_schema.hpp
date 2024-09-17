/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/

#pragma once

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

    yang::NodeTypes nodeType(const std::string& path) const override;
    yang::NodeTypes nodeType(const schemaPath_& location, const ModuleNodePair& node) const override;
    bool isModule(const std::string& name) const override;
    bool listHasKey(const schemaPath_& listPath, const std::string& key) const override;
    bool leafIsKey(const std::string& leafPath) const override;
    bool isConfig(const std::string& leafPath) const override;
    std::optional<std::string> defaultValue(const std::string& leafPath) const override;
    const std::set<std::string> listKeys(const schemaPath_& listPath) const override;
    yang::TypeInfo leafType(const schemaPath_& location, const ModuleNodePair& node) const override;
    yang::TypeInfo leafType(const std::string& path) const override;
    std::optional<std::string> leafTypeName(const std::string& path) const override;
    std::string leafrefPath(const std::string& leafrefPath) const override;
    std::set<ModuleNodePair> availableNodes(const boost::variant<dataPath_, schemaPath_, module_>& path, const Recursion recursion) const override;
    std::optional<std::string> description(const std::string& path) const override;
    yang::Status status(const std::string& location) const override;
    bool hasInputNodes(const std::string& path) const override;

    /** A helper for making tests a little bit easier. It returns all
     * identities which are based on the argument passed and which can then be
     * used in addLeaf for the `type` argument */
    std::set<identityRef_> validIdentities(const std::string& module, const std::string& value);
    void addContainer(const std::string& location, const std::string& name, yang::ContainerTraits isPresence = yang::ContainerTraits::None);
    void addLeaf(const std::string& location, const std::string& name, const yang::LeafDataType& type, const yang::AccessType accessType = yang::AccessType::Writable);
    void addLeafList(const std::string& location, const std::string& name, const yang::LeafDataType& type);
    void addList(const std::string& location, const std::string& name, const std::set<std::string>& keys);
    void addRpc(const std::string& location, const std::string& name);
    void addAction(const std::string& location, const std::string& name);
    void addModule(const std::string& name);
    void addIdentity(const std::optional<identityRef_>& base, const identityRef_& name);

private:
    const std::unordered_map<std::string, NodeInfo>& children(const std::string& name) const;
    void getIdentSet(const identityRef_& ident, std::set<identityRef_>& res) const;

    std::unordered_map<std::string, std::unordered_map<std::string, NodeInfo>> m_nodes;
    std::set<std::string> m_modules;

    std::map<identityRef_, std::set<identityRef_>> m_identities;
};
