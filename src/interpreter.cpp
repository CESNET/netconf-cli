/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/

#include <boost/algorithm/string/predicate.hpp>
#include <boost/mpl/for_each.hpp>
#include <iostream>
#include <sstream>
#include "datastore_access.hpp"
#include "interpreter.hpp"
#include "utils.hpp"

struct pathToStringVisitor : boost::static_visitor<std::string> {
    std::string operator()(const module_& path) const
    {
        using namespace std::string_literals;
        return "/"s + boost::get<module_>(path).m_name + ":*";
    }
    std::string operator()(const schemaPath_& path) const
    {
        return pathToSchemaString(path, Prefixes::WhenNeeded);
    }
    std::string operator()(const dataPath_& path) const
    {
        return pathToDataString(path, Prefixes::WhenNeeded);
    }
};

template <typename PathType>
std::string pathToString(const PathType& path)
{
    return boost::apply_visitor(pathToStringVisitor(), path);
}

void Interpreter::operator()(const commit_&) const
{
    m_datastore.commitChanges();
}

void Interpreter::operator()(const discard_&) const
{
    m_datastore.discardChanges();
}

void Interpreter::operator()(const set_& set) const
{
    auto data = set.m_data;

    // If the user didn't supply a module prefix for identityref, we need to add it ourselves
    if (data.type() == typeid(identityRef_)) {
        auto identityRef = boost::get<identityRef_>(data);
        if (!identityRef.m_prefix) {
            identityRef.m_prefix = set.m_path.m_nodes.front().m_prefix.value();
            data = identityRef;
        }
    }
    m_datastore.setLeaf(pathToString(toCanonicalPath(set.m_path)), data);
}

void Interpreter::operator()(const get_& get) const
{
    auto items = m_datastore.getItems(pathToString(toCanonicalPath(get.m_path)));
    for (auto it = items.begin(); it != items.end(); it++) {
        auto [path, value] = *it;
        if (value.type() == typeid(special_) && boost::get<special_>(value).m_value == SpecialValue::LeafList) {
            auto leafListPrefix = path;
            std::cout << path << " = " << leafDataToString(value) << std::endl;

            while (it + 1 != items.end() && boost::starts_with((it + 1) ->first, leafListPrefix)) {
                ++it;
                std::cout << stripLeafListValueFromPath(it->first) << " = " << leafDataToString(it->second) << std::endl;
            }
        } else {
            std::cout << path << " = " << leafDataToString(value) << std::endl;
        }
    }
}

void Interpreter::operator()(const cd_& cd) const
{
    m_parser.changeNode(cd.m_path);
}

void Interpreter::operator()(const create_& create) const
{
    if (std::holds_alternative<listElement_>(create.m_path.m_nodes.back().m_suffix))
        m_datastore.createListInstance(pathToString(toCanonicalPath(create.m_path)));
    else if (std::holds_alternative<leafListElement_>(create.m_path.m_nodes.back().m_suffix))
        m_datastore.createLeafListInstance(pathToString(toCanonicalPath(create.m_path)));
    else
        m_datastore.createPresenceContainer(pathToString(toCanonicalPath(create.m_path)));
}

void Interpreter::operator()(const delete_& delet) const
{
    if (std::holds_alternative<container_>(delet.m_path.m_nodes.back().m_suffix))
        m_datastore.deletePresenceContainer(pathToString(toCanonicalPath(delet.m_path)));
    else if (std::holds_alternative<leafListElement_>(delet.m_path.m_nodes.back().m_suffix))
        m_datastore.deleteLeafListInstance(pathToString(toCanonicalPath(delet.m_path)));
    else
        m_datastore.deleteListInstance(pathToString(toCanonicalPath(delet.m_path)));
}

void Interpreter::operator()(const ls_& ls) const
{
    std::cout << "Possible nodes:" << std::endl;
    auto recursion{Recursion::NonRecursive};
    for (auto it : ls.m_options) {
        if (it == LsOption::Recursive)
            recursion = Recursion::Recursive;
    }

    auto toPrint = m_datastore.schema()->availableNodes(toCanonicalPath(ls.m_path), recursion);

    for (const auto& it : toPrint) {
        std::cout << (it.first ? *it.first + ":" : "" ) + it.second << std::endl;
    }
}

void Interpreter::operator()(const copy_& copy) const
{
    m_datastore.copyConfig(copy.m_source, copy.m_destination);
}

