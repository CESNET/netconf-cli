/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/

#include <boost/mpl/for_each.hpp>
#include <iostream>
#include "datastore_access.hpp"
#include "interpreter.hpp"
#include "utils.hpp"

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
    auto data = set.m_data;

    // If the user didn't supply a module prefix for identityref, we need to add it ourselves
    if (data.type() == typeid(identityRef_)) {
        auto identityRef = boost::get<identityRef_>(data);
        if (!identityRef.m_prefix) {
            identityRef.m_prefix = set.m_path.m_nodes.front().m_prefix.value();
            data = identityRef;
        }
    }
    m_datastore.setLeaf(absolutePathFromCommand(set), data);
}

void Interpreter::operator()(const get_& get) const
{
    auto items = m_datastore.getItems(absolutePathFromCommand(get));
    for (auto it : items) {
        std::cout << it.first << " = " << leafDataToString(it.second) << std::endl;
    }
}

void Interpreter::operator()(const cd_& cd) const
{
    m_parser.changeNode(cd.m_path);
}

void Interpreter::operator()(const create_& create) const
{
    if (create.m_path.m_nodes.back().m_suffix.type() == typeid(listElement_))
        m_datastore.createListInstance(absolutePathFromCommand(create));
    else
        m_datastore.createPresenceContainer(absolutePathFromCommand(create));
}

void Interpreter::operator()(const delete_& delet) const
{
    if (delet.m_path.m_nodes.back().m_suffix.type() == typeid(container_))
        m_datastore.deletePresenceContainer(absolutePathFromCommand(delet));
    else
        m_datastore.deleteListInstance(absolutePathFromCommand(delet));
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

struct commandLongHelpVisitor : boost::static_visitor<const char*> {
    template <typename T>
    auto constexpr operator()(boost::type<T>) const
    {
        return T::longHelp;
    }
};

struct commandShortHelpVisitor : boost::static_visitor<const char*> {
    template <typename T>
    auto constexpr operator()(boost::type<T>) const
    {
        return T::shortHelp;
    }
};

void Interpreter::operator()(const help_& help) const
{
    if (help.m_cmd)
        std::cout << boost::apply_visitor(commandLongHelpVisitor(), help.m_cmd.get()) << std::endl;
    else
        boost::mpl::for_each<CommandTypes, boost::type<boost::mpl::_>>([](auto cmd) {
            std::cout << commandShortHelpVisitor()(cmd) << std::endl;
        });
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
