/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/

#include <iostream>
#include "datastore_access.hpp"
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
    m_datastore.setLeaf(absolutePathFromCommand(set), set.m_data);
}

void Interpreter::operator()(const cd_& cd) const
{
    m_parser.changeNode(cd.m_path);
}

void Interpreter::operator()(const create_& create) const
{
    m_datastore.createPresenceContainer(absolutePathFromCommand(create));
}

void Interpreter::operator()(const delete_& delet) const
{
    m_datastore.deletePresenceContainer(absolutePathFromCommand(delet));
}

void Interpreter::operator()(const ls_& ls) const
{
    std::cout << "Possible nodes:" << std::endl;

    for (const auto& it : m_parser.availableNodes(ls.m_path))
        std::cout << it << std::endl;
}

template <typename T>
std::string Interpreter::absolutePathFromCommand(const T& command) const
{
    return joinPaths(m_parser.currentNode(), pathToDataString(command.m_path));
}

Interpreter::Interpreter(Parser& parser, DatastoreAccess& datastore)
    : m_parser(parser), m_datastore(datastore)
{
}
