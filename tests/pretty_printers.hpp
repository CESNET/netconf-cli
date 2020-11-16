/*
 * Copyright (C) 2020 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
*/

#include <experimental/iterator>
#include <sstream>
#include "parser.hpp"
#include "utils.hpp"
namespace std {
std::ostream& operator<<(std::ostream& s, const leaf_data_& value)
{
    s << "leaf_data_<" << boost::core::demangle(value.type().name()) << ">{" << leafDataToString(value) << "}" << std::endl;
    return s;
}
std::ostream& operator<<(std::ostream& s, const Completions& completion)
{
    s << std::endl
      << "Completions {" << std::endl
      << "    m_completions: ";
    std::transform(completion.m_completions.begin(), completion.m_completions.end(), std::experimental::make_ostream_joiner(s, ", "), [](auto it) { return '"' + it + '"'; });
    s << std::endl
      << "    m_contextLength: " << completion.m_contextLength << std::endl;
    s << "}" << std::endl;
    return s;
}

std::ostream& operator<<(std::ostream& s, const std::optional<std::string>& opt)
{
    s << (opt ? *opt : "std::nullopt");
    return s;
}

std::ostream& operator<<(std::ostream& s, const ListInstance& map)
{
    s << std::endl
      << "{";
    for (const auto& it : map) {
        s << "{\"" << it.first << "\", " << leafDataToString(it.second) << "}" << std::endl;
    }
    s << "}" << std::endl;
    return s;
}

std::ostream& operator<<(std::ostream& s, const DatastoreAccess::Tree& tree)
{
    s << "DatastoreAccess::Tree {\n";
    for (const auto& [xpath, value] : tree) {
        s << "    {" << xpath << ", " << boost::core::demangle(value.type().name()) << "{" << leafDataToString(value) << "}},\n";
    }
    s << "}\n";
    return s;
}

std::ostream& operator<<(std::ostream& s, const yang::LeafDataType& type)
{
    s << std::endl
      << leafDataTypeToString(type);
    if (std::holds_alternative<yang::Enum>(type)) {
        s << "{";
        auto values = std::get<yang::Enum>(type).m_allowedValues;
        std::transform(values.begin(), values.end(), std::experimental::make_ostream_joiner(s, ", "), [](const auto& value) {
            return value.m_value;
        });
        s << "}";
    }
    if (std::holds_alternative<yang::IdentityRef>(type)) {
        s << "{";
        auto values = std::get<yang::IdentityRef>(type).m_allowedValues;
        std::transform(values.begin(), values.end(), std::experimental::make_ostream_joiner(s, ", "), [](const auto& value) {
            std::string res;
            if (value.m_prefix) {
                res += value.m_prefix->m_name;
                res += ":";
            }
            res += value.m_value;
            return res;
        });
        s << "}";
    }
    if (std::holds_alternative<yang::LeafRef>(type)) {
        s << "{" << std::get<yang::LeafRef>(type).m_targetXPath << "," << std::get<yang::LeafRef>(type).m_targetType->m_type << "}";
    }
    if (std::holds_alternative<yang::Union>(type)) {
        s << "{" << std::endl;
        auto types = std::get<yang::Union>(type).m_unionTypes;
        std::transform(types.begin(), types.end(), std::experimental::make_ostream_joiner(s, ",\n"), [](const auto& type) {
            return type.m_type;
        });
    }
    s << std::endl;
    return s;
}

std::ostream& operator<<(std::ostream& s, const yang::TypeInfo& type)
{
    s << type.m_type << (type.m_units ? " units: " + *type.m_units : "");
    return s;
}

std::ostream& operator<<(std::ostream& s, const boost::optional<std::string>& opt)
{
    s << (opt ? *opt : "std::nullopt");
    return s;
}

std::ostream& operator<<(std::ostream& s, const std::set<ModuleNodePair>& set)
{
    std::transform(set.begin(), set.end(),
            std::experimental::make_ostream_joiner(s, ", "),
            [] (const ModuleNodePair& it) { return (it.first ? *it.first + ":" : "") + it.second; });
    return s;
}

std::ostream& operator<<(std::ostream& s, const std::set<std::string> set)
{
    s << std::endl
      << "{";
    std::copy(set.begin(), set.end(), std::experimental::make_ostream_joiner(s, ", "));
    s << "}" << std::endl;
    return s;
}

std::ostream& operator<<(std::ostream& s, const std::vector<ListInstance> set)
{
    s << std::endl
      << "{" << std::endl;
    std::transform(set.begin(), set.end(), std::experimental::make_ostream_joiner(s, ", \n"), [](const auto& map) {
        std::ostringstream ss;
        ss << "    {" << std::endl
           << "        ";
        std::transform(map.begin(), map.end(), std::experimental::make_ostream_joiner(ss, ", \n        "), [](const auto& keyValue) {
            return "{" + keyValue.first + "{" + boost::core::demangle(keyValue.second.type().name()) + "}" + ", " + leafDataToString(keyValue.second) + "}";
        });
        ss << std::endl
           << "    }";
        return ss.str();
    });
    s << std::endl
      << "}" << std::endl;
    return s;
}
}

std::ostream& operator<<(std::ostream& s, const boost::variant<dataPath_, schemaPath_, module_>& path)
{
    if (path.type() == typeid(module_)) {
        s << "module: " << boost::get<module_>(path).m_name << "\n";
    } else if (path.type() == typeid(dataPath_)) {
        s << "dataPath: " << pathToDataString(boost::get<dataPath_>(path), Prefixes::WhenNeeded) << "\n";
    } else {
        s << "schemaPath: " << pathToSchemaString(boost::get<schemaPath_>(path), Prefixes::WhenNeeded) << "\n";
    }
    return s;
}

std::ostream& operator<<(std::ostream& s, const boost::optional<boost::variant<dataPath_, schemaPath_, module_>>& path)
{
    if (path) {
        s << *path;
    } else {
        s << "boost::none";
    }

    return s;
}

std::ostream& operator<<(std::ostream& s, const create_& create)
{
    s << "\nls_ {\n    " << create.m_path << "}\n";
    return s;
}

std::ostream& operator<<(std::ostream& s, const ls_& ls)
{
    s << "\nls_ {\n    " << ls.m_path << "}\n";
    return s;
}

std::ostream& operator<<(std::ostream& s, const move_& move)
{
    s << "\nmove_ {\n";
    s << "    path: " << move.m_source;
    s << "    mode: ";
    if (std::holds_alternative<yang::move::Absolute>(move.m_destination)) {
        if (std::get<yang::move::Absolute>(move.m_destination) == yang::move::Absolute::Begin) {
            s << "Absolute::Begin";
        } else {
            s << "Absolute::End";
        }
    } else {
        const auto& relative = std::get<yang::move::Relative>(move.m_destination);
        s << "Relative {\n";
        s << "        position: ";
        if (relative.m_position == yang::move::Relative::Position::After) {
            s << "Position::After\n";
        } else {
            s << "Position::Before\n";
        }
        s << "        path: ";
        s << relative.m_path;
    }
    s << "\n}\n";
    return s;
}

std::ostream& operator<<(std::ostream& s, const set_ cmd)
{
    return s << "Command SET {path: " << pathToSchemaString(cmd.m_path, Prefixes::Always) << ", type " << boost::core::demangle(cmd.m_data.type().name()) << ", data: " << leafDataToString(cmd.m_data) << "}";
}

std::ostream& operator<<(std::ostream& s, const prepare_ cmd)
{
    return s << "Command PREPARE {path: " << pathToDataString(cmd.m_path, Prefixes::Always) << "}";
}
