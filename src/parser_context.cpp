/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/

#include "parser_context.hpp"
ParserContext::ParserContext(const CTree& tree)
        : m_tree(tree)
{
    m_curPath = m_tree.currentNode();
}
