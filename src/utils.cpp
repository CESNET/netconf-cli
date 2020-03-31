/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/
#include <sstream>
#include "completion.hpp"
#include "utils.hpp"

std::string joinPaths(const std::string& prefix, const std::string& suffix)
{
    // These two if statements are essential for the algorithm:
    // The first one solves joining nothing and a relative path - the algorithm
    // down below adds a leading slash, turning it into an absolute path.
    // The second one would always add a trailing slash to the path.
    if (prefix.empty()) {
        return suffix;
    }

    if (suffix.empty()) {
        return prefix;
    }

    // Otherwise, strip slashes where the join is going to happen. This will
    // also change "/" to "", but the return statement takes care of that and
    // inserts the slash again.
    auto prefixWithoutTrailingSlash = !prefix.empty() && prefix.back() == '/' ? prefix.substr(0, prefix.length() - 1) : prefix;
    auto suffixWithoutLeadingSlash = !suffix.empty() && suffix.front() == '/' ? suffix.substr(1) : suffix;

    // And join the result with a slash.
    return prefixWithoutTrailingSlash + '/' + suffixWithoutLeadingSlash;
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

struct impl_leafDataTypeToString {
    std::string operator()(const yang::String)
    {
        return "a string";
    }
    std::string operator()(const yang::Decimal)
    {
        return "a decimal";
    }
    std::string operator()(const yang::Bool)
    {
        return "a boolean";
    }
    std::string operator()(const yang::Int8)
    {
        return "an 8-bit integer";
    }
    std::string operator()(const yang::Uint8)
    {
        return "an 8-bit unsigned integer";
    }
    std::string operator()(const yang::Int16)
    {
        return "a 16-bit integer";
    }
    std::string operator()(const yang::Uint16)
    {
        return "a 16-bit unsigned integer";
    }
    std::string operator()(const yang::Int32)
    {
        return "a 32-bit integer";
    }
    std::string operator()(const yang::Uint32)
    {
        return "a 32-bit unsigned integer";
    }
    std::string operator()(const yang::Int64)
    {
        return "a 64-bit integer";
    }
    std::string operator()(const yang::Uint64)
    {
        return "a 64-bit unsigned integer";
    }
    std::string operator()(const yang::Binary)
    {
        return "a base64-encoded binary value";
    }
    std::string operator()(const yang::Enum&)
    {
        return "an enum";
    }
    std::string operator()(const yang::IdentityRef&)
    {
        return "an identity";
    }
    std::string operator()(const yang::LeafRef&)
    {
        return "a leafref";
    }
};

std::string leafDataTypeToString(const yang::LeafDataType& type)
{
    return std::visit(impl_leafDataTypeToString{}, type);
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
