/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/

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

list_::list_(const std::string& listName)
    : m_name(listName)
{
}

bool schemaPath_::operator==(const schemaPath_& b) const
{
    if (this->m_nodes.size() != b.m_nodes.size())
        return false;
    return this->m_nodes == b.m_nodes;
}

bool dataPath_::operator==(const dataPath_& b) const
{
    if (this->m_nodes.size() != b.m_nodes.size())
        return false;
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
        if (it.m_prefix)
            res = joinPaths(res, it.m_prefix.value().m_name + ":" + std::visit(nodeToDataStringVisitor(), it.m_suffix));
        else
            res = joinPaths(res, (prefixes == Prefixes::Always ? path.m_nodes.at(0).m_prefix.value().m_name + ":" : "") + std::visit(nodeToDataStringVisitor(), it.m_suffix));
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
        if (it.m_prefix)
            res = joinPaths(res, it.m_prefix.value().m_name + ":" + std::visit(nodeToSchemaStringVisitor(), it.m_suffix));
        else
            res = joinPaths(res, (prefixes == Prefixes::Always ? path.m_nodes.at(0).m_prefix.value().m_name + ":" : "") + std::visit(nodeToSchemaStringVisitor(), it.m_suffix));
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
        where.push_back(what);
    }
}
}

void schemaPath_::pushFragment(const schemaNode_& fragment)
{
    impl_pushFragment(m_nodes, fragment);
}

void dataPath_::pushFragment(const dataNode_& fragment)
{
    impl_pushFragment(m_nodes, fragment);
}
