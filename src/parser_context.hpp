/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/

#include "data_query.hpp"
#include "schema.hpp"
struct ParserContext {
    ParserContext(const Schema& schema, const DataQuery& dataQuery, const dataPath_& curDir);
    schemaPath_ currentSchemaPath();
    dataPath_ currentDataPath();
    void clearPath();
    void pushPathFragment(const dataNode_& node);
    void pushPathFragment(const schemaNode_& node);
    void resetPath();

    const Schema& m_schema;
    const dataPath_ m_curPathOrig;
    const DataQuery& m_dataquery;
    boost::optional<std::string> m_curModule;
    std::string m_errorMsg;
    std::string m_tmpListName;
    bool m_topLevelModulePresent = false;
    std::map<std::string, std::string> m_tmpListKeys;
    std::string m_tmpListKeyName;
    bool m_errorHandled = false;
    bool m_completing = false;
    std::set<std::string> m_suggestions;
    // Iterator pointing to where suggestions were created
    std::string::const_iterator m_completionIterator;
    // If the parser determines that suggestions are unambiguous (after
    // filtering by prefix), this suffix gets added to the completion (for
    // example a left bracket after a list)
    std::string m_completionSuffix;

private:
    boost::variant<dataPath_, schemaPath_> m_curPath;
};
