/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/

#pragma once

#include <functional>
#include <set>
#include <stdexcept>
#include <unordered_map>
#include "ast_path.hpp"
#include "schema.hpp"

namespace libyang {
class Context;
class Set;
class Schema_Node;
class Data_Node;
}

/*! \class YangSchema
 *     \brief A schema class, which uses libyang for queries.
 *         */
class YangSchema : public Schema {
public:
    YangSchema();
    ~YangSchema() override;

    bool isContainer(const schemaPath_& location, const ModuleNodePair& node) const override;
    bool isLeaf(const schemaPath_& location, const ModuleNodePair& node) const override;
    bool isModule(const schemaPath_& location, const std::string& name) const override;
    bool isList(const schemaPath_& location, const ModuleNodePair& node) const override;
    bool isPresenceContainer(const schemaPath_& location, const ModuleNodePair& node) const override;
    bool leafEnumHasValue(const schemaPath_& location, const ModuleNodePair& node, const std::string& value) const override;
    bool listHasKey(const schemaPath_& location, const ModuleNodePair& node, const std::string& key) const override;
    bool nodeExists(const std::string& location, const std::string& node) const override;
    const std::set<std::string> listKeys(const schemaPath_& location, const ModuleNodePair& node) const override;
    yang::LeafDataTypes leafType(const schemaPath_& location, const ModuleNodePair& node) const override;
    const std::set<std::string> enumValues(const schemaPath_& location, const ModuleNodePair& node) const override;
    std::set<std::string> childNodes(const schemaPath_& path, const Recursion recursion) const override;

    void registerModuleCallback(const std::function<std::string(const char*, const char*, const char*)>& clb);

    /** @short Loads a module called moduleName. */
    void loadModule(const std::string& moduleName);

    /** @short Adds a new module passed as a YANG string. */
    void addSchemaString(const char* schema);

    /** @short Adds a new module from a file. */
    void addSchemaFile(const char* filename);

    /** @short Adds a new directory for schema lookup. */
    void addSchemaDirectory(const char* directoryName);

    std::shared_ptr<libyang::Data_Node> dataNodeFromPath(const std::string& path, const std::string& value) const;

private:
    std::set<std::string> modules() const;
    bool nodeExists(const schemaPath_& location, const ModuleNodePair& node) const;

    /** @short Returns a set of nodes, that match the location and name criteria. */
    std::shared_ptr<libyang::Set> getNodeSet(const schemaPath_& location, const ModuleNodePair& node) const;

    /** @short Returns a single Schema_Node if the criteria matches only one, otherwise nullptr. */
    std::shared_ptr<libyang::Schema_Node> getSchemaNode(const schemaPath_& location, const ModuleNodePair& node) const;
    std::shared_ptr<libyang::Context> m_context;
};
