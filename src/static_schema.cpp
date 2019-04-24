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

bool StaticSchema::isModule(const schemaPath_&, const std::string& name) const
{
    return m_modules.find(name) != m_modules.end();
}

bool StaticSchema::isContainer(const schemaPath_& location, const ModuleNodePair& node) const
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

bool StaticSchema::listHasKey(const schemaPath_& location, const ModuleNodePair& node, const std::string& key) const
{
    std::string locationString = pathToAbsoluteSchemaString(location);
    assert(isList(location, node));

    const auto& child = children(locationString).at(fullNodeName(location, node));
    const auto& list = boost::get<yang::list>(child);
    return list.m_keys.find(key) != list.m_keys.end();
}

const std::set<std::string> StaticSchema::listKeys(const schemaPath_& location, const ModuleNodePair& node) const
{
    std::string locationString = pathToAbsoluteSchemaString(location);
    assert(isList(location, node));

    const auto& child = children(locationString).at(fullNodeName(location, node));
    const auto& list = boost::get<yang::list>(child);
    return list.m_keys;
}

bool StaticSchema::isList(const schemaPath_& location, const ModuleNodePair& node) const
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

    std::string key = joinPaths(location, name);
    m_nodes.emplace(key, std::unordered_map<std::string, NodeType>());
}

bool StaticSchema::isPresenceContainer(const schemaPath_& location, const ModuleNodePair& node) const
{
    if (!isContainer(location, node))
        return false;
    std::string locationString = pathToAbsoluteSchemaString(location);
    return boost::get<yang::container>(children(locationString).at(fullNodeName(location, node))).m_presence == yang::ContainerTraits::Presence;
}

void StaticSchema::addLeaf(const std::string& location, const std::string& name, const yang::LeafDataTypes& type)
{
    m_nodes.at(location).emplace(name, yang::leaf{type, {}, {}});
}

void StaticSchema::addLeafEnum(const std::string& location, const std::string& name, std::set<std::string> enumValues)
{
    yang::leaf toAdd;
    toAdd.m_type = yang::LeafDataTypes::Enum;
    toAdd.m_enumValues = enumValues;
    m_nodes.at(location).emplace(name, toAdd);
}

void StaticSchema::addLeafIdentityRef(const std::string& location, const std::string& name, const ModuleValuePair& base)
{
    assert(base.first); // base identity cannot have an empty module
    yang::leaf toAdd;
    toAdd.m_type = yang::LeafDataTypes::IdentityRef;
    toAdd.m_identBase = base;
    m_nodes.at(location).emplace(name, toAdd);
}

void StaticSchema::addModule(const std::string& name)
{
    m_modules.emplace(name);
}

void StaticSchema::addIdentity(const std::optional<ModuleValuePair>& base, const ModuleValuePair& name)
{
    if (base)
        m_identities.at(base.value()).emplace(name);

    m_identities.emplace(name, std::set<ModuleValuePair>());
}

bool StaticSchema::leafEnumHasValue(const schemaPath_& location, const ModuleNodePair& node, const std::string& value) const
{
    auto enums = enumValues(location, node);
    return enums.find(value) != enums.end();
}

void StaticSchema::getIdentSet(const ModuleValuePair& ident, std::set<ModuleValuePair>& res) const
{
    res.insert(ident);
    auto derivedIdentities = m_identities.at(ident);
    for (auto it : derivedIdentities) {
        getIdentSet(it, res);
    }
}

const std::set<std::string> StaticSchema::validIdentities(const schemaPath_& location, const ModuleNodePair& node, const Prefixes prefixes) const
{
    std::string locationString = pathToAbsoluteSchemaString(location);
    assert(isLeaf(location, node));

    const auto& child = children(locationString).at(fullNodeName(location, node));
    const auto& leaf = boost::get<yang::leaf>(child);

    std::set<ModuleValuePair> identSet;
    getIdentSet(leaf.m_identBase, identSet);

    std::set<std::string> res;
    std::transform(identSet.begin(), identSet.end(), std::inserter(res, res.end()), [location, node, prefixes](const auto& it) {
        auto topLevelModule = location.m_nodes.empty() ? node.first.get() : location.m_nodes.front().m_prefix.get().m_name;
        std::string stringIdent;
        if (prefixes == Prefixes::Always || (it.first && it.first.value() != topLevelModule)) {
            stringIdent += it.first ? it.first.value() : topLevelModule;
            stringIdent += ":";
        }
        stringIdent += it.second;
        return stringIdent;
    });

    return res;
}

bool StaticSchema::leafIdentityIsValid(const schemaPath_& location, const ModuleNodePair& node, const ModuleValuePair& value) const
{
    auto identities = validIdentities(location, node, Prefixes::Always);

    auto topLevelModule = location.m_nodes.empty() ? node.first.get() : location.m_nodes.front().m_prefix.get().m_name;
    auto identModule = value.first ? value.first.value() : topLevelModule;
    return std::any_of(identities.begin(), identities.end(), [toFind = identModule + ":" + value.second](const auto& x) { return x == toFind; });
}

bool StaticSchema::isLeaf(const schemaPath_& location, const ModuleNodePair& node) const
{
    std::string locationString = pathToAbsoluteSchemaString(location);
    auto fullName = fullNodeName(location, node);
    if (!nodeExists(locationString, fullName))
        return false;

    return children(locationString).at(fullName).type() == typeid(yang::leaf);
}

// This method is just a stub, because leafrefs are not tested in StaticSchema;
yang::LeafDataTypes StaticSchema::leafrefBase(const schemaPath_&, const ModuleNodePair&) const
{
    return yang::LeafDataTypes::Binary;
}

yang::LeafDataTypes StaticSchema::leafType(const schemaPath_& location, const ModuleNodePair& node) const
{
    std::string locationString = pathToAbsoluteSchemaString(location);
    return boost::get<yang::leaf>(children(locationString).at(fullNodeName(location, node))).m_type;
}

const std::set<std::string> StaticSchema::enumValues(const schemaPath_& location, const ModuleNodePair& node) const
{
    std::string locationString = pathToAbsoluteSchemaString(location);
    assert(isLeaf(location, node));

    const auto& child = children(locationString).at(fullNodeName(location, node));
    const auto& leaf = boost::get<yang::leaf>(child);
    return leaf.m_enumValues;
}

// We do not test StaticSchema, so we don't need to implement recursive childNodes
// for this class.
std::set<std::string> StaticSchema::childNodes(const schemaPath_& path, const Recursion) const
{
    std::string locationString = pathToAbsoluteSchemaString(path);
    std::set<std::string> res;

    auto childrenRef = children(locationString);

    std::transform(childrenRef.begin(), childrenRef.end(),
                std::inserter(res, res.end()),
                [] (auto it) { return it.first; });
    return res;
}
