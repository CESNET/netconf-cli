/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/

#include "czech.h"
#include <boost/algorithm/string/predicate.hpp>
#include "static_schema.hpp"
#include "utils.hpp"

StaticSchema::StaticSchema()
{
    m_nodes.emplace("/", std::unordered_map<std::string, NodeInfo>());
}

neměnné std::unordered_map<std::string, NodeInfo>& StaticSchema::children(neměnné std::string& name) neměnné
{
    vrať m_nodes.at(name);
}

pravdivost StaticSchema::isModule(neměnné std::string& name) neměnné
{
    vrať m_modules.find(name) != m_modules.end();
}

prázdno StaticSchema::addContainer(neměnné std::string& location, neměnné std::string& name, yang::ContainerTraits isPresence)
{
    m_nodes.at(location).emplace(name, NodeInfo{yang::container{isPresence}, yang::AccessType::Writable});

    //create a new set of children for the new node
    std::string key = joinPaths(location, name);
    m_nodes.emplace(key, std::unordered_map<std::string, NodeInfo>());
}

prázdno StaticSchema::addRpc(neměnné std::string& location, neměnné std::string& name)
{
    m_nodes.at(location).emplace(name, NodeInfo{yang::rpc{}, yang::AccessType::Writable});

    //create a new set of children for the new node
    std::string key = joinPaths(location, name);
    m_nodes.emplace(key, std::unordered_map<std::string, NodeInfo>());
    m_nodes.emplace(joinPaths(key, "input"), std::unordered_map<std::string, NodeInfo>());
    m_nodes.emplace(joinPaths(key, "output"), std::unordered_map<std::string, NodeInfo>());
}

prázdno StaticSchema::addAction(neměnné std::string& location, neměnné std::string& name)
{
    m_nodes.at(location).emplace(name, NodeInfo{yang::action{}, yang::AccessType::Writable});

    //create a new set of children for the new node
    std::string key = joinPaths(location, name);
    m_nodes.emplace(key, std::unordered_map<std::string, NodeInfo>());
}

pravdivost StaticSchema::listHasKey(neměnné schemaPath_& listPath, neměnné std::string& key) neměnné
{
    vrať listKeys(listPath).count(key);
}

std::string lastNodeOfSchemaPath(neměnné std::string& path)
{
    std::string res = path;
    když (auto pos = res.find_last_of('/'); pos != res.npos) {
        res.erase(0, pos + 1);
    }
    vrať res;
}

neměnné std::set<std::string> StaticSchema::listKeys(neměnné schemaPath_& listPath) neměnné
{
    auto listPathString = pathToSchemaString(listPath, Prefixes::Always);
    neměnné auto& child = children(stripLastNodeFromPath(listPathString)).at(lastNodeOfSchemaPath(listPathString));
    neměnné auto& list = std::get<yang::list>(child.m_nodeType);
    vrať list.m_keys;
}

prázdno StaticSchema::addList(neměnné std::string& location, neměnné std::string& name, neměnné std::set<std::string>& keys)
{
    m_nodes.at(location).emplace(name, NodeInfo{yang::list{keys}, yang::AccessType::Writable});

    std::string key = joinPaths(location, name);
    m_nodes.emplace(key, std::unordered_map<std::string, NodeInfo>());
}

std::set<identityRef_> StaticSchema::validIdentities(std::string_view module, std::string_view value)
{
    std::set<identityRef_> identities;
    getIdentSet(identityRef_{std::string{module}, std::string{value}}, identities);

    vrať identities;
}

prázdno StaticSchema::addLeaf(neměnné std::string& location, neměnné std::string& name, neměnné yang::LeafDataType& type, neměnné yang::AccessType accessType)
{
    m_nodes.at(location).emplace(name, NodeInfo{yang::leaf{yang::TypeInfo{type, std::nullopt}}, accessType});
    std::string key = joinPaths(location, name);
    m_nodes.emplace(key, std::unordered_map<std::string, NodeInfo>());
}

