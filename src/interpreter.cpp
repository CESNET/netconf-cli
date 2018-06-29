/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/

#include <iostream>
#include "interpreter.hpp"

struct leafDataToString : boost::static_visitor<std::string> {
    std::string operator()(const enum_& data) const
    {
        return data.m_value;
    }
    template <typename T>
    std::string operator()(const T& data) const
    {
        std::stringstream stream;
        stream << data;
        return stream.str();
    }
};

void Interpreter::operator()(const set_& set) const
{
    std::cout << "Setting " << pathToDataString(set.m_path) << " to " << boost::apply_visitor(leafDataToString(), set.m_data) << std::endl;
}

void Interpreter::operator()(const cd_& cd) const
{
    m_parser.changeNode(cd.m_path);
}

void Interpreter::operator()(const create_& create) const
{
    auto cont = boost::get<container_>(create.m_path.m_nodes.back().m_suffix);
    std::cout << "Presence container " << cont.m_name << " created." << std::endl;
}

void Interpreter::operator()(const delete_& delet) const
{
    auto cont = boost::get<container_>(delet.m_path.m_nodes.back().m_suffix);
    std::cout << "Presence container " << cont.m_name << " deleted." << std::endl;
}

void Interpreter::operator()(const ls_&) const
{
    std::cout << "Possible nodes:" << std::endl;

    for (const auto& it : m_parser.availableNodes())
        std::cout << it << std::endl;
}

Interpreter::Interpreter(Parser& parser, Schema&)
    : m_parser(parser)
{
}
