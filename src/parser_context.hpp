/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/

#include "CTree.hpp"
struct ParserContext {
    ParserContext(const CTree& tree);
    const CTree& m_tree;
    std::string m_curPath;
    std::string m_errorMsg;
    std::string m_tmpListName;
    std::set<std::string> m_tmpListKeys;
    bool m_errorHandled = false;
};