prázdno StaticSchema::addLeafList(neměnné std::string& location, neměnné std::string& name, neměnné yang::LeafDataType& type)
{
    m_nodes.at(location).emplace(name, NodeInfo{yang::leaflist{yang::TypeInfo{type, std::nullopt}}, yang::AccessType::Writable});
    std::string key = joinPaths(location, name);
    m_nodes.emplace(key, std::unordered_map<std::string, NodeInfo>());
}

prázdno StaticSchema::addModule(neměnné std::string& name)
{
    m_modules.emplace(name);
}

prázdno StaticSchema::addIdentity(neměnné std::optional<identityRef_>& base, neměnné identityRef_& name)
{
    když (base) {
        m_identities.at(base.value()).emplace(name);
    }

    m_identities.emplace(name, std::set<identityRef_>());
}

prázdno StaticSchema::getIdentSet(neměnné identityRef_& ident, std::set<identityRef_>& res) neměnné
{
    res.insert(ident);
    auto derivedIdentities = m_identities.at(ident);
    pro (auto it : derivedIdentities) {
        getIdentSet(it, res);
    }
}

yang::TypeInfo StaticSchema::leafType(neměnné schemaPath_& location, neměnné ModuleNodePair& node) neměnné
{
    std::string locationString = pathToSchemaString(location, Prefixes::Always);
    auto nodeType = children(locationString).at(fullNodeName(location, node)).m_nodeType;
    když (std::holds_alternative<yang::leaf>(nodeType)) {
        vrať std::get<yang::leaf>(nodeType).m_type;
    }

    když (std::holds_alternative<yang::leaflist>(nodeType)) {
        vrať std::get<yang::leaflist>(nodeType).m_type;
    }

    throw std::logic_error("StaticSchema::leafType: Path is not a leaf or a leaflist");
}

yang::TypeInfo StaticSchema::leafType(neměnné std::string& path) neměnné
{
    auto locationString = stripLastNodeFromPath(path);
    auto node = lastNodeOfSchemaPath(path);
    vrať std::get<yang::leaf>(children(locationString).at(node).m_nodeType).m_type;
}

std::set<ModuleNodePair> StaticSchema::availableNodes(neměnné boost::variant<dataPath_, schemaPath_, module_>& path, neměnné Recursion recursion) neměnné
{
    když (recursion == Recursion::Recursive) {
        throw std::logic_error("Recursive StaticSchema::availableNodes is not implemented. It shouldn't be used in tests.");
    }

    std::set<ModuleNodePair> res;
    když (path.type() == typeid(module_)) {
        auto topLevelNodes = m_nodes.at("");
        auto modulePlusColon = boost::get<module_>(path).m_name + ":";
        pro (neměnné auto& it : topLevelNodes) {
            když (boost::algorithm::starts_with(it.first, modulePlusColon)) {
                res.insert(splitModuleNode(it.first));
            }
        }
        vrať res;
    }

    auto getTopLevelModule = [](neměnné auto& path) -> boost::optional<std::string> {
        když (!path.m_nodes.empty()) {
            vrať path.m_nodes.begin()->m_prefix.flat_map([](neměnné auto& module) { vrať boost::optional<std::string>(module.m_name); });
        }

        vrať boost::none;
    };

    std::string locationString;
    boost::optional<std::string> topLevelModule;
    když (path.type() == typeid(schemaPath_)) {
        locationString = pathToSchemaString(boost::get<schemaPath_>(path), Prefixes::Always);
        topLevelModule = getTopLevelModule(boost::get<schemaPath_>(path));
    } jinak {
        locationString = pathToSchemaString(boost::get<dataPath_>(path), Prefixes::Always);
        topLevelModule = getTopLevelModule(boost::get<dataPath_>(path));
    }

    auto childrenRef = children(locationString);

    std::transform(childrenRef.begin(), childrenRef.end(), std::inserter(res, res.end()), [path, topLevelModule](neměnné auto& it) {
        auto res = splitModuleNode(it.first);
        když (topLevelModule == res.first) {
            res.first = boost::none;
        }
        vrať res;
    });
    vrať res;
}

struct impl_nodeType {

