/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/

#pragma once

#include <filesystem>
#include <functional>
#include <libyang-cpp/Context.hpp>
#include <optional>
#include <set>
#include "ast_path.hpp"
#include "schema.hpp"

/*! \class YangSchema
 *     \brief A schema class, which uses libyang for queries.
 *         */
class YangSchema : public Schema {
public:
    YangSchema();
    YangSchema(libyang::Context lyCtx);
    ~YangSchema() override;

    [[nodiscard]] yang::NodeTypes nodeType(const std::string& path) const override;
    [[nodiscard]] yang::NodeTypes nodeType(const schemaPath_& location, const ModuleNodePair& node) const override;
    [[nodiscard]] bool isModule(const std::string& name) const override;
    [[nodiscard]] bool listHasKey(const schemaPath_& listPath, const std::string& key) const override;
    [[nodiscard]] bool leafIsKey(const std::string& leafPath) const override;
    [[nodiscard]] bool isConfig(const std::string& path) const override;
    [[nodiscard]] std::optional<std::string> defaultValue(const std::string& leafPath) const override;
    [[nodiscard]] const std::set<std::string> listKeys(const schemaPath_& listPath) const override;
    [[nodiscard]] yang::TypeInfo leafType(const schemaPath_& location, const ModuleNodePair& node) const override;
    [[nodiscard]] yang::TypeInfo leafType(const std::string& path) const override;
    /** @brief If the leaf type is a typedef, returns the typedef name. */
    [[nodiscard]] std::optional<std::string> leafTypeName(const std::string& path) const override;
    [[nodiscard]] std::string leafrefPath(const std::string& leafrefPath) const override;
    [[nodiscard]] std::set<ModuleNodePair> availableNodes(const boost::variant<dataPath_, schemaPath_, module_>& path, const Recursion recursion) const override;
    [[nodiscard]] std::optional<std::string> description(const std::string& path) const override;
    [[nodiscard]] yang::Status status(const std::string& location) const override;
    [[nodiscard]] bool hasInputNodes(const std::string& path) const override;

    void registerModuleCallback(const std::function<std::string(const std::string&, const std::optional<std::string>&, const std::optional<std::string>&, const std::optional<std::string>&)>& clb);

    /** @short Loads a module called moduleName. */
    void loadModule(const std::string& moduleName);

    /** @short Sets enabled features. */
    void setEnabledFeatures(const std::string& moduleName, const std::vector<std::string>& features);

    /** @short Adds a new module passed as a YANG string. */
    void addSchemaString(const char* schema);

    /** @short Adds a new module from a file. */
    void addSchemaFile(const std::filesystem::path& filename);

    /** @short Adds a new directory for schema lookup. */
    void addSchemaDirectory(const std::filesystem::path& directory);

    /** @short Creates a new data node from a path (to be used with NETCONF edit-config) */
    [[nodiscard]] libyang::CreatedNodes dataNodeFromPath(const std::string& path, const std::optional<const std::string> value = std::nullopt) const;
    std::optional<libyang::Module> getYangModule(const std::string& name);

    [[nodiscard]] std::string dataPathToSchemaPath(const std::string& path);

private:
    friend class YangAccess;
    template <typename NodeType>
    [[nodiscard]] yang::TypeInfo impl_leafType(const NodeType& node) const;
    [[nodiscard]] std::set<std::string> modules() const;


    /** @short Returns a single SchemaNode if the criteria matches only one, otherwise nullopt. */
    [[nodiscard]] std::optional<libyang::SchemaNode> getSchemaNode(const std::string& node) const;
    /** @short Returns a single Schema_Node if the criteria matches only one, otherwise nullopt. */
    [[nodiscard]] std::optional<libyang::SchemaNode> getSchemaNode(const schemaPath_& listPath) const;

    /** @short Returns a single Schema_Node if the criteria matches only one, otherwise nullptr. */
    [[nodiscard]] std::optional<libyang::SchemaNode> getSchemaNode(const schemaPath_& location, const ModuleNodePair& node) const;
    libyang::Context m_context;

    [[nodiscard]] std::optional<libyang::SchemaNode> impl_getSchemaNode(const std::string& node) const;
};
