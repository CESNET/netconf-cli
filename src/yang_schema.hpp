/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/

#pragma once

#include <functional>
#include <optional>
#include <set>
#include "ast_path.hpp"
#include "schema.hpp"

namespace libyang {
class Context;
class Schema_Node;
class Schema_Node_Leaf;
class Data_Node;
class Module;
}

/*! \class YangSchema
 *     \brief A schema class, which uses libyang for queries.
 *         */
class YangSchema : public Schema {
public:
    YangSchema();
    YangSchema(std::shared_ptr<libyang::Context> lyCtx);
    ~YangSchema() override;

    yang::NodeTypes nodeType(const std::string& path) const override;
    yang::NodeTypes nodeType(const schemaPath_& location, const ModuleNodePair& node) const override;
    bool isModule(const std::string& name) const override;
    bool listHasKey(const schemaPath_& location, const ModuleNodePair& node, const std::string& key) const override;
    bool leafIsKey(const std::string& leafPath) const override;
    bool isConfig(const std::string& path) const override;
    std::optional<std::string> defaultValue(const std::string& leafPath) const override;
    const std::set<std::string> listKeys(const schemaPath_& location, const ModuleNodePair& node) const override;
    yang::TypeInfo leafType(const schemaPath_& location, const ModuleNodePair& node) const override;
    yang::TypeInfo leafType(const std::string& path) const override;
    std::optional<std::string> leafTypeName(const std::string& path) const override;
    std::string leafrefPath(const std::string& leafrefPath) const override;
    std::set<ModuleNodePair> availableNodes(const boost::variant<dataPath_, schemaPath_, module_>& path, const Recursion recursion) const override;
    std::optional<std::string> description(const std::string& path) const override;
    yang::Status status(const std::string& location) const override;

    void registerModuleCallback(const std::function<std::string(const char*, const char*, const char*, const char*)>& clb);

    /** @short Loads a module called moduleName. */
    void loadModule(const std::string& moduleName);

    /** @short Enables a feature called featureName on a module called moduleName. */
    void enableFeature(const std::string& moduleName, const std::string& featureName);

    /** @short Adds a new module passed as a YANG string. */
    void addSchemaString(const char* schema);

    /** @short Adds a new module from a file. */
    void addSchemaFile(const char* filename);

    /** @short Adds a new directory for schema lookup. */
    void addSchemaDirectory(const char* directoryName);

    /** @short Creates a new data node from a path (to be used with NETCONF edit-config) */
    std::shared_ptr<libyang::Data_Node> dataNodeFromPath(const std::string& path, const std::optional<const std::string> value = std::nullopt) const;
    std::shared_ptr<libyang::Module> getYangModule(const std::string& name);

private:
    yang::TypeInfo impl_leafType(const std::shared_ptr<libyang::Schema_Node>& node) const;
    std::set<std::string> modules() const;

    /** @short Returns a set of nodes, that match the location and name criteria. */

    /** @short Returns a single Schema_Node if the criteria matches only one, otherwise nullptr. */
    std::shared_ptr<libyang::Schema_Node> getSchemaNode(const std::string& node) const;

    /** @short Returns a single Schema_Node if the criteria matches only one, otherwise nullptr. */
    std::shared_ptr<libyang::Schema_Node> getSchemaNode(const schemaPath_& location, const ModuleNodePair& node) const;
    std::shared_ptr<libyang::Context> m_context;

    std::shared_ptr<libyang::Schema_Node> impl_getSchemaNode(const std::string& node) const;
};
