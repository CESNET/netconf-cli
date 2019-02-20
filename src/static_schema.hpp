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


/*! \class StaticSchema
 *     \brief Static schema, used mainly for testing
 *         */

class StaticSchema : public Schema {
public:
    StaticSchema();

    bool isContainer(const schemaPath_& location, const ModuleNodePair& node) const override;
    bool isModule(const schemaPath_& location, const std::string& name) const override;
    bool isLeaf(const schemaPath_& location, const ModuleNodePair& node) const override;
    bool isList(const schemaPath_& location, const ModuleNodePair& node) const override;
    bool isPresenceContainer(const schemaPath_& location, const ModuleNodePair& node) const override;
    bool leafEnumHasValue(const schemaPath_& location, const ModuleNodePair& node, const std::string& value) const override;
    bool listHasKey(const schemaPath_& location, const ModuleNodePair& node, const std::string& key) const override;
    bool nodeExists(const std::string& location, const std::string& node) const override;
    const std::set<std::string> listKeys(const schemaPath_& location, const ModuleNodePair& node) const override;
    yang::LeafDataTypes leafType(const schemaPath_& location, const ModuleNodePair& node) const override;
    const std::set<std::string> enumValues(const schemaPath_& location, const ModuleNodePair& node) const override;
    std::set<std::string> childNodes(const schemaPath_& path, const Recursion) const override;

    void addContainer(const std::string& location, const std::string& name, yang::ContainerTraits isPresence = yang::ContainerTraits::None);
    void addLeaf(const std::string& location, const std::string& name, const yang::LeafDataTypes& type);
    void addLeafEnum(const std::string& location, const std::string& name, std::set<std::string> enumValues);
    void addList(const std::string& location, const std::string& name, const std::set<std::string>& keys);
    void addModule(const std::string& name);

private:
    const std::unordered_map<std::string, NodeType>& children(const std::string& name) const;

    std::unordered_map<std::string, std::unordered_map<std::string, NodeType>> m_nodes;
    std::set<std::string> m_modules;
};
