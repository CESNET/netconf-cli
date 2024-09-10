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

ModuleNodePair splitModuleNode(const std::string& input)
{
    auto colonLocation = input.find_first_of(':');
    if (colonLocation != std::string::npos) {
        return ModuleNodePair{input.substr(0, colonLocation), input.substr(colonLocation + 1)};
    }
    throw std::logic_error("Internal error: got module-unqualified node name");
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
    std::string operator()(const yang::Empty&)
    {
        return "an empty leaf";
    }
    std::string operator()(const yang::InstanceIdentifier&)
    {
        return "an instance identifier";
    }
    std::string operator()(const yang::Union& type)
    {
        std::ostringstream ss;
        std::transform(type.m_unionTypes.begin(), type.m_unionTypes.end(), std::experimental::make_ostream_joiner(ss, ", "), [this](const auto& unionType) {
            return std::visit(*this, unionType.m_type);
        });
        return ss.str();
    }
    std::string operator()(const yang::Bits& type)
    {
        std::ostringstream ss;
        ss << "bits {";
        std::copy(type.m_allowedValues.begin(), type.m_allowedValues.end(), std::experimental::make_ostream_joiner(ss, ", "));
        ss << "}";
        return ss.str();
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

    std::string operator()(const empty_) const
    {
        return "[empty]";
    }

    std::string operator()(const identityRef_& data) const
    {
        return data.m_prefix ? (data.m_prefix.value().m_name + ":" + data.m_value) : data.m_value;
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
        if (data) {
            return "true";
        } else {
            return "false";
        }
    }

    std::string operator()(const bits_& data) const
    {
        std::stringstream ss;
        std::copy(data.m_bits.begin(), data.m_bits.end(), std::experimental::make_ostream_joiner(ss, " "));
        return ss.str();
    }

    std::string operator()(const instanceIdentifier_& data) const
    {
        return data.m_xpath;
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

struct getSchemaPathVisitor : boost::static_visitor<schemaPath_> {
    schemaPath_ operator()(const dataPath_& path) const
    {
        return dataPathToSchemaPath(path);
    }

    schemaPath_ operator()(const schemaPath_& path) const
    {
        return path;
    }

    [[noreturn]] schemaPath_ operator()([[maybe_unused]] const module_& path) const
    {
        throw std::logic_error("getSchemaPathVisitor: Tried getting a schema path from a module");
    }
};

schemaPath_ anyPathToSchemaPath(const boost::variant<dataPath_, schemaPath_, module_>& path)
{
    return boost::apply_visitor(getSchemaPathVisitor(), path);
}

std::string stripLeafListValueFromPath(const std::string& path)
{
    auto res = path;
    res.erase(res.find_last_of('['));
    return res;
}

std::string stripLastListInstanceFromPath(const std::string& path)
{
    auto res = path;
    res.erase(res.find_first_of('[', res.find_last_of('/')));
    return res;
}

std::string instanceToString(const ListInstance& instance, const std::optional<std::string>& modName)
{
    std::string instanceStr;
    auto modulePrefix = modName ? *modName + ":" : "";
    for (const auto& [key, value] : instance) {
        using namespace std::string_literals;
        instanceStr += "[" + modulePrefix + key + "=" + escapeListKeyString(leafDataToString(value)) + "]";
    }
    return instanceStr;
}