std::string Interpreter::buildTypeInfo(const std::string& path) const
{
    std::ostringstream ss;
    switch (m_datastore.schema()->nodeType(path)) {
    case yang::NodeTypes::Container:
        ss << "container";
        break;
    case yang::NodeTypes::PresenceContainer:
        ss << "presence container";
        break;
    case yang::NodeTypes::Leaf:
    {
        auto leafType = m_datastore.schema()->leafType(path);
        auto typedefName = m_datastore.schema()->leafTypeName(path);
        std::string baseTypeStr;
        if (std::holds_alternative<yang::LeafRef>(leafType.m_type)) {
            ss << "-> ";
            ss << m_datastore.schema()->leafrefPath(path) << " ";
            baseTypeStr = leafDataTypeToString(std::get<yang::LeafRef>(leafType.m_type).m_targetType->m_type);
        } else {
            baseTypeStr = leafDataTypeToString(leafType.m_type);
        }

        if (typedefName) {
            ss << *typedefName << " (" << baseTypeStr << ")";
        } else {
            ss << baseTypeStr;
        }

        if (leafType.m_units) {
            ss << " [" + *leafType.m_units + "]";
        }

        if (m_datastore.schema()->leafIsKey(path)) {
            ss << " (key)";
        }

        if (auto defaultValue = m_datastore.schema()->defaultValue(path)) {
            ss << " default: " << leafDataToString(*defaultValue);
        }
        break;
    }
    case yang::NodeTypes::List:
        ss << "list";
        break;
    case yang::NodeTypes::Action:
    case yang::NodeTypes::AnyXml:
    case yang::NodeTypes::LeafList:
    case yang::NodeTypes::Notification:
    case yang::NodeTypes::Rpc:
        throw std::logic_error("describe got an rpc or an action: this should never happen, because their paths cannot be parsed");
    }

    if (!m_datastore.schema()->isConfig(path)) {
        ss << " (ro)";
    }

    return ss.str();
}

void Interpreter::operator()(const describe_& describe) const
{
    auto path = pathToString(toCanonicalPath(describe.m_path));
    auto status = m_datastore.schema()->status(path);
    auto statusStr = status == yang::Status::Deprecated ? " (deprecated)" :
        status == yang::Status::Obsolete ? " (obsolete)" :
        "";

    std::cout << path << ": " << buildTypeInfo(path) << statusStr << std::endl;
    if (auto description = m_datastore.schema()->description(path)) {
        std::cout << std::endl << *description << std::endl;
    }
}

void Interpreter::operator()(const move_& move) const
{
    m_datastore.moveItem(pathToDataString(move.m_source, Prefixes::WhenNeeded), move.m_destination);
}

struct commandLongHelpVisitor : boost::static_visitor<const char*> {
    template <typename T>
    auto constexpr operator()(boost::type<T>) const
    {
        return T::longHelp;
    }
};

struct commandShortHelpVisitor : boost::static_visitor<const char*> {
    template <typename T>
    auto constexpr operator()(boost::type<T>) const
    {
        return T::shortHelp;
    }
};

void Interpreter::operator()(const help_& help) const
{
    if (help.m_cmd)
        std::cout << boost::apply_visitor(commandLongHelpVisitor(), help.m_cmd.get()) << std::endl;
    else
        boost::mpl::for_each<CommandTypes, boost::type<boost::mpl::_>>([](auto cmd) {
            std::cout << commandShortHelpVisitor()(cmd) << std::endl;
        });
}

template <typename PathType>
boost::variant<dataPath_, schemaPath_, module_> Interpreter::toCanonicalPath(const boost::optional<PathType>& optPath) const
{
    if (!optPath) {
        return m_parser.currentPath();
    }
    return toCanonicalPath(*optPath);
}

struct impl_toCanonicalPath {
    const dataPath_& m_parserPath;

    using ReturnType = boost::variant<dataPath_, schemaPath_, module_>;

    impl_toCanonicalPath(const dataPath_& parserPath)
        : m_parserPath(parserPath)
    {
    }
    ReturnType operator()(const module_& path) const
    {
        using namespace std::string_literals;
        return path;
    }
    ReturnType operator()(const schemaPath_& path) const
    {
        return impl(path);
    }
    ReturnType operator()(const dataPath_& path) const
    {
        return impl(path);
    }

private:
    template <typename PathType>
    ReturnType impl(const PathType& suffix) const
    {
        PathType res = [this] {
            if constexpr (std::is_same<PathType, schemaPath_>()) {
                return dataPathToSchemaPath(m_parserPath);
            } else {
                return m_parserPath;
            }
        }();

        if (suffix.m_scope == Scope::Absolute) {
            return suffix;
        }

        for (const auto& fragment : suffix.m_nodes) {
            if (std::holds_alternative<nodeup_>(fragment.m_suffix)) {
                res.m_nodes.pop_back();
            } else {
                res.m_nodes.push_back(fragment);
            }
        }

        return res;
    }
};

template <typename PathType>
boost::variant<dataPath_, schemaPath_, module_> Interpreter::toCanonicalPath(const PathType& path) const
{
    if constexpr (std::is_same<PathType, dataPath_>()) {
        return impl_toCanonicalPath(m_parser.currentPath())(path);
    } else {
        return boost::apply_visitor(impl_toCanonicalPath(m_parser.currentPath()), path);
    }
}

Interpreter::Interpreter(Parser& parser, DatastoreAccess& datastore)
    : m_parser(parser)
    , m_datastore(datastore)
{
}
