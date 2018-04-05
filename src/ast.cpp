/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/
#include <iostream>
#include "ast.hpp"
container_::container_(const std::string& name)
    : m_name(name)
{
}

bool container_::operator==(const container_& b) const
{
    return this->m_name == b.m_name;
}

listElement_::listElement_(const std::string& listName, const std::map<std::string, std::string>& keys)
    : m_listName(listName),
      m_keys(keys)
{
}

bool listElement_::operator==(const listElement_& b) const
{
    return (this->m_listName == b.m_listName && this->m_keys == b.m_keys);
}

bool path_::operator==(const path_& b) const
{
    if (this->m_nodes.size() != b.m_nodes.size())
        return false;
    std::cout << "command" << std::endl;
    for (auto it: b.m_nodes)
        std::cout << it.type().name()<< std::endl;
    std::cout << "expected" << std::endl;
    for (auto it: this->m_nodes)
        std::cout << it.type().name()<< std::endl;
    if(! (this->m_nodes.size() == b.m_nodes.size()))
        std::cout << "m_nodes not equal" << std::endl;
    std::cout << std::endl;
    return this->m_nodes == b.m_nodes;
}

bool cd_::operator==(const cd_& b) const
{
    if(! (this->m_path == b.m_path))
        std::cout << "m_path not equal" << std::endl;
    return this->m_path == b.m_path;
}


ParserContext::ParserContext(const CTree& tree)
    : m_tree(tree)
{
    m_curPath = m_tree.currentNode();
}
