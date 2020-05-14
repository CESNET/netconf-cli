/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/
#pragma once

#include "completion.hpp"
#include "data_query.hpp"
#include "schema.hpp"

enum Backtracking {
    Enabled,
    Disabled
};

struct ParserContext {
    ParserContext(const Schema& schema, const std::shared_ptr<const DataQuery> dataQuery, const dataPath_& curDir);
    schemaPath_ currentSchemaPath();
    dataPath_ currentDataPath();
    void clearPath();
    void pushPathFragment(const dataNode_& node);
    void pushPathFragment(const schemaNode_& node);
    void resetPath();

    const Schema& m_schema;
    const dataPath_ m_curPathOrig;
    const std::shared_ptr<const DataQuery> m_dataquery;
    boost::optional<std::string> m_curModule;
    std::string m_errorMsg;
    std::string m_tmpListName;
    bool m_topLevelModulePresent = false;

    struct {
        schemaPath_ m_location;
        ModuleNodePair m_node;
    } m_tmpListKeyLeafPath;

    std::map<std::string, leaf_data_> m_tmpListKeys;
    bool m_errorHandled = false;
    bool m_completing = false;

    std::set<Completion> m_suggestions;
    // Iterator pointing to where suggestions were created
    boost::optional<std::string::const_iterator> m_completionIterator;

    Backtracking m_pathBacktracking = Backtracking::Enabled;

private:
    boost::variant<dataPath_, schemaPath_> m_curPath;
};
