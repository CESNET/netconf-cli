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

void Interpreter::operator()(const commit_&) const
{
    m_datastore.commitChanges();
}

void Interpreter::operator()(const discard_&) const
{
    m_datastore.discardChanges();
}

void Interpreter::operator()(const set_& set) const
{
    m_datastore.setLeaf(absolutePathFromCommand(set), set.m_data);
}

void Interpreter::operator()(const get_& get) const
{
    auto items = m_datastore.getItems(absolutePathFromCommand(get));
    for (auto it : items) {
        std::cout << it.first << " = " << boost::apply_visitor(leafDataToString(), it.second) << std::endl;
    }
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
    auto recursion{Recursion::NonRecursive};
    for (auto it : ls.m_options) {
        if (it == LsOption::Recursive)
            recursion = Recursion::Recursive;
    }

    for (const auto& it : m_parser.availableNodes(ls.m_path, recursion))
        std::cout << it << std::endl;
}

template <typename T>
std::string Interpreter::absolutePathFromCommand(const T& command) const
{
    if (command.m_path.m_scope == Scope::Absolute)
        return "/" + pathToDataString(command.m_path);
    else
        return joinPaths(m_parser.currentNode(), pathToDataString(command.m_path));
}

struct pathToStringVisitor : boost::static_visitor<std::string> {
    std::string operator()(const schemaPath_& path) const
    {
        return pathToSchemaString(path);
    }
    std::string operator()(const dataPath_& path) const
    {
        return pathToDataString(path);
    }
};

struct getPathScopeVisitor : boost::static_visitor<Scope> {
    template <typename T>
    Scope operator()(const T& path) const
    {
        return path.m_scope;
    }
};

std::string Interpreter::absolutePathFromCommand(const get_& get) const
{
    if (!get.m_path) {
        return m_parser.currentNode();
    }

    const auto path = *get.m_path;
    std::string pathString = boost::apply_visitor(pathToStringVisitor(), path);
    auto pathScope{boost::apply_visitor(getPathScopeVisitor(), path)};

    if (pathScope == Scope::Absolute) {
        return "/" + pathString;
    } else {
        return joinPaths(m_parser.currentNode(), pathString);
    }
}

Interpreter::Interpreter(Parser& parser, DatastoreAccess& datastore)
    : m_parser(parser)
    , m_datastore(datastore)
{
}
