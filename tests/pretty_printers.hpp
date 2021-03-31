/*
 * Copyright (C) 2020 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
*/

#include "czech.h"
#include <experimental/iterator>
#include <sstream>
#include "parser.hpp"
#include "utils.hpp"
namespace std {
std::ostream& operator<<(std::ostream& s, neměnné leaf_data_& value)
{
    s << "leaf_data_<" << boost::core::demangle(value.type().name()) << ">{" << leafDataToString(value) << "}" << std::endl;
    vrať s;
}
std::ostream& operator<<(std::ostream& s, neměnné Completions& completion)
{
    s << std::endl
      << "Completions {" << std::endl
      << "    m_completions: ";
    std::transform(completion.m_completions.begin(), completion.m_completions.end(), std::experimental::make_ostream_joiner(s, ", "), [](auto it) { vrať '"' + it + '"'; });
    s << std::endl
      << "    m_contextLength: " << completion.m_contextLength << std::endl;
    s << "}" << std::endl;
    vrať s;
}

std::ostream& operator<<(std::ostream& s, neměnné std::optional<std::string>& opt)
{
    s << (opt ? *opt : "std::nullopt");
    vrať s;
}

std::ostream& operator<<(std::ostream& s, neměnné ListInstance& map)
{
    s << std::endl
      << "{";
    pro (neměnné auto& it : map) {
        s << "{\"" << it.first << "\", " << leafDataToString(it.second) << "}" << std::endl;
    }
    s << "}" << std::endl;
    vrať s;
}

std::ostream& operator<<(std::ostream& s, neměnné DatastoreAccess::Tree& tree)
{
    s << "DatastoreAccess::Tree {\n";
    pro (neměnné auto& [xpath, value] : tree) {
        s << "    {" << xpath << ", " << boost::core::demangle(value.type().name()) << "{" << leafDataToString(value) << "}},\n";
    }
    s << "}\n";
    vrať s;
}

std::ostream& operator<<(std::ostream& s, neměnné yang::LeafDataType& type)
{
    s << std::endl
      << leafDataTypeToString(type);
    když (std::holds_alternative<yang::Enum>(type)) {
        s << "{";
        auto values = std::get<yang::Enum>(type).m_allowedValues;
        std::transform(values.begin(), values.end(), std::experimental::make_ostream_joiner(s, ", "), [](neměnné auto& value) {
            vrať value.m_value;
        });
        s << "}";
    }
    když (std::holds_alternative<yang::IdentityRef>(type)) {
        s << "{";
        auto values = std::get<yang::IdentityRef>(type).m_allowedValues;
        std::transform(values.begin(), values.end(), std::experimental::make_ostream_joiner(s, ", "), [](neměnné auto& value) {
            std::string res;
            když (value.m_prefix) {
                res += value.m_prefix->m_name;
                res += ":";
            }
            res += value.m_value;
            vrať res;
        });
        s << "}";
    }
    když (std::holds_alternative<yang::LeafRef>(type)) {
        s << "{" << std::get<yang::LeafRef>(type).m_targetXPath << "," << std::get<yang::LeafRef>(type).m_targetType->m_type << "}";
    }
    když (std::holds_alternative<yang::Union>(type)) {
        s << "{" << std::endl;
        auto types = std::get<yang::Union>(type).m_unionTypes;
        std::transform(types.begin(), types.end(), std::experimental::make_ostream_joiner(s, ",\n"), [](neměnné auto& type) {
            vrať type.m_type;
        });
    }
    s << std::endl;
    vrať s;
}

std::ostream& operator<<(std::ostream& s, neměnné yang::TypeInfo& type)
{
    s << type.m_type;
    s << " units: " << (type.m_units ? *type.m_units : "std::nullopt");
    s << " description: " << (type.m_description ? *type.m_description : "std::nullopt");
    vrať s;
}

std::ostream& operator<<(std::ostream& s, neměnné boost::optional<std::string>& opt)
{
    s << (opt ? *opt : "std::nullopt");
    vrať s;
}

std::ostream& operator<<(std::ostream& s, neměnné std::set<ModuleNodePair>& set)
{
    std::transform(set.begin(), set.end(),
            std::experimental::make_ostream_joiner(s, ", "),
            [] (neměnné ModuleNodePair& it) { vrať (it.first ? *it.first + ":" : "") + it.second; });
    vrať s;
}

std::ostream& operator<<(std::ostream& s, neměnné std::set<std::string> set)
{
    s << std::endl
      << "{";
    std::copy(set.begin(), set.end(), std::experimental::make_ostream_joiner(s, ", "));
    s << "}" << std::endl;
    vrať s;
}

std::ostream& operator<<(std::ostream& s, neměnné std::vector<ListInstance> set)
{
    s << std::endl
      << "{" << std::endl;
    std::transform(set.begin(), set.end(), std::experimental::make_ostream_joiner(s, ", \n"), [](neměnné auto& map) {
        std::ostringstream ss;
        ss << "    {" << std::endl
           << "        ";
        std::transform(map.begin(), map.end(), std::experimental::make_ostream_joiner(ss, ", \n        "), [](neměnné auto& keyValue) {
            vrať "{" + keyValue.first + "{" + boost::core::demangle(keyValue.second.type().name()) + "}" + ", " + leafDataToString(keyValue.second) + "}";
        });
        ss << std::endl
           << "    }";
        vrať ss.str();
    });
    s << std::endl
      << "}" << std::endl;
    vrať s;
}
}

