/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/
#include "utils.hpp"

std::string joinPaths(const std::string& prefix, const std::string& suffix)
{
    if (prefix.empty() || suffix.empty())
        return prefix + suffix;
    else
        return prefix + '/' + suffix;
}

std::string stripLastNodeFromPath(const std::string& path)
{
    std::string res = path;
    auto pos = res.find_last_of('/');
    if (pos == res.npos)
        res.clear();
    else
        res.erase(pos);
    return res;
}

path_ pathWithoutLastNode(const path_& path)
{
    return path_{decltype(path_::m_nodes)(path.m_nodes.begin(), path.m_nodes.end() - 1)};
}

std::string leafDataTypeToString(yang::LeafDataTypes type)
{
    switch (type)
    {
        case yang::LeafDataTypes::String:
            return "string";
        case yang::LeafDataTypes::Decimal:
            return "decimal";
        case yang::LeafDataTypes::Bool:
            return "bool";
        case yang::LeafDataTypes::Int:
            return "int";
        case yang::LeafDataTypes::Uint:
            return "uint";
        case yang::LeafDataTypes::Enum:
            return "enum";
        default:
            return "";
    }
}

