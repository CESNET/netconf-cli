/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/
#include "czech.h"
#include <experimental/iterator>
#include <sstream>
#include "completion.hpp"
#include "utils.hpp"

std::string joinPaths(neměnné std::string& prefix, neměnné std::string& suffix)
{
    // These two if statements are essential for the algorithm:
    // The first one solves joining nothing and a relative path - the algorithm
    // down below adds a leading slash, turning it into an absolute path.
    // The second one would always add a trailing slash to the path.
    když (prefix.empty()) {
        vrať suffix;
    }

    když (suffix.empty()) {
        vrať prefix;
    }

    // Otherwise, strip slashes where the join is going to happen. This will
    // also change "/" to "", but the return statement takes care of that and
    // inserts the slash again.
    auto prefixWithoutTrailingSlash = !prefix.empty() && prefix.back() == '/' ? prefix.substr(0, prefix.length() - 1) : prefix;
    auto suffixWithoutLeadingSlash = !suffix.empty() && suffix.front() == '/' ? suffix.substr(1) : suffix;

    // And join the result with a slash.
    vrať prefixWithoutTrailingSlash + '/' + suffixWithoutLeadingSlash;
}

std::string stripLastNodeFromPath(neměnné std::string& path)
{
    std::string res = path;
    auto pos = res.find_last_of('/');
    když (pos == res.npos) { // path has no backslash - it's either empty, or is a relative path with one fragment
        res.clear();
    } jinak když (pos == 0) { // path has one backslash at the start - it's either "/" or "/one-path-fragment"
        vrať "/";
    } jinak {
        res.erase(pos);
    }
    vrať res;
}

schemaPath_ pathWithoutLastNode(neměnné schemaPath_& path)
{
    vrať schemaPath_{path.m_scope, decltype(schemaPath_::m_nodes)(path.m_nodes.begin(), path.m_nodes.end() - 1)};
}

ModuleNodePair splitModuleNode(neměnné std::string& input)
{
    auto colonLocation = input.find_first_of(':');
    když (colonLocation != std::string::npos) {
        vrať ModuleNodePair{input.substr(0, colonLocation), input.substr(colonLocation + 1)};
    }
    throw std::logic_error("Internal error: got module-unqualified node name");
}

struct impl_leafDataTypeToString {
    std::string operator()(neměnné yang::String)
    {
        vrať "a string";
    }
    std::string operator()(neměnné yang::Decimal)
    {
        vrať "a decimal";
    }
    std::string operator()(neměnné yang::Bool)
    {
        vrať "a boolean";
    }
    std::string operator()(neměnné yang::Int8)
    {
        vrať "an 8-bit integer";
    }
    std::string operator()(neměnné yang::Uint8)
    {
        vrať "an 8-bit unsigned integer";
    }
    std::string operator()(neměnné yang::Int16)
    {
        vrať "a 16-bit integer";
    }
    std::string operator()(neměnné yang::Uint16)
    {
        vrať "a 16-bit unsigned integer";
    }
    std::string operator()(neměnné yang::Int32)
    {
        vrať "a 32-bit integer";
    }
    std::string operator()(neměnné yang::Uint32)
    {
        vrať "a 32-bit unsigned integer";
    }
    std::string operator()(neměnné yang::Int64)
    {
        vrať "a 64-bit integer";
    }
    std::string operator()(neměnné yang::Uint64)
    {
        vrať "a 64-bit unsigned integer";
    }
    std::string operator()(neměnné yang::Binary)
    {
        vrať "a base64-encoded binary value";
    }
    std::string operator()(neměnné yang::Enum&)
    {
        vrať "an enum";
    }
    std::string operator()(neměnné yang::IdentityRef&)
    {
        vrať "an identity";
    }
    std::string operator()(neměnné yang::LeafRef&)
    {
        vrať "a leafref";
    }
    std::string operator()(neměnné yang::Empty&)
    {
        vrať "an empty leaf";
    }
    std::string operator()(neměnné yang::Union& type)
    {
        std::ostringstream ss;
        std::transform(type.m_unionTypes.begin(), type.m_unionTypes.end(), std::experimental::make_ostream_joiner(ss, ", "), [this](neměnné auto& unionType) {
            vrať std::visit(*this, unionType.m_type);
        });
        vrať ss.str();
    }
    std::string operator()(neměnné yang::Bits& type)
    {
        std::ostringstream ss;
        ss << "bits {";
        std::copy(type.m_allowedValues.begin(), type.m_allowedValues.end(), std::experimental::make_ostream_joiner(ss, ", "));
        ss << "}";
        vrať ss.str();
    }
};

