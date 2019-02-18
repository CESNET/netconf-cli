/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/

#include "schema.hpp"
struct ParserContext {
    ParserContext(const Schema& schema, const schemaPath_& curDir);
    const Schema& m_schema;
    schemaPath_ m_curPath;
    const schemaPath_ m_curPathOrig;
    boost::optional<std::string> m_curModule;
    std::string m_errorMsg;
    std::string m_tmpListName;
    bool m_topLevelModulePresent = false;
    std::set<std::string> m_tmpListKeys;
    bool m_errorHandled = false;
    bool m_completing = false;
    std::set<std::string> m_suggestions;
    // Iterator pointing to where suggestions were created
    std::string::const_iterator m_completionIterator;
};
