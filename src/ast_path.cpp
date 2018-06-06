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


struct nodeToSchemaString : public boost::static_visitor<std::string> {
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
struct nodeToDataString : public boost::static_visitor<std::string> {
    std::string operator()(const listElement_& node) const
    {
        std::ostringstream res;
        res << node.m_name + "[";
        std::transform(node.m_keys.begin(), node.m_keys.end(),
                std::experimental::make_ostream_joiner(res, ' '),
                [] (const auto& it) { return it.first + "=" + it.second; });
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

std::string pathToDataString(const path_& path)
{
    std::string res;
    for (const auto it : path.m_nodes)
        res = joinPaths(res, boost::apply_visitor(nodeToDataString(), it));
    return res;
}

std::string pathToSchemaString(const path_& path)
{
    std::string res;
    for (const auto it : path.m_nodes)
        res = joinPaths(res, boost::apply_visitor(nodeToSchemaString(), it));
    return res;
}