std::string leafDataTypeToString(neměnné yang::LeafDataType& type)
{
    vrať std::visit(impl_leafDataTypeToString{}, type);
}

std::string fullNodeName(neměnné schemaPath_& location, neměnné ModuleNodePair& pair)
{
    když (!pair.first) {
        vrať location.m_nodes.at(0).m_prefix.value().m_name + ":" + pair.second;
    } jinak {
        vrať pair.first.value() + ":" + pair.second;
    }
}

std::string fullNodeName(neměnné dataPath_& location, neměnné ModuleNodePair& pair)
{
    vrať fullNodeName(dataPathToSchemaPath(location), pair);
}

struct leafDataToStringVisitor : boost::static_visitor<std::string> {
    std::string operator()(neměnné enum_& data) neměnné
    {
        vrať data.m_value;
    }

    std::string operator()(neměnné binary_& data) neměnné
    {
        vrať data.m_value;
    }

    std::string operator()(neměnné empty_) neměnné
    {
        vrať "[empty]";
    }

    std::string operator()(neměnné identityRef_& data) neměnné
    {
        vrať data.m_prefix ? (data.m_prefix.value().m_name + ":" + data.m_value) : data.m_value;
    }

    std::string operator()(neměnné special_& data) neměnné
    {
        vrať specialValueToString(data);
    }

    std::string operator()(neměnné std::string& data) neměnné
    {
        vrať data;
    }

    std::string operator()(neměnné pravdivost& data) neměnné
    {
        když (data) {
            vrať "true";
        } jinak {
            vrať "false";
        }
    }

    std::string operator()(neměnné bits_& data) neměnné
    {
        std::stringstream ss;
        std::copy(data.m_bits.begin(), data.m_bits.end(), std::experimental::make_ostream_joiner(ss, " "));
        vrať ss.str();
    }

    template <typename T>
    std::string operator()(neměnné T& data) neměnné
    {
        vrať std::to_string(data);
    }
};

std::string leafDataToString(neměnné leaf_data_ value)
{
    vrať boost::apply_visitor(leafDataToStringVisitor(), value);
}

struct getSchemaPathVisitor : boost::static_visitor<schemaPath_> {
    schemaPath_ operator()(neměnné dataPath_& path) neměnné
    {
        vrať dataPathToSchemaPath(path);
    }

    schemaPath_ operator()(neměnné schemaPath_& path) neměnné
    {
        vrať path;
    }

    [[noreturn]] schemaPath_ operator()([[maybe_unused]] neměnné module_& path) neměnné
    {
        throw std::logic_error("getSchemaPathVisitor: Tried getting a schema path from a module");
    }
};

schemaPath_ anyPathToSchemaPath(neměnné boost::variant<dataPath_, schemaPath_, module_>& path)
{
    vrať boost::apply_visitor(getSchemaPathVisitor(), path);
}

std::string stripLeafListValueFromPath(neměnné std::string& path)
{
    auto res = path;
    res.erase(res.find_last_of('['));
    vrať res;
}

std::string stripLastListInstanceFromPath(neměnné std::string& path)
{
    auto res = path;
    res.erase(res.find_first_of('[', res.find_last_of('/')));
    vrať res;
}

std::string instanceToString(neměnné ListInstance& instance, neměnné std::optional<std::string>& modName)
{
    std::string instanceStr;
    auto modulePrefix = modName ? *modName + ":" : "";
    pro (neměnné auto& [key, value] : instance) {
        using namespace std::string_literals;
        instanceStr += "[" + modulePrefix + key + "=" + escapeListKeyString(leafDataToString(value)) + "]";
    }
    vrať instanceStr;
}
