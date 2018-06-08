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
    std::string locationString = pathToSchemaString(location);
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
    std::string locationString = pathToSchemaString(location);
    assert(isList(location, name));

    const auto& child = children(locationString).at(name);
    const auto& list = boost::get<yang::list>(child);
    return list.m_keys.find(key) != list.m_keys.end();
}

const std::set<std::string>& Schema::listKeys(const path_& location, const std::string& name) const
{
    std::string locationString = pathToSchemaString(location);
    assert(isList(location, name));

    const auto& child = children(locationString).at(name);
    const auto& list = boost::get<yang::list>(child);
    return list.m_keys;
}

bool Schema::isList(const path_& location, const std::string& name) const
{
    std::string locationString = pathToSchemaString(location);
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
    std::string locationString = pathToSchemaString(location);
    return boost::get<yang::container>(children(locationString).at(name)).m_presence == yang::ContainerTraits::Presence;
}

void Schema::addLeaf(const std::string& location, const std::string& name, const yang::LeafDataTypes& type)
{
    m_nodes.at(location).emplace(name, yang::leaf{type});
}

void Schema::addLeafEnum(const std::string& location, const std::string& name, std::set<std::string> enumValues)
{
    m_nodes.at(location).emplace(name, yang::leaf{yang::LeafDataTypes::Enum, enumValues});
}

bool Schema::leafEnumHasValue(const path_& location, const std::string& name, const std::string& value) const
{
    std::string locationString = pathToSchemaString(location);
    assert(isLeaf(location, name));

    const auto& child = children(locationString).at(name);
    const auto& list = boost::get<yang::leaf>(child);
    return list.m_enumValues.find(value) != list.m_enumValues.end();
}

bool Schema::isLeaf(const path_& location, const std::string& name) const
{
    std::string locationString = pathToSchemaString(location);
    if (!nodeExists(locationString, name))
        return false;

    return children(locationString).at(name).type() == typeid(yang::leaf);
}

bool Schema::leafIsEnum(const path_& location, const std::string& name) const
{
    std::string locationString = pathToSchemaString(location);
    if (!nodeExists(locationString, name) || !isLeaf(location, name))
        return false;

    return boost::get<yang::leaf>(children(locationString).at(name)).m_type == yang::LeafDataTypes::Enum;
}

bool Schema::leafIsDecimal(const path_& location, const std::string& name) const
{
    std::string locationString = pathToSchemaString(location);
    if (!nodeExists(locationString, name) || !isLeaf(location, name))
        return false;

    return boost::get<yang::leaf>(children(locationString).at(name)).m_type == yang::LeafDataTypes::Decimal;

}

bool Schema::leafIsBool(const path_& location, const std::string& name) const
{
    std::string locationString = pathToSchemaString(location);
    if (!nodeExists(locationString, name) || !isLeaf(location, name))
        return false;

    return boost::get<yang::leaf>(children(locationString).at(name)).m_type == yang::LeafDataTypes::Bool;
}

bool Schema::leafIsInt(const path_& location, const std::string& name) const
{
    std::string locationString = pathToSchemaString(location);
    if (!nodeExists(locationString, name) || !isLeaf(location, name))
        return false;

    return boost::get<yang::leaf>(children(locationString).at(name)).m_type == yang::LeafDataTypes::Int;
}

bool Schema::leafIsUint(const path_& location, const std::string& name) const
{
    std::string locationString = pathToSchemaString(location);
    if (!nodeExists(locationString, name) || !isLeaf(location, name))
        return false;

    return boost::get<yang::leaf>(children(locationString).at(name)).m_type == yang::LeafDataTypes::Uint;
}

bool Schema::leafIsString(const path_& location, const std::string& name) const
{
    std::string locationString = pathToSchemaString(location);
    if (!nodeExists(locationString, name) || !isLeaf(location, name))
        return false;

    return boost::get<yang::leaf>(children(locationString).at(name)).m_type == yang::LeafDataTypes::String;
}
