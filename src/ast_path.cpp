/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/

#include <algorithm>
#include <experimental/iterator>
#include <sstream>
#include "ast_path.hpp"
#include "utils.hpp"

container_::container_(const std::string& name)
    : m_name(name)
{
}

bool container_::operator==(const container_& b) const
{
    return this->m_name == b.m_name;
}

leaf_::leaf_(const std::string& name)
    : m_name(name)
{
}

bool leafListElement_::operator==(const leafListElement_& b) const
{
    return this->m_name == b.m_name && this->m_value == b.m_value;
}

leafList_::leafList_(const std::string& name)
    : m_name(name)
{
}

bool leafList_::operator==(const leafList_& b) const
{
    return this->m_name == b.m_name;
}

bool module_::operator==(const module_& b) const
{
    return this->m_name == b.m_name;
}

dataNode_::dataNode_() = default;

dataNode_::dataNode_(decltype(m_suffix) node)
    : m_suffix(node)
{
}

dataNode_::dataNode_(module_ module, decltype(m_suffix) node)
    : m_prefix(module)
    , m_suffix(node)
{
}

dataNode_::dataNode_(boost::optional<module_> module, decltype(m_suffix) node)
    : m_prefix(module)
    , m_suffix(node)
{
}

schemaNode_::schemaNode_(decltype(m_suffix) node)
    : m_suffix(node)
{
}

schemaNode_::schemaNode_(module_ module, decltype(m_suffix) node)
    : m_prefix(module)
    , m_suffix(node)
{
}

schemaNode_::schemaNode_() = default;

bool schemaNode_::operator==(const schemaNode_& b) const
{
    return this->m_suffix == b.m_suffix && this->m_prefix == b.m_prefix;
}

bool dataNode_::operator==(const dataNode_& b) const
{
    return this->m_suffix == b.m_suffix && this->m_prefix == b.m_prefix;
}

bool leaf_::operator==(const leaf_& b) const
{
    return this->m_name == b.m_name;
}

listElement_::listElement_(const std::string& listName, const ListInstance& keys)
    : m_name(listName)
    , m_keys(keys)
{
}

bool listElement_::operator==(const listElement_& b) const
{
    return (this->m_name == b.m_name && this->m_keys == b.m_keys);
}

bool list_::operator==(const list_& b) const
{
    return (this->m_name == b.m_name);
}

bool rpcNode_::operator==(const rpcNode_& other) const
{
    return this->m_name == other.m_name;
}

bool actionNode_::operator==(const actionNode_& other) const
{
    return this->m_name == other.m_name;
}

list_::list_(const std::string& listName)
    : m_name(listName)
{
}

namespace {
template <typename T, typename U>
auto findFirstOf(const std::vector<U>& nodes)
{
    return std::find_if(nodes.begin(), nodes.end(), [](const auto& e) {
        return std::holds_alternative<T>(e.m_suffix);
    });
}

template <typename T>
void validatePathNodes(const std::vector<T>& nodes)
{
    static_assert(std::is_same<T, dataNode_>() || std::is_same<T, schemaNode_>());

    if (nodes.empty()) {
        // there are default ctors, so it makes sense to specify the same thing via explicit args and not fail
        return;
    }

    if (auto firstLeaf = findFirstOf<leaf_>(nodes);
            firstLeaf != nodes.end() && firstLeaf != nodes.end() - 1) {
        throw std::logic_error{"Cannot put any extra nodes after a leaf"};
    }

    if (auto firstLeafList = findFirstOf<leafList_>(nodes);
            firstLeafList != nodes.end() && firstLeafList != nodes.end() - 1) {
        throw std::logic_error{"Cannot put any extra nodes after a leaf-list"};
    }

    if constexpr (std::is_same<T, dataNode_>()) {
        if (auto firstLeafListElements = findFirstOf<leafListElement_>(nodes);
                firstLeafListElements != nodes.end() && firstLeafListElements != nodes.end() - 1) {
            throw std::logic_error{"Cannot put any extra nodes after a leaf-list with element specification"};
        }
        if (auto firstList = findFirstOf<list_>(nodes);
                firstList != nodes.end() && firstList != nodes.end() - 1) {
            throw std::logic_error{
                "A list with no key specification can be present only as a last item in a dataPath. Did you mean to use a schemaPath?"
            };
        }
    }
}
}

schemaPath_::schemaPath_() = default;

schemaPath_::schemaPath_(const Scope scope, const std::vector<schemaNode_>& nodes)
    : m_scope(scope)
    , m_nodes(nodes)
{
    validatePathNodes(m_nodes);
}

bool schemaPath_::operator==(const schemaPath_& b) const
{
    if (this->m_nodes.size() != b.m_nodes.size()) {
        return false;
    }
    return this->m_nodes == b.m_nodes;
}

dataPath_::dataPath_() = default;

dataPath_::dataPath_(const Scope scope, const std::vector<dataNode_>& nodes)
    : m_scope(scope)
    , m_nodes(nodes)
{
    validatePathNodes(m_nodes);
}

