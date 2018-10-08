/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/

#include <experimental/iterator>
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

listElement_::listElement_(const std::string& listName, const std::map<std::string, std::string>& keys)
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


struct nodeToSchemaStringVisitor : public boost::static_visitor<std::string> {
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

struct nodeToDataStringVisitor : public boost::static_visitor<std::string> {
    std::string operator()(const listElement_& node) const
    {
        std::ostringstream res;
        res << node.m_name + "[";
        std::transform(node.m_keys.begin(), node.m_keys.end(),
                std::experimental::make_ostream_joiner(res, ' '),
                [] (const auto& it) { return it.first + "=" + escapeListKeyString(it.second); });
        res << "]";
        return res.str();
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
    return boost::apply_visitor(nodeToSchemaStringVisitor(), node.m_suffix);
}

std::string pathToDataString(const dataPath_& path)
{
    std::string res;
    for (const auto it : path.m_nodes)
        if (it.m_prefix)
            res = joinPaths(res, it.m_prefix.value().m_name + ":" + boost::apply_visitor(nodeToDataStringVisitor(), it.m_suffix));
        else
            res = joinPaths(res, boost::apply_visitor(nodeToDataStringVisitor(), it.m_suffix));

    return res;
}

std::string pathToAbsoluteSchemaString(const dataPath_& path)
{
    return pathToAbsoluteSchemaString(dataPathToSchemaPath(path));
}

std::string pathToAbsoluteSchemaString(const schemaPath_& path)
{
    std::string res;
    if (path.m_nodes.empty()) {
        return "";
    }

    auto topLevelModule = path.m_nodes.at(0).m_prefix.value();
    for (const auto it : path.m_nodes) {
        if (it.m_prefix)
            res = joinPaths(res, it.m_prefix.value().m_name + ":" + boost::apply_visitor(nodeToSchemaStringVisitor(), it.m_suffix));
        else
            res = joinPaths(res, topLevelModule.m_name + ":" + boost::apply_visitor(nodeToSchemaStringVisitor(), it.m_suffix));
    }
    return res;
}

std::string pathToSchemaString(const dataPath_& path)
{
    std::string res;
    for (const auto it : path.m_nodes) {
        if (it.m_prefix)
            res = joinPaths(res, it.m_prefix.value().m_name + ":" + boost::apply_visitor(nodeToSchemaStringVisitor(), it.m_suffix));
        else
            res = joinPaths(res, boost::apply_visitor(nodeToSchemaStringVisitor(), it.m_suffix));
    }
    return res;
}

struct dataSuffixToSchemaSuffix : boost::static_visitor<decltype(schemaNode_::m_suffix)> {
    auto operator()(const listElement_& listElement) const
    {
        return list_{listElement.m_name};
    }

    template <typename T>
    auto operator()(const T& suffix) const
    {
        return suffix;
    }
};

schemaNode_ dataNodeToSchemaNode(const dataNode_& node)
{
    schemaNode_ res;
    res.m_prefix = node.m_prefix;
    res.m_suffix = boost::apply_visitor(dataSuffixToSchemaSuffix(), node.m_suffix);
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
