/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/

#include "parser_context.hpp"
ParserContext::ParserContext(const Schema& schema, const std::shared_ptr<const DataQuery> dataQuery, const dataPath_& curDir)
    : m_schema(schema)
    , m_curPathOrig(curDir)
    , m_dataquery(dataQuery)
    , m_curPath(curDir)
{
}

void ParserContext::clearPath()
{
    m_curPath = dataPath_{Scope::Absolute, {}};
}

schemaPath_ ParserContext::currentSchemaPath()
{
    if (m_curPath.type() == typeid(dataPath_)) {
        return dataPathToSchemaPath(boost::get<dataPath_>(m_curPath));
    } else {
        return boost::get<schemaPath_>(m_curPath);
    }
}

dataPath_ ParserContext::currentDataPath()
{
    if (m_curPath.type() != typeid(dataPath_)) {
        throw std::runtime_error("Tried getting a dataPath_ from ParserContext when only schemaPath_ was available.");
    }
    return boost::get<dataPath_>(m_curPath);
}

void ParserContext::pushPathFragment(const dataNode_& node)
{
    if (m_curPath.type() == typeid(dataPath_)) {
        boost::get<dataPath_>(m_curPath).pushFragment(node);
    } else {
        boost::get<schemaPath_>(m_curPath).pushFragment(dataNodeToSchemaNode(node));
    }
}

void ParserContext::pushPathFragment(const schemaNode_& node)
{
    if (m_curPath.type() == typeid(dataPath_)) {
        m_curPath = dataPathToSchemaPath(boost::get<dataPath_>(m_curPath));
    }

    boost::get<schemaPath_>(m_curPath).m_nodes.emplace_back(node);
}

void ParserContext::resetPath()
{
    m_curPath = m_curPathOrig;
}

ParserContext ParserContext::copy() const
{
    return ParserContext{m_schema, m_dataquery, dataPath_{Scope::Absolute, {}}};
}
