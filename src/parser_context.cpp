/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/

#include "parser_context.hpp"
ParserContext::ParserContext(const Schema& schema, const path_ curDir)
    : m_schema(schema)
{
    m_curPath = curDir;

    if (!m_curPath.m_nodes.empty() && m_curPath.m_nodes.at(0).m_prefix)
        m_topLevelModulePresent = true;
}
