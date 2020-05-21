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
    m_nodes.emplace("/", std::unordered_map<std::string, NodeInfo>());
}

const std::unordered_map<std::string, NodeInfo>& StaticSchema::children(const std::string& name) const
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
    m_nodes.at(location).emplace(name, NodeInfo{yang::container{isPresence}, yang::AccessType::Writable});

    //create a new set of children for the new node
    std::string key = joinPaths(location, name);
    m_nodes.emplace(key, std::unordered_map<std::string, NodeInfo>());
}

bool StaticSchema::listHasKey(const schemaPath_& location, const ModuleNodePair& node, const std::string& key) const
{
    std::string locationString = pathToSchemaString(location, Prefixes::Always);
    assert(isList(location, node));

    const auto& child = children(locationString).at(fullNodeName(location, node));
    const auto& list = boost::get<yang::list>(child.m_nodeType);
    return list.m_keys.find(key) != list.m_keys.end();
}

const std::set<std::string> StaticSchema::listKeys(const schemaPath_& location, const ModuleNodePair& node) const
{
    std::string locationString = pathToSchemaString(location, Prefixes::Always);
    assert(isList(location, node));

    const auto& child = children(locationString).at(fullNodeName(location, node));
    const auto& list = boost::get<yang::list>(child.m_nodeType);
    return list.m_keys;
}

void StaticSchema::addList(const std::string& location, const std::string& name, const std::set<std::string>& keys)
{
    m_nodes.at(location).emplace(name, NodeInfo{yang::list{keys}, yang::AccessType::Writable});

    std::string key = joinPaths(location, name);
    m_nodes.emplace(key, std::unordered_map<std::string, NodeInfo>());
}

std::set<identityRef_> StaticSchema::validIdentities(std::string_view module, std::string_view value)
{
    std::set<identityRef_> identities;
    getIdentSet(identityRef_{std::string{module}, std::string{value}}, identities);

    return identities;
}

void StaticSchema::addLeaf(const std::string& location, const std::string& name, const yang::LeafDataType& type, const yang::AccessType accessType)
{
    m_nodes.at(location).emplace(name, NodeInfo{yang::leaf{yang::TypeInfo{type, std::nullopt}}, accessType});
    std::string key = joinPaths(location, name);
    m_nodes.emplace(key, std::unordered_map<std::string, NodeInfo>());
}

void StaticSchema::addModule(const std::string& name)
{
    m_modules.emplace(name);
}

void StaticSchema::addIdentity(const std::optional<identityRef_>& base, const identityRef_& name)
{
    if (base)
        m_identities.at(base.value()).emplace(name);

    m_identities.emplace(name, std::set<identityRef_>());
}

void StaticSchema::getIdentSet(const identityRef_& ident, std::set<identityRef_>& res) const
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
    return boost::get<yang::leaf>(children(locationString).at(fullNodeName(location, node)).m_nodeType).m_type;
}

yang::TypeInfo StaticSchema::leafType(const std::string& path) const
{
    auto locationString = stripLastNodeFromPath(path);
    auto node = lastNodeOfSchemaPath(path);
    return boost::get<yang::leaf>(children(locationString).at(node).m_nodeType).m_type;
}

std::set<ModuleNodePair> StaticSchema::availableNodes(const boost::variant<dataPath_, schemaPath_, module_>& path, const Recursion recursion) const
{
    if (recursion == Recursion::Recursive) {
        throw std::logic_error("Recursive StaticSchema::availableNodes is not implemented. It shouldn't be used in tests.");
    }

    std::set<ModuleNodePair> res;
    if (path.type() == typeid(module_)) {
        auto topLevelNodes = m_nodes.at("");
        auto modulePlusColon = boost::get<module_>(path).m_name + ":";
        for (const auto& it : topLevelNodes) {
            if (boost::algorithm::starts_with(it.first, modulePlusColon)) {
                res.insert(splitModuleNode(it.first));
            }
        }
        return res;
    }

    auto getTopLevelModule = [] (const auto& path) -> boost::optional<std::string> {
        if (!path.m_nodes.empty()) {
            return path.m_nodes.begin()->m_prefix.flat_map([] (const auto& module) {return boost::optional<std::string>(module.m_name);});
        }

        return boost::none;
    };

    std::string locationString;
    boost::optional<std::string> topLevelModule;
    if (path.type() == typeid(schemaPath_)) {
        locationString = pathToSchemaString(boost::get<schemaPath_>(path), Prefixes::Always);
        topLevelModule = getTopLevelModule(boost::get<schemaPath_>(path));
    } else {
        locationString = pathToSchemaString(boost::get<dataPath_>(path), Prefixes::Always);
        topLevelModule = getTopLevelModule(boost::get<dataPath_>(path));
    }

    auto childrenRef = children(locationString);

    std::transform(childrenRef.begin(), childrenRef.end(), std::inserter(res, res.end()), [path, topLevelModule](const auto& it) {
        auto res = splitModuleNode(it.first);
        if (topLevelModule == res.first) {
            res.first = boost::none;
        }
        return res;
    });
    return res;
}

yang::NodeTypes StaticSchema::nodeType(const schemaPath_& location, const ModuleNodePair& node) const
{
    std::string locationString = pathToSchemaString(location, Prefixes::Always);
    auto fullName = fullNodeName(location, node);
    try {
        auto targetNode = children(locationString).at(fullName);

        if (targetNode.m_nodeType.type() == typeid(yang::container)) {
            if (boost::get<yang::container>(targetNode.m_nodeType).m_presence == yang::ContainerTraits::Presence) {
                return yang::NodeTypes::PresenceContainer;
            }
            return yang::NodeTypes::Container;
        }

        if (targetNode.m_nodeType.type() == typeid(yang::list)) {
            return yang::NodeTypes::List;
        }

        if (targetNode.m_nodeType.type() == typeid(yang::leaf)) {
            return yang::NodeTypes::Leaf;
        }

        throw std::runtime_error{"YangSchema::nodeType: unsupported type"};

    } catch (std::out_of_range&) {
        throw InvalidNodeException();
    }
}

std::string fullNodeName(const std::string& location, const std::string& node)
{
    // If the node already contains a module name, just return it.
    if (node.find_first_of(':') != std::string::npos) {
        return node;
    }

    // Otherwise take the module name from the first node of location.
    return location.substr(location.find_first_not_of('/'), location.find_first_of(':') - 1) + ":" + node;
}

bool StaticSchema::isConfig(const std::string& leafPath) const
{
    auto locationString = stripLastNodeFromPath(leafPath);

    auto node = fullNodeName(locationString, lastNodeOfSchemaPath(leafPath));
    return children(locationString).at(node).m_configType == yang::AccessType::Writable;
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

std::optional<std::string> StaticSchema::defaultValue([[maybe_unused]] const std::string& leafPath) const
{
    throw std::runtime_error{"Internal error: StaticSchema::defaultValue(std::string) not implemented. The tests should not have called this overload."};
}
