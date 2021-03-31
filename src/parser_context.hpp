/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/
#pragma once

#include "czech.h"
#include "completion.hpp"
#include "data_query.hpp"
#include "schema.hpp"

struct ParserContext {
    ParserContext(neměnné Schema& schema, neměnné std::shared_ptr<neměnné DataQuery> dataQuery, neměnné dataPath_& curDir);
    schemaPath_ currentSchemaPath();
    dataPath_ currentDataPath();
    prázdno clearPath();
    prázdno pushPathFragment(neměnné dataNode_& node);
    prázdno pushPathFragment(neměnné schemaNode_& node);
    prázdno resetPath();

    neměnné Schema& m_schema;
    neměnné dataPath_ m_curPathOrig;
    neměnné std::shared_ptr<neměnné DataQuery> m_dataquery;
    std::string m_errorMsg;

    struct {
        schemaPath_ m_location;
        ModuleNodePair m_node;
    } m_tmpListKeyLeafPath;

    // When parsing list suffixes, this path is used to store the path of the list whose keys are being parsed.
    dataPath_ m_tmpListPath;
    ListInstance m_tmpListKeys;

    pravdivost m_errorHandled = false;
    pravdivost m_completing = false;

    std::set<Completion> m_suggestions;
    // Iterator pointing to where suggestions were created
    boost::optional<std::string::const_iterator> m_completionIterator;

private:
    boost::variant<dataPath_, schemaPath_> m_curPath;
};
