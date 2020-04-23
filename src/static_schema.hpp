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

struct module {
};
}

using NodeType = boost::variant<yang::container, yang::list, yang::leaf, yang::module>;


/*! \class StaticSchema
 *     \brief Static schema, used mainly for testing
 *         */

class StaticSchema : public Schema {
public:
    StaticSchema();

    yang::NodeTypes nodeType(const std::string& path) const override;
    yang::NodeTypes nodeType(const schemaPath_& location, const ModuleNodePair& node) const override;
    bool isModule(const std::string& name) const override;
    bool listHasKey(const schemaPath_& location, const ModuleNodePair& node, const std::string& key) const override;
    bool leafIsKey(const std::string& leafPath) const override;
    bool isConfig(const std::string& leafPath) const override;
    std::optional<std::string> defaultValue(const std::string& leafPath) const override;
    const std::set<std::string> listKeys(const schemaPath_& location, const ModuleNodePair& node) const override;
    yang::TypeInfo leafType(const schemaPath_& location, const ModuleNodePair& node) const override;
    yang::TypeInfo leafType(const std::string& path) const override;
    std::optional<std::string> leafTypeName(const std::string& path) const override;
    std::string leafrefPath(const std::string& leafrefPath) const override;
    std::set<std::string> childNodes(const schemaPath_& path, const Recursion) const override;
    std::set<std::string> moduleNodes(const module_& module, const Recursion recursion) const override;
    std::optional<std::string> description(const std::string& path) const override;
    yang::Status status(const std::string& location) const override;

    /** A helper for making tests a little bit easier. It returns all
     * identities which are based on the argument passed and which can then be
     * used in addLeaf for the `type` argument */
    std::set<identityRef_> validIdentities(std::string_view module, std::string_view value);
    void addContainer(const std::string& location, const std::string& name, yang::ContainerTraits isPresence = yang::ContainerTraits::None);
    void addLeaf(const std::string& location, const std::string& name, const yang::LeafDataType& type);
    void addList(const std::string& location, const std::string& name, const std::set<std::string>& keys);
    void addModule(const std::string& name);
    void addIdentity(const std::optional<ModuleValuePair>& base, const ModuleValuePair& name);

private:
    const std::unordered_map<std::string, NodeType>& children(const std::string& name) const;
    void getIdentSet(const ModuleValuePair& ident, std::set<ModuleValuePair>& res) const;
    bool nodeExists(const std::string& location, const std::string& node) const;

    std::unordered_map<std::string, std::unordered_map<std::string, NodeType>> m_nodes;
    std::set<std::string> m_modules;

    // FIXME: Change the template arguments to identityRef_
    std::map<ModuleValuePair, std::set<ModuleValuePair>> m_identities;
};
