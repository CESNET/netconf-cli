/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/

#pragma once

#include <set>
#include <stdexcept>
#include <unordered_map>
#include "ast_path.hpp"
#include "schema.hpp"

class Context;
class Set;

/*! \class YangSchema
 *     \brief A schema class, which uses libyang for queries.
 *         */
#define OVERDRIVE override
class YangSchema : public Schema {
public:
    YangSchema();
    ~YangSchema() override {}

    bool isContainer(const path_& location, const ModuleNodePair& node) const override;
    bool isLeaf(const path_& location, const ModuleNodePair& node) const override;
    bool isModule(const path_& location, const std::string& name) const override;
    bool isList(const path_& location, const ModuleNodePair& node) const override;
    bool isPresenceContainer(const path_& location, const ModuleNodePair& node) const override;
    bool leafEnumHasValue(const path_& location, const ModuleNodePair& node, const std::string& value) const override;
    bool listHasKey(const path_& location, const ModuleNodePair& node, const std::string& key) const override;
    bool nodeExists(const std::string& location, const std::string& node) const override;
    const std::set<std::string> listKeys(const path_& location, const ModuleNodePair& node) const override;
    yang::LeafDataTypes leafType(const path_& location, const ModuleNodePair& node) const override;
    std::set<std::string> childNodes(const path_& path) const override;

    /** addSchemaString
     * Imports a new schema from a string.
     */
    void addSchemaString(const char* schema);

    /** addSchemaFile
     * Imports a new schema from a file.
     */
    void addSchemaFile(const char* filename);

    /** addSchemaDirectory
     * Add a new directory for schema lookup.
     */
    void addSchemaDirectory(const char* directoryName);

private:
    std::set<std::string> modules() const;
    bool nodeExists(const path_& location, const ModuleNodePair& node) const;

    /** getNodeSet
     * This method returns a set of nodes, that match the location and name criteria.
     */
    std::shared_ptr<Set> getNodeSet(const path_& location, const ModuleNodePair& node) const;
    std::shared_ptr<Context> m_context;
};
