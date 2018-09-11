/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/

#include "static_schema.hpp"
#include "utils.hpp"

InvalidNodeException::~InvalidNodeException() = default;

StaticSchema::StaticSchema()
{
    m_nodes.emplace("", std::unordered_map<std::string, NodeType>());
}


const std::unordered_map<std::string, NodeType>& StaticSchema::children(const std::string& name) const
{
    return m_nodes.at(name);
}

bool StaticSchema::nodeExists(const std::string& location, const std::string& node) const
{
    if (node.empty())
        return true;
    const auto& childrenRef = children(location);

    return childrenRef.find(node) != childrenRef.end();
}

bool StaticSchema::isModule(const path_&, const std::string& name) const
{
    return m_modules.find(name) != m_modules.end();
}

bool StaticSchema::isContainer(const path_& location, const ModuleNodePair& node) const
{
    std::string locationString = pathToAbsoluteSchemaString(location);
    auto fullName = fullNodeName(location, node);
    if (!nodeExists(locationString, fullName))
        return false;

    return children(locationString).at(fullName).type() == typeid(yang::container);
}

void StaticSchema::addContainer(const std::string& location, const std::string& name, yang::ContainerTraits isPresence)
{
    m_nodes.at(location).emplace(name, yang::container{isPresence});

    //create a new set of children for the new node
    std::string key = joinPaths(location, name);
    m_nodes.emplace(key, std::unordered_map<std::string, NodeType>());
}


bool StaticSchema::listHasKey(const path_& location, const ModuleNodePair& node, const std::string& key) const
{
    std::string locationString = pathToAbsoluteSchemaString(location);
    assert(isList(location, node));

    const auto& child = children(locationString).at(fullNodeName(location, node));
    const auto& list = boost::get<yang::list>(child);
    return list.m_keys.find(key) != list.m_keys.end();
}

const std::set<std::string> StaticSchema::listKeys(const path_& location, const ModuleNodePair& node) const
{
    std::string locationString = pathToAbsoluteSchemaString(location);
    assert(isList(location, node));

    const auto& child = children(locationString).at(fullNodeName(location, node));
    const auto& list = boost::get<yang::list>(child);
    return list.m_keys;
}

bool StaticSchema::isList(const path_& location, const ModuleNodePair& node) const
{
    std::string locationString = pathToAbsoluteSchemaString(location);
    auto fullName = fullNodeName(location, node);
    if (!nodeExists(locationString, fullName))
        return false;
    const auto& child = children(locationString).at(fullName);
    if (child.type() != typeid(yang::list))
        return false;

    return true;
}

void StaticSchema::addList(const std::string& location, const std::string& name, const std::set<std::string>& keys)
{
    m_nodes.at(location).emplace(name, yang::list{keys});

    m_nodes.emplace(name, std::unordered_map<std::string, NodeType>());
}

bool StaticSchema::isPresenceContainer(const path_& location, const ModuleNodePair& node) const
{
    if (!isContainer(location, node))
        return false;
    std::string locationString = pathToAbsoluteSchemaString(location);
    return boost::get<yang::container>(children(locationString).at(fullNodeName(location, node))).m_presence == yang::ContainerTraits::Presence;
}

void StaticSchema::addLeaf(const std::string& location, const std::string& name, const yang::LeafDataTypes& type)
{
    m_nodes.at(location).emplace(name, yang::leaf{type, {}});
}

void StaticSchema::addLeafEnum(const std::string& location, const std::string& name, std::set<std::string> enumValues)
{
    m_nodes.at(location).emplace(name, yang::leaf{yang::LeafDataTypes::Enum, enumValues});
}

void StaticSchema::addModule(const std::string& name)
{
    m_modules.emplace(name);
}


bool StaticSchema::leafEnumHasValue(const path_& location, const ModuleNodePair& node, const std::string& value) const
{
    std::string locationString = pathToAbsoluteSchemaString(location);
    assert(isLeaf(location, node));

    const auto& child = children(locationString).at(fullNodeName(location, node));
    const auto& list = boost::get<yang::leaf>(child);
    return list.m_enumValues.find(value) != list.m_enumValues.end();
}

bool StaticSchema::isLeaf(const path_& location, const ModuleNodePair& node) const
{
    std::string locationString = pathToAbsoluteSchemaString(location);
    auto fullName = fullNodeName(location, node);
    if (!nodeExists(locationString, fullName))
        return false;

    return children(locationString).at(fullName).type() == typeid(yang::leaf);
}

yang::LeafDataTypes StaticSchema::leafType(const path_& location, const ModuleNodePair& node) const
{
    std::string locationString = pathToAbsoluteSchemaString(location);
    return boost::get<yang::leaf>(children(locationString).at(fullNodeName(location, node))).m_type;
}

std::set<std::string> StaticSchema::childNodesRec(const path_& path) const
{
    std::string locationString = pathToAbsoluteSchemaString(path);
    std::set<std::string> res;

    auto childrenRef = children(locationString);

    std::transform(childrenRef.begin(), childrenRef.end(),
                std::inserter(res, res.end()),
                [] (auto it) { return it.first; });
    return res;
}

std::set<std::string> StaticSchema::childNodes(const path_& path) const
{
    std::string locationString = pathToAbsoluteSchemaString(path);
    std::set<std::string> res;

    auto childrenRef = children(locationString);

    std::transform(childrenRef.begin(), childrenRef.end(),
                std::inserter(res, res.end()),
                [] (auto it) { return it.first; });
    return res;
}
