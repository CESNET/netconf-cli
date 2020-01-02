/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/

#include "parser_context.hpp"
ParserContext::ParserContext(const Schema& schema, const DataQuery& dataQuery, const dataPath_& curDir)
    : m_schema(schema)
    , m_curPathOrig(curDir)
    , m_dataquery(dataQuery)
    , m_curPath(curDir)
{
    if (!currentDataPath().m_nodes.empty() && currentDataPath().m_nodes.at(0).m_prefix)
        m_topLevelModulePresent = true;
}

void ParserContext::clearPath()
{
    m_curPath = dataPath_{Scope::Absolute, {}};
    m_topLevelModulePresent = false;
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
    auto pushNode = [this] (auto& where, const auto& what) {
        if (what.m_suffix.type() == typeid(nodeup_)) {
            where.m_nodes.pop_back();
            if (where.m_nodes.empty()) {
                m_topLevelModulePresent = false;
            }
        } else {
            where.m_nodes.push_back(what);
        }
    };

    if (m_curPath.type() == typeid(dataPath_)) {
        pushNode(boost::get<dataPath_>(m_curPath), node);
    } else {
        pushNode(boost::get<schemaPath_>(m_curPath), dataNodeToSchemaNode(node));
    }
}

void ParserContext::pushPathFragment(const schemaNode_& node)
{
    if (m_curPath.type() == typeid(dataPath_)) {
        m_curPath = dataPathToSchemaPath(boost::get<dataPath_>(m_curPath));
    }

    boost::get<schemaPath_>(m_curPath).m_nodes.push_back(node);
    if (currentSchemaPath().m_nodes.empty()) {
        m_topLevelModulePresent = false;
    }
}

void ParserContext::resetPath()
{
    m_curPath = m_curPathOrig;
    m_topLevelModulePresent = !currentDataPath().m_nodes.empty() && currentDataPath().m_nodes.at(0).m_prefix;
}
