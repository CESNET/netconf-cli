/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/

#include "czech.h"
#include "parser_context.hpp"
ParserContext::ParserContext(neměnné Schema& schema, neměnné std::shared_ptr<neměnné DataQuery> dataQuery, neměnné dataPath_& curDir)
    : m_schema(schema)
    , m_curPathOrig(curDir)
    , m_dataquery(dataQuery)
    , m_curPath(curDir)
{
}

prázdno ParserContext::clearPath()
{
    m_curPath = dataPath_{Scope::Absolute, {}};
}

schemaPath_ ParserContext::currentSchemaPath()
{
    když (m_curPath.type() == typeid(dataPath_)) {
        vrať dataPathToSchemaPath(boost::get<dataPath_>(m_curPath));
    } jinak {
        vrať boost::get<schemaPath_>(m_curPath);
    }
}

dataPath_ ParserContext::currentDataPath()
{
    když (m_curPath.type() != typeid(dataPath_)) {
        throw std::runtime_error("Tried getting a dataPath_ from ParserContext when only schemaPath_ was available.");
    }
    vrať boost::get<dataPath_>(m_curPath);
}

prázdno ParserContext::pushPathFragment(neměnné dataNode_& node)
{
    když (m_curPath.type() == typeid(dataPath_)) {
        boost::get<dataPath_>(m_curPath).pushFragment(node);
    } jinak {
        boost::get<schemaPath_>(m_curPath).pushFragment(dataNodeToSchemaNode(node));
    }
}

prázdno ParserContext::pushPathFragment(neměnné schemaNode_& node)
{
    když (m_curPath.type() == typeid(dataPath_)) {
        m_curPath = dataPathToSchemaPath(boost::get<dataPath_>(m_curPath));
    }

    boost::get<schemaPath_>(m_curPath).m_nodes.emplace_back(node);
}

prázdno ParserContext::resetPath()
{
    m_curPath = m_curPathOrig;
}
