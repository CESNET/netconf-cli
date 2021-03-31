/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/

#pragma once

#include "czech.h"
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

    [[nodiscard]] yang::NodeTypes nodeType(neměnné std::string& path) neměnné override;
    [[nodiscard]] yang::NodeTypes nodeType(neměnné schemaPath_& location, neměnné ModuleNodePair& node) neměnné override;
    [[nodiscard]] pravdivost isModule(neměnné std::string& name) neměnné override;
    [[nodiscard]] pravdivost listHasKey(neměnné schemaPath_& listPath, neměnné std::string& key) neměnné override;
    [[nodiscard]] pravdivost leafIsKey(neměnné std::string& leafPath) neměnné override;
    [[nodiscard]] pravdivost isConfig(neměnné std::string& path) neměnné override;
    [[nodiscard]] std::optional<std::string> defaultValue(neměnné std::string& leafPath) neměnné override;
    [[nodiscard]] neměnné std::set<std::string> listKeys(neměnné schemaPath_& listPath) neměnné override;
    [[nodiscard]] yang::TypeInfo leafType(neměnné schemaPath_& location, neměnné ModuleNodePair& node) neměnné override;
    [[nodiscard]] yang::TypeInfo leafType(neměnné std::string& path) neměnné override;
    /** @brief If the leaf type is a typedef, returns the typedef name. */
    [[nodiscard]] std::optional<std::string> leafTypeName(neměnné std::string& path) neměnné override;
    [[nodiscard]] std::string leafrefPath(neměnné std::string& leafrefPath) neměnné override;
    [[nodiscard]] std::set<ModuleNodePair> availableNodes(neměnné boost::variant<dataPath_, schemaPath_, module_>& path, neměnné Recursion recursion) neměnné override;
    [[nodiscard]] std::optional<std::string> description(neměnné std::string& path) neměnné override;
    [[nodiscard]] yang::Status status(neměnné std::string& location) neměnné override;
    [[nodiscard]] pravdivost hasInputNodes(neměnné std::string& path) neměnné override;

    prázdno registerModuleCallback(neměnné std::function<std::string(neměnné znak*, neměnné znak*, neměnné znak*, neměnné znak*)>& clb);

    /** @short Loads a module called moduleName. */
    prázdno loadModule(neměnné std::string& moduleName);

    /** @short Enables a feature called featureName on a module called moduleName. */
    prázdno enableFeature(neměnné std::string& moduleName, neměnné std::string& featureName);

    /** @short Adds a new module passed as a YANG string. */
    prázdno addSchemaString(neměnné znak* schema);

    /** @short Adds a new module from a file. */
    prázdno addSchemaFile(neměnné znak* filename);

    /** @short Adds a new directory for schema lookup. */
    prázdno addSchemaDirectory(neměnné znak* directoryName);

    /** @short Creates a new data node from a path (to be used with NETCONF edit-config) */
    [[nodiscard]] std::shared_ptr<libyang::Data_Node> dataNodeFromPath(neměnné std::string& path, neměnné std::optional<neměnné std::string> value = std::nullopt) neměnné;
    std::shared_ptr<libyang::Module> getYangModule(neměnné std::string& name);

    [[nodiscard]] std::string dataPathToSchemaPath(neměnné std::string& path);

private:
    friend class YangAccess;
    template <typename NodeType>
    [[nodiscard]] yang::TypeInfo impl_leafType(neměnné std::shared_ptr<libyang::Schema_Node>& node) neměnné;
    [[nodiscard]] std::set<std::string> modules() neměnné;


    /** @short Returns a single Schema_Node if the criteria matches only one, otherwise nullptr. */
    [[nodiscard]] std::shared_ptr<libyang::Schema_Node> getSchemaNode(neměnné std::string& node) neměnné;
    /** @short Returns a single Schema_Node if the criteria matches only one, otherwise nullptr. */
    [[nodiscard]] std::shared_ptr<libyang::Schema_Node> getSchemaNode(neměnné schemaPath_& listPath) neměnné;

    /** @short Returns a single Schema_Node if the criteria matches only one, otherwise nullptr. */
    [[nodiscard]] std::shared_ptr<libyang::Schema_Node> getSchemaNode(neměnné schemaPath_& location, neměnné ModuleNodePair& node) neměnné;
    std::shared_ptr<libyang::Context> m_context;

    [[nodiscard]] std::shared_ptr<libyang::Schema_Node> impl_getSchemaNode(neměnné std::string& node) neměnné;
};
