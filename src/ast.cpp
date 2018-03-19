/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/
#include "ast.hpp"


template <typename T, typename Iterator, typename Context>
inline void container_class::on_success(Iterator const& first, Iterator const& last
    , T& ast, Context const& context)
{
    ast.m_name = ast.m_first + ast.m_name;
    //std::cout <<"parsed " << ast.m_name << "(container)\n";
}

template <typename T, typename Iterator, typename Context>
inline void path_class::on_success(Iterator const& first, Iterator const& last
    , T& ast, Context const& context)
{
    //std::cout << "parsed path:" << std::endl;
    //for (auto it : ast.m_nodes)
    //std::cout << it.m_name << std::endl;
}

template <typename T, typename Iterator, typename Context>
inline void cd_class::on_success(Iterator const& first, Iterator const& last
    , T& ast, Context const& context)
{
    //std::cout << "parsed cd! final path:" << std::endl;
    //for (auto it : ast.m_path.m_nodes)
    //std::cout << it.m_name << std::endl;

}
