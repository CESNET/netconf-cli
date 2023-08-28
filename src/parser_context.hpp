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
    std::string m_errorMsg;

    struct {
        schemaPath_ m_location;
        ModuleNodePair m_node;
    } m_tmpListKeyLeafPath;

    // When parsing list suffixes, this path is used to store the path of the list whose keys are being parsed.
    dataPath_ m_tmpListPath;
    ListInstance m_tmpListKeys;

    bool m_errorHandled = false;
    bool m_completing = false;

    std::set<Completion> m_suggestions;
    // Iterator pointing to where suggestions were created
    boost::optional<std::string::const_iterator> m_completionIterator;

    ParserContext copy() const;

private:
    boost::variant<dataPath_, schemaPath_> m_curPath;
};
