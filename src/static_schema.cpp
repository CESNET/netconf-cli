/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/

#include <boost/algorithm/string/predicate.hpp>
#include "static_schema.hpp"
#include "utils.hpp"

StaticSchema::StaticSchema()
{
    m_nodes.emplace("/", std::unordered_map<std::string, NodeType>());
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

bool StaticSchema::isModule(const std::string& name) const
{
    return m_modules.find(name) != m_modules.end();
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
    std::string locationString = pathToSchemaString(location, Prefixes::Always);
    assert(isList(location, node));

    const auto& child = children(locationString).at(fullNodeName(location, node));
    const auto& list = boost::get<yang::list>(child);
    return list.m_keys.find(key) != list.m_keys.end();
}

const std::set<std::string> StaticSchema::listKeys(const schemaPath_& location, const ModuleNodePair& node) const
{
    std::string locationString = pathToSchemaString(location, Prefixes::Always);
    assert(isList(location, node));

    const auto& child = children(locationString).at(fullNodeName(location, node));
    const auto& list = boost::get<yang::list>(child);
    return list.m_keys;
}

void StaticSchema::addList(const std::string& location, const std::string& name, const std::set<std::string>& keys)
{
    m_nodes.at(location).emplace(name, yang::list{keys});

    std::string key = joinPaths(location, name);
    m_nodes.emplace(key, std::unordered_map<std::string, NodeType>());
}

void StaticSchema::addLeaf(const std::string& location, const std::string& name, const yang::LeafDataTypes& type)
{
    m_nodes.at(location).emplace(name, yang::leaf{type, {}, {}, {}});
    std::string key = joinPaths(location, name);
    m_nodes.emplace(key, std::unordered_map<std::string, NodeType>());
}

void StaticSchema::addLeafEnum(const std::string& location, const std::string& name, std::set<std::string> enumValues)
{
    yang::leaf toAdd;
    toAdd.m_type = yang::LeafDataTypes::Enum;
    toAdd.m_enumValues = enumValues;
    m_nodes.at(location).emplace(name, toAdd);
    std::string key = joinPaths(location, name);
    m_nodes.emplace(key, std::unordered_map<std::string, NodeType>());
}

void StaticSchema::addLeafIdentityRef(const std::string& location, const std::string& name, const ModuleValuePair& base)
{
    assert(base.first); // base identity cannot have an empty module
    yang::leaf toAdd;
    toAdd.m_type = yang::LeafDataTypes::IdentityRef;
    toAdd.m_identBase = base;
    m_nodes.at(location).emplace(name, toAdd);
    std::string key = joinPaths(location, name);
    m_nodes.emplace(key, std::unordered_map<std::string, NodeType>());
}

void StaticSchema::addLeafRef(const std::string& location, const std::string& name, const std::string& source)
{
    yang::leaf toAdd;
    toAdd.m_type = yang::LeafDataTypes::LeafRef;
    toAdd.m_leafRefSource = source;
    m_nodes.at(location).emplace(name, toAdd);
    std::string key = joinPaths(location, name);
    m_nodes.emplace(key, std::unordered_map<std::string, NodeType>());
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
    std::string locationString = pathToSchemaString(location, Prefixes::Always);
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

std::string lastNodeOfSchemaPath(const std::string& path)
{
    std::string res = path;
    auto pos = res.find_last_of('/');
    if (pos == 0) { // path had only one path fragment - "/something:something"
        res.erase(0, 1);
        return res;
    }
    if (pos != res.npos) { // path had more fragments
        res.erase(0, pos);
        return res;
    }

    // path was empty
    return res;
}

yang::LeafDataTypes StaticSchema::leafrefBaseType(const schemaPath_& location, const ModuleNodePair& node) const
{
    std::string locationString = pathToSchemaString(location, Prefixes::Always);
    auto leaf{boost::get<yang::leaf>(children(locationString).at(fullNodeName(location, node)))};
    auto locationOfSource = stripLastNodeFromPath(leaf.m_leafRefSource);
    auto nameOfSource = lastNodeOfSchemaPath(leaf.m_leafRefSource);
    return boost::get<yang::leaf>(children(locationOfSource).at(nameOfSource)).m_type;
}

yang::LeafDataTypes StaticSchema::leafType(const schemaPath_& location, const ModuleNodePair& node) const
{
    std::string locationString = pathToSchemaString(location, Prefixes::Always);
    return boost::get<yang::leaf>(children(locationString).at(fullNodeName(location, node))).m_type;
}

yang::LeafDataTypes StaticSchema::leafType([[maybe_unused]] const std::string& path) const
{
    throw std::runtime_error{"StaticSchema::leafType not implemented"};
}

const std::set<std::string> StaticSchema::enumValues(const schemaPath_& location, const ModuleNodePair& node) const
{
    std::string locationString = pathToSchemaString(location, Prefixes::Always);
    assert(isLeaf(location, node));

    const auto& child = children(locationString).at(fullNodeName(location, node));
    const auto& leaf = boost::get<yang::leaf>(child);
    return leaf.m_enumValues;
}

// We do not test StaticSchema, so we don't need to implement recursive childNodes
// for this class.
std::set<std::string> StaticSchema::childNodes(const schemaPath_& path, const Recursion) const
{
    std::string locationString = pathToSchemaString(path, Prefixes::Always);
    std::set<std::string> res;

    auto childrenRef = children(locationString);

    std::transform(childrenRef.begin(), childrenRef.end(), std::inserter(res, res.end()), [](auto it) { return it.first; });
    return res;
}

// We do not test StaticSchema, so we don't need to implement recursive moduleNodes
// for this class.
std::set<std::string> StaticSchema::moduleNodes(const module_& module, const Recursion) const
{
    std::set<std::string> res;
    auto topLevelNodes = m_nodes.at("");
    auto modulePlusColon = module.m_name + ":";
    for (const auto& it : topLevelNodes) {
        if (boost::algorithm::starts_with(it.first, modulePlusColon)) {
            res.insert(it.first);
        }
    }
    return res;
}

yang::NodeTypes StaticSchema::nodeType(const schemaPath_& location, const ModuleNodePair& node) const
{
    std::string locationString = pathToSchemaString(location, Prefixes::Always);
    auto fullName = fullNodeName(location, node);
    try {
        auto targetNode = children(locationString).at(fullName);

        if (targetNode.type() == typeid(yang::container)) {
            if (boost::get<yang::container>(targetNode).m_presence == yang::ContainerTraits::Presence) {
                return yang::NodeTypes::PresenceContainer;
            }
            return yang::NodeTypes::Container;
        }

        if (targetNode.type() == typeid(yang::list)) {
            return yang::NodeTypes::List;
        }

        if (targetNode.type() == typeid(yang::leaf)) {
            return yang::NodeTypes::Leaf;
        }

        throw std::runtime_error{"YangSchema::nodeType: unsupported type"};

    } catch (std::out_of_range&) {
        throw InvalidNodeException();
    }
}

std::optional<std::string> StaticSchema::description([[maybe_unused]] const std::string& path) const
{
    throw std::runtime_error{"StaticSchema::description not implemented"};
}

std::optional<std::string> StaticSchema::units([[maybe_unused]] const std::string& path) const
{
    throw std::runtime_error{"StaticSchema::units not implemented"};
}

yang::NodeTypes StaticSchema::nodeType([[maybe_unused]] const std::string& path) const
{
    throw std::runtime_error{"Internal error: StaticSchema::nodeType(std::string) not implemented. The tests should not have called this overload."};
}

yang::LeafDataTypes StaticSchema::leafrefBaseType([[maybe_unused]] const std::string& path) const
{
    throw std::runtime_error{"Internal error: StaticSchema::leafrefBaseType(std::string) not implemented. The tests should not have called this overload."};
}