bool dataPath_::operator==(const dataPath_& b) const
{
    if (this->m_nodes.size() != b.m_nodes.size()) {
        return false;
    }
    return this->m_nodes == b.m_nodes;
}

struct nodeToSchemaStringVisitor {
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

std::string escapeListKeyString(const std::string& what)
{
    // If we have both single and double quote, then we're screwed, but that "shouldn't happen"
    // in <= YANG 1.1 due to limitations in XPath 1.0.
    if (what.find('\'') != std::string::npos) {
        return '\"' + what + '\"';
    } else {
        return '\'' + what + '\'';
    }
}

struct nodeToDataStringVisitor {
    std::string operator()(const listElement_& node) const
    {
        std::ostringstream res;
        res << node.m_name + "[";
        std::transform(node.m_keys.begin(), node.m_keys.end(),
                std::experimental::make_ostream_joiner(res, "]["),
                [] (const auto& it) { return it.first + "=" + escapeListKeyString(leafDataToString(it.second)); });
        res << "]";
        return res.str();
    }
    std::string operator()(const leafListElement_& node) const
    {
        return node.m_name + "[.=" + escapeListKeyString(leafDataToString(node.m_value)) + "]";
    }
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

std::string nodeToSchemaString(decltype(dataPath_::m_nodes)::value_type node)
{
    return std::visit(nodeToSchemaStringVisitor(), node.m_suffix);
}

std::string pathToDataString(const dataPath_& path, Prefixes prefixes)
{
    std::string res;
    if (path.m_scope == Scope::Absolute) {
        res = "/";
    }

    for (const auto& it : path.m_nodes) {
        if (it.m_prefix) {
            res = joinPaths(res, it.m_prefix.value().m_name + ":" + std::visit(nodeToDataStringVisitor(), it.m_suffix));
        } else {
            res = joinPaths(res, (prefixes == Prefixes::Always && path.m_nodes.at(0).m_prefix ? path.m_nodes.at(0).m_prefix.value().m_name + ":" : "") + std::visit(nodeToDataStringVisitor(), it.m_suffix));
        }
    }

    return res;
}

std::string pathToSchemaString(const schemaPath_& path, Prefixes prefixes)
{
    std::string res;
    if (path.m_scope == Scope::Absolute) {
        res = "/";
    }

    for (const auto& it : path.m_nodes) {
        if (it.m_prefix) {
            res = joinPaths(res, it.m_prefix.value().m_name + ":" + std::visit(nodeToSchemaStringVisitor(), it.m_suffix));
        } else {
            res = joinPaths(res, (prefixes == Prefixes::Always && path.m_nodes.at(0).m_prefix ? path.m_nodes.at(0).m_prefix.value().m_name + ":" : "") + std::visit(nodeToSchemaStringVisitor(), it.m_suffix));
        }
    }
    return res;
}

std::string pathToSchemaString(const dataPath_& path, Prefixes prefixes)
{
    return pathToSchemaString(dataPathToSchemaPath(path), prefixes);
}

struct dataSuffixToSchemaSuffix {
    using ReturnType = decltype(schemaNode_::m_suffix);
    ReturnType operator()(const listElement_& listElement) const
    {
        return list_{listElement.m_name};
    }

    ReturnType operator()(const leafListElement_& leafListElement) const
    {
        return leafList_{leafListElement.m_name};
    }

    template <typename T>
    ReturnType operator()(const T& suffix) const
    {
        return suffix;
    }
};

schemaNode_ dataNodeToSchemaNode(const dataNode_& node)
{
    schemaNode_ res;
    res.m_prefix = node.m_prefix;
    res.m_suffix = std::visit(dataSuffixToSchemaSuffix(), node.m_suffix);
    return res;
}

schemaPath_ dataPathToSchemaPath(const dataPath_& path)
{
    schemaPath_ res{path.m_scope, {}};

    std::transform(path.m_nodes.begin(), path.m_nodes.end(),
                   std::back_inserter(res.m_nodes),
                   [](const dataNode_& node) { return dataNodeToSchemaNode(node); });

    return res;
}

namespace {
template <typename NodeType>
void impl_pushFragment(std::vector<NodeType>& where, const NodeType& what)
{
    if (std::holds_alternative<nodeup_>(what.m_suffix)) {
        if (!where.empty()) { // Allow going up, when already at root
            where.pop_back();
        }
    } else {
        where.emplace_back(what);
    }
}
}

void schemaPath_::pushFragment(const schemaNode_& fragment)
{
    impl_pushFragment(m_nodes, fragment);
    validatePathNodes(m_nodes);
}

void dataPath_::pushFragment(const dataNode_& fragment)
{
    impl_pushFragment(m_nodes, fragment);
    validatePathNodes(m_nodes);
}

dataPath_ realPath(const dataPath_& cwd, const dataPath_& newPath)
{
    if (newPath.m_scope == Scope::Absolute) {
        return newPath;
    }

    dataPath_ res = cwd;
    for (const auto& it : newPath.m_nodes) {
        res.pushFragment(it);
    }
    return res;
}
