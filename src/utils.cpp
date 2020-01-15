/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/
#include <boost/algorithm/string/predicate.hpp>
#include <sstream>
#include "utils.hpp"

std::string joinPaths(const std::string& prefix, const std::string& suffix)
{
    if (prefix.empty() || suffix.empty() || prefix == "/")
        return prefix + suffix;
    else
        return prefix + '/' + suffix;
}

std::string stripLastNodeFromPath(const std::string& path)
{
    std::string res = path;
    auto pos = res.find_last_of('/');
    if (pos == res.npos) { // path has no backslash - it's either empty, or is a relative path with one fragment
        res.clear();
    } else if (pos == 0) { // path has one backslash at the start - it's either "/" or "/one-path-fragment"
        return "/";
    } else {
        res.erase(pos);
    }
    return res;
}

schemaPath_ pathWithoutLastNode(const schemaPath_& path)
{
    return schemaPath_{path.m_scope, decltype(schemaPath_::m_nodes)(path.m_nodes.begin(), path.m_nodes.end() - 1)};
}

std::string leafDataTypeToString(yang::LeafDataTypes type)
{
    switch (type) {
    case yang::LeafDataTypes::String:
        return "a string";
    case yang::LeafDataTypes::Decimal:
        return "a decimal";
    case yang::LeafDataTypes::Bool:
        return "a boolean";
    case yang::LeafDataTypes::Int8:
        return "an 8-bit integer";
    case yang::LeafDataTypes::Uint8:
        return "an 8-bit unsigned integer";
    case yang::LeafDataTypes::Int16:
        return "a 16-bit integer";
    case yang::LeafDataTypes::Uint16:
        return "a 16-bit unsigned integer";
    case yang::LeafDataTypes::Int32:
        return "a 32-bit integer";
    case yang::LeafDataTypes::Uint32:
        return "a 32-bit unsigned integer";
    case yang::LeafDataTypes::Int64:
        return "a 64-bit integer";
    case yang::LeafDataTypes::Uint64:
        return "a 64-bit unsigned integer";
    case yang::LeafDataTypes::Enum:
        return "an enum";
    case yang::LeafDataTypes::IdentityRef:
        return "an identity";
    case yang::LeafDataTypes::LeafRef:
        return "a leafref";
    case yang::LeafDataTypes::Binary:
        return "a base64-encoded binary value";
    default:
        throw std::runtime_error("leafDataTypeToString: unsupported leaf data type");
    }
}

std::string fullNodeName(const schemaPath_& location, const ModuleNodePair& pair)
{
    if (!pair.first) {
        return location.m_nodes.at(0).m_prefix.value().m_name + ":" + pair.second;
    } else {
        return pair.first.value() + ":" + pair.second;
    }
}

std::string fullNodeName(const dataPath_& location, const ModuleNodePair& pair)
{
    return fullNodeName(dataPathToSchemaPath(location), pair);
}

std::set<std::string> filterByPrefix(const std::set<std::string>& set, const std::string_view prefix)
{
    std::set<std::string> filtered;
    std::copy_if(set.begin(), set.end(),
            std::inserter(filtered, filtered.end()),
            [prefix] (auto it) { return boost::starts_with(it, prefix); });
    return filtered;
}

struct leafDataToStringVisitor : boost::static_visitor<std::string> {
    std::string operator()(const enum_& data) const
    {
        return data.m_value;
    }

    std::string operator()(const binary_& data) const
    {
        return data.m_value;
    }

    std::string operator()(const identityRef_& data) const
    {
        return data.m_prefix.value().m_name + ":" + data.m_value;
    }

    std::string operator()(const special_& data) const
    {
        return specialValueToString(data);
    }

    std::string operator()(const std::string& data) const
    {
        return data;
    }

    std::string operator()(const bool& data) const
    {
        if (data)
            return "true";
        else
            return "false";
    }

    template <typename T>
    std::string operator()(const T& data) const
    {
        return std::to_string(data);
    }
};

std::string leafDataToString(const leaf_data_ value)
{
    return boost::apply_visitor(leafDataToStringVisitor(), value);
}
