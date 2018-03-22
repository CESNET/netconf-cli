/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/
#include "ast.hpp"
container_::container_(const std::string& name)
    : m_name(name)
{
}

bool container_::operator==(const container_& b) const
{
    return this->m_name == b.m_name;
}

bool path_::operator==(const path_& b) const
{
    if (this->m_nodes.size() != b.m_nodes.size())
        return false;
    return this->m_nodes == b.m_nodes;
}

bool cd_::operator==(const cd_& b) const
{
    return this->m_path == b.m_path;
}


ParserContext::ParserContext(const CTree& tree)
    : m_tree(tree)
{
    m_curPath = m_tree.currentNode();
}

/*template <typename T, typename Iterator, typename Context>
void container_class::on_success(Iterator const& first, Iterator const& last
    , T& ast, Context const& context)
{
    ast.m_name = ast.m_first + ast.m_name;
    //std::cout <<"parsed " << ast.m_name << "(container)\n";
}

template <typename T, typename Iterator, typename Context>
void path_class::on_success(Iterator const& first, Iterator const& last
    , T& ast, Context const& context)
{
    //std::cout << "parsed path:" << std::endl;
    //for (auto it : ast.m_nodes)
    //std::cout << it.m_name << std::endl;
}

template <typename T, typename Iterator, typename Context>
void cd_class::on_success(Iterator const& first, Iterator const& last
    , T& ast, Context const& context)
{
    //std::cout << "parsed cd! final path:" << std::endl;
    //for (auto it : ast.m_path.m_nodes)
    //std::cout << it.m_name << std::endl;

}*/
