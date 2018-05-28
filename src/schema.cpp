/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/

#include "schema.hpp"
#include "utils.hpp"

InvalidNodeException::~InvalidNodeException() = default;

struct nodeToSchemaString : public boost::static_visitor<std::string> {
    std::string operator()(const nodeup_&) const
    {
        return "..";
    }
    template <class T>
    std::string operator()(const T& node) const
    {
        return node.m_name;
    }
};

std::string pathToString(const path_& path)
{
    std::string res;
    for (const auto it : path.m_nodes)
        res = joinPaths(res, boost::apply_visitor(nodeToSchemaString(), it));
    return res;
}

Schema::Schema()
{
    m_nodes.emplace("", std::unordered_map<std::string, NodeType>());
}

const std::unordered_map<std::string, NodeType>& Schema::children(const std::string& name) const
{
    return m_nodes.at(name);
}

bool Schema::nodeExists(const std::string& location, const std::string& name) const
{
    if (name.empty())
        return true;
    const auto& childrenRef = children(location);

    return childrenRef.find(name) != childrenRef.end();
}

bool Schema::isContainer(const path_& location, const std::string& name) const
{
    std::string locationString = pathToString(location);
    if (!nodeExists(locationString, name))
        return false;

    return children(locationString).at(name).type() == typeid(yang::container);
}

void Schema::addContainer(const std::string& location, const std::string& name, yang::ContainerTraits isPresence)
{
    m_nodes.at(location).emplace(name, yang::container{isPresence});

    //create a new set of children for the new node
    std::string key = joinPaths(location, name);
    m_nodes.emplace(key, std::unordered_map<std::string, NodeType>());
}


bool Schema::listHasKey(const path_& location, const std::string& name, const std::string& key) const
{
    std::string locationString = pathToString(location);
    assert(isList(location, name));

    const auto& child = children(locationString).at(name);
    const auto& list = boost::get<yang::list>(child);
    return list.m_keys.find(key) != list.m_keys.end();
}

const std::set<std::string>& Schema::listKeys(const path_& location, const std::string& name) const
{
    std::string locationString = pathToString(location);
    assert(isList(location, name));

    const auto& child = children(locationString).at(name);
    const auto& list = boost::get<yang::list>(child);
    return list.m_keys;
}

bool Schema::isList(const path_& location, const std::string& name) const
{
    std::string locationString = pathToString(location);
    if (!nodeExists(locationString, name))
        return false;
    const auto& child = children(locationString).at(name);
    if (child.type() != typeid(yang::list))
        return false;

    return true;
}

void Schema::addList(const std::string& location, const std::string& name, const std::set<std::string>& keys)
{
    m_nodes.at(location).emplace(name, yang::list{keys});

    m_nodes.emplace(name, std::unordered_map<std::string, NodeType>());
}

bool Schema::isPresenceContainer(const path_& location, const std::string& name) const
{
    if (!isContainer(location, name))
        return false;
    std::string locationString = pathToString(location);
    return boost::get<yang::container>(children(locationString).at(name)).m_presence == yang::ContainerTraits::Presence;
}
