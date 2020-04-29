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

std::set<identityRef_> StaticSchema::validIdentities(std::string_view module, std::string_view value)
{
    std::set<ModuleValuePair> identities;
    getIdentSet(ModuleNodePair{boost::optional<std::string>{module}, value}, identities);
    std::set<identityRef_> res;

    std::transform(identities.begin(), identities.end(), std::inserter(res, res.end()), [](const auto& identity) {
        return identityRef_{*identity.first, identity.second};
    });

    return res;
}

void StaticSchema::addLeaf(const std::string& location, const std::string& name, const yang::LeafDataType& type)
{
    m_nodes.at(location).emplace(name, yang::leaf{yang::TypeInfo{type, std::nullopt}});
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

void StaticSchema::getIdentSet(const ModuleValuePair& ident, std::set<ModuleValuePair>& res) const
{
    res.insert(ident);
    auto derivedIdentities = m_identities.at(ident);
    for (auto it : derivedIdentities) {
        getIdentSet(it, res);
    }
}

std::string lastNodeOfSchemaPath(const std::string& path)
{
    std::string res = path;
    if (auto pos = res.find_last_of('/'); pos != res.npos) {
        res.erase(0, pos + 1);
    }
    return res;
}

yang::TypeInfo StaticSchema::leafType(const schemaPath_& location, const ModuleNodePair& node) const
{
    std::string locationString = pathToSchemaString(location, Prefixes::Always);
    return boost::get<yang::leaf>(children(locationString).at(fullNodeName(location, node))).m_type;
}

yang::TypeInfo StaticSchema::leafType(const std::string& path) const
{
    auto locationString = stripLastNodeFromPath(path);
    auto node = lastNodeOfSchemaPath(path);
    return boost::get<yang::leaf>(children(locationString).at(node)).m_type;
}

// We do not test StaticSchema, so we don't need to implement recursive childNodes
// for this class.
std::set<std::string> StaticSchema::availableNodes(const boost::variant<dataPath_, schemaPath_, module_>& path, [[maybe_unused]] const Recursion recursion) const
{
    std::set<std::string> res;
    if (path.type() == typeid(module_)) {
        auto topLevelNodes = m_nodes.at("");
        auto modulePlusColon = boost::get<module_>(path).m_name + ":";
        for (const auto& it : topLevelNodes) {
            if (boost::algorithm::starts_with(it.first, modulePlusColon)) {
                res.insert(it.first);
            }
        }
        return res;
    }

    std::string locationString =
        path.type() == typeid(schemaPath_) ?
        pathToSchemaString(boost::get<schemaPath_>(path), Prefixes::Always) :
        pathToSchemaString(boost::get<dataPath_>(path), Prefixes::Always);

    auto childrenRef = children(locationString);

    std::transform(childrenRef.begin(), childrenRef.end(), std::inserter(res, res.end()), [](auto it) { return it.first; });
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

yang::Status StaticSchema::status([[maybe_unused]] const std::string& location) const
{
    throw std::runtime_error{"Internal error: StaticSchema::status(std::string) not implemented. The tests should not have called this overload."};
}

yang::NodeTypes StaticSchema::nodeType([[maybe_unused]] const std::string& path) const
{
    throw std::runtime_error{"Internal error: StaticSchema::nodeType(std::string) not implemented. The tests should not have called this overload."};
}

std::string StaticSchema::leafrefPath([[maybe_unused]] const std::string& leafrefPath) const
{
    throw std::runtime_error{"Internal error: StaticSchema::leafrefPath(std::string) not implemented. The tests should not have called this overload."};
}

bool StaticSchema::leafIsKey([[maybe_unused]] const std::string& leafPath) const
{
    throw std::runtime_error{"Internal error: StaticSchema::leafIsKey(std::string) not implemented. The tests should not have called this overload."};
}

std::optional<std::string> StaticSchema::leafTypeName([[maybe_unused]] const std::string& path) const
{
    throw std::runtime_error{"Internal error: StaticSchema::leafTypeName(std::string) not implemented. The tests should not have called this overload."};
}

bool StaticSchema::isConfig([[maybe_unused]] const std::string& leafPath) const
{
    throw std::runtime_error{"Internal error: StaticSchema::isConfigLeaf(std::string) not implemented. The tests should not have called this overload."};
}

std::optional<std::string> StaticSchema::defaultValue([[maybe_unused]] const std::string& leafPath) const
{
    throw std::runtime_error{"Internal error: StaticSchema::defaultValue(std::string) not implemented. The tests should not have called this overload."};
}
