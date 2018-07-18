/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/

#pragma once

#include <boost/variant.hpp>
#include <libyang/Libyang.hpp>
#include <libyang/Tree_Schema.hpp>
#include <set>
#include <stdexcept>
#include <unordered_map>
#include "ast_path.hpp"
#include "schema.hpp"


/*! \class YangSchema
 *     \brief A schema class, which uses libyang for queries.
 *         */
#define OVERDRIVE override
class YangSchema : public Schema {
public:
    YangSchema();
    YangSchema(const char* filename);
    ~YangSchema() override {}

    bool isContainer(const path_& location, const ModuleNodePair& node) const override;
    bool isLeaf(const path_& location, const ModuleNodePair& node) const override;
    bool isModule(const path_& location, const std::string& name) const override;
    bool isList(const path_& location, const ModuleNodePair& node) const override;
    bool isPresenceContainer(const path_& location, const ModuleNodePair& node) const override;
    bool leafEnumHasValue(const path_& location, const ModuleNodePair& node, const std::string& value) const override;
    bool listHasKey(const path_& location, const ModuleNodePair& node, const std::string& key) const override;
    bool nodeExists(const std::string& location, const ModuleNodePair& node) const override;
    const std::set<std::string>& listKeys(const path_& location, const ModuleNodePair& node) const override;
    yang::LeafDataTypes leafType(const path_& location, const ModuleNodePair& node) const override;
    std::set<std::string> childNodes(const path_& path) const override;

private:
    std::set<std::string> modules() const override;

    S_Context m_context;
    S_Module m_moduleTEMP;
};
