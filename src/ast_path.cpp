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

node_::node_() = default;

node_::node_(decltype(m_suffix) node)
    : m_suffix(node)
{
}

node_::node_(module_ module, decltype(m_suffix) node)
    : m_prefix(module)
    , m_suffix(node)
{
}


bool node_::operator==(const node_& b) const
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

bool path_::operator==(const path_& b) const
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
struct nodeToDataStringVisitor : public boost::static_visitor<std::string> {
    std::string operator()(const listElement_& node) const
    {
        std::ostringstream res;
        res << node.m_name + "[";
        std::transform(node.m_keys.begin(), node.m_keys.end(),
                std::experimental::make_ostream_joiner(res, ' '),
                [] (const auto& it) { return it.first + "=" + '\'' + it.second + '\''; });
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

std::string nodeToSchemaString(decltype(path_::m_nodes)::value_type node)
{
    return boost::apply_visitor(nodeToSchemaStringVisitor(), node.m_suffix);
}

std::string pathToDataString(const path_& path)
{
    std::string res;
    for (const auto it : path.m_nodes)
        if (it.m_prefix)
            res = joinPaths(res, it.m_prefix.value().m_name + ":" + boost::apply_visitor(nodeToDataStringVisitor(), it.m_suffix));
        else
            res = joinPaths(res, boost::apply_visitor(nodeToDataStringVisitor(), it.m_suffix));

    return res;
}

std::string pathToAbsoluteSchemaString(const path_& path)
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

std::string pathToSchemaString(const path_& path)
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