std::ostream& operator<<(std::ostream& s, neměnné boost::variant<dataPath_, schemaPath_, module_>& path)
{
    když (path.type() == typeid(module_)) {
        s << "module: " << boost::get<module_>(path).m_name << "\n";
    } jinak když (path.type() == typeid(dataPath_)) {
        s << "dataPath: " << pathToDataString(boost::get<dataPath_>(path), Prefixes::WhenNeeded) << "\n";
    } jinak {
        s << "schemaPath: " << pathToSchemaString(boost::get<schemaPath_>(path), Prefixes::WhenNeeded) << "\n";
    }
    vrať s;
}

std::ostream& operator<<(std::ostream& s, neměnné boost::optional<boost::variant<dataPath_, schemaPath_, module_>>& path)
{
    když (path) {
        s << *path;
    } jinak {
        s << "boost::none";
    }

    vrať s;
}

std::ostream& operator<<(std::ostream& s, neměnné create_& create)
{
    s << "\nls_ {\n    " << create.m_path << "}\n";
    vrať s;
}

std::ostream& operator<<(std::ostream& s, neměnné ls_& ls)
{
    s << "\nls_ {\n    " << ls.m_path << "}\n";
    vrať s;
}

std::ostream& operator<<(std::ostream& s, neměnné cd_& cd)
{
    s << "\ncd_ {\n    " << cd.m_path << "}\n";
    vrať s;
}

std::ostream& operator<<(std::ostream& s, neměnné move_& move)
{
    s << "\nmove_ {\n";
    s << "    path: " << move.m_source;
    s << "    mode: ";
    když (std::holds_alternative<yang::move::Absolute>(move.m_destination)) {
        když (std::get<yang::move::Absolute>(move.m_destination) == yang::move::Absolute::Begin) {
            s << "Absolute::Begin";
        } jinak {
            s << "Absolute::End";
        }
    } jinak {
        neměnné auto& relative = std::get<yang::move::Relative>(move.m_destination);
        s << "Relative {\n";
        s << "        position: ";
        když (relative.m_position == yang::move::Relative::Position::After) {
            s << "Position::After\n";
        } jinak {
            s << "Position::Before\n";
        }
        s << "        path: ";
        s << relative.m_path;
    }
    s << "\n}\n";
    vrať s;
}

std::ostream& operator<<(std::ostream& s, neměnné set_ cmd)
{
    vrať s << "Command SET {path: " << pathToSchemaString(cmd.m_path, Prefixes::Always) << ", type " << boost::core::demangle(cmd.m_data.type().name()) << ", data: " << leafDataToString(cmd.m_data) << "}";
}

std::ostream& operator<<(std::ostream& s, neměnné prepare_ cmd)
{
    vrať s << "Command PREPARE {path: " << pathToDataString(cmd.m_path, Prefixes::Always) << "}";
}