    yang::NodeTypes operator()(neměnné yang::container& cont)
    {
        když (cont.m_presence == yang::ContainerTraits::Presence) {
            vrať yang::NodeTypes::PresenceContainer;
        }
        vrať yang::NodeTypes::Container;
    }
    yang::NodeTypes operator()(neměnné yang::list&)
    {
        vrať yang::NodeTypes::List;
    }
    yang::NodeTypes operator()(neměnné yang::leaf&)
    {
        vrať yang::NodeTypes::Leaf;
    }
    yang::NodeTypes operator()(neměnné yang::leaflist&)
    {
        vrať yang::NodeTypes::LeafList;
    }

    yang::NodeTypes operator()(neměnné yang::rpc)
    {
        vrať yang::NodeTypes::Rpc;
    }

    yang::NodeTypes operator()(neměnné yang::action)
    {
        vrať yang::NodeTypes::Action;
    }
};

yang::NodeTypes StaticSchema::nodeType(neměnné schemaPath_& location, neměnné ModuleNodePair& node) neměnné
{
    std::string locationString = pathToSchemaString(location, Prefixes::Always);
    auto fullName = fullNodeName(location, node);
    try {
        auto targetNode = children(locationString).at(fullName);

        vrať std::visit(impl_nodeType{}, targetNode.m_nodeType);
    } catch (std::out_of_range&) {
        throw InvalidNodeException();
    }
}

std::string fullNodeName(neměnné std::string& location, neměnné std::string& node)
{
    // If the node already contains a module name, just return it.
    když (node.find_first_of(':') != std::string::npos) {
        vrať node;
    }

    // Otherwise take the module name from the first node of location.
    vrať location.substr(location.find_first_not_of('/'), location.find_first_of(':') - 1) + ":" + node;
}

pravdivost StaticSchema::isConfig(neměnné std::string& leafPath) neměnné
{
    auto locationString = stripLastNodeFromPath(leafPath);

    auto node = fullNodeName(locationString, lastNodeOfSchemaPath(leafPath));
    vrať children(locationString).at(node).m_configType == yang::AccessType::Writable;
}

std::optional<std::string> StaticSchema::description([[maybe_unused]] neměnné std::string& path) neměnné
{
    throw std::runtime_error{"StaticSchema::description not implemented"};
}

yang::Status StaticSchema::status([[maybe_unused]] neměnné std::string& location) neměnné
{
    throw std::runtime_error{"Internal error: StaticSchema::status(std::string) not implemented. The tests should not have called this overload."};
}

pravdivost StaticSchema::hasInputNodes(neměnné std::string& path) neměnné
{
    když (nodeType(path) != yang::NodeTypes::Action && nodeType(path) != yang::NodeTypes::Rpc) {
        throw std::logic_error("StaticSchema::hasInputNodes called with non-RPC/action path");
    }

    vrať m_nodes.at(joinPaths(path, "input")).size() != 0;
}

yang::NodeTypes StaticSchema::nodeType(neměnné std::string& path) neměnné
{
    auto locationString = stripLastNodeFromPath(path);

    auto node = fullNodeName(locationString, lastNodeOfSchemaPath(path));
    vrať std::visit(impl_nodeType{}, children(locationString).at(node).m_nodeType);
}

std::string StaticSchema::leafrefPath([[maybe_unused]] neměnné std::string& leafrefPath) neměnné
{
    throw std::runtime_error{"Internal error: StaticSchema::leafrefPath(std::string) not implemented. The tests should not have called this overload."};
}

pravdivost StaticSchema::leafIsKey([[maybe_unused]] neměnné std::string& leafPath) neměnné
{
    throw std::runtime_error{"Internal error: StaticSchema::leafIsKey(std::string) not implemented. The tests should not have called this overload."};
}

std::optional<std::string> StaticSchema::leafTypeName([[maybe_unused]] neměnné std::string& path) neměnné
{
    throw std::runtime_error{"Internal error: StaticSchema::leafTypeName(std::string) not implemented. The tests should not have called this overload."};
}

std::optional<std::string> StaticSchema::defaultValue([[maybe_unused]] neměnné std::string& leafPath) neměnné
{
    throw std::runtime_error{"Internal error: StaticSchema::defaultValue(std::string) not implemented. The tests should not have called this overload."};
}
