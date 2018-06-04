/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/
#include "ast.hpp"


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

bool set_::operator==(const set_& b) const
{
    return this->m_path == b.m_path && this->m_data == b.m_data;
}

bool cd_::operator==(const cd_& b) const
{
    return this->m_path == b.m_path;
}

bool create_::operator==(const create_& b) const
{
    return this->m_path == b.m_path;
}

bool delete_::operator==(const delete_& b) const
{
    return this->m_path == b.m_path;
}
