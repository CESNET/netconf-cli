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
#include "UniqueResource.hpp"
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

namespace {
void printTree(const DatastoreAccess::Tree tree)
{
    for (auto it = tree.begin(); it != tree.end(); it++) {
        auto [path, value] = *it;
        if (value.type() == typeid(special_) && boost::get<special_>(value).m_value == SpecialValue::LeafList) {
            auto leafListPrefix = path;
            std::cout << path << " = " << leafDataToString(value) << std::endl;

            while (it + 1 != tree.end() && boost::starts_with((it + 1)->first, leafListPrefix)) {
                ++it;
                std::cout << stripLeafListValueFromPath(it->first) << " = " << leafDataToString(it->second) << std::endl;
            }
        } else {
            std::cout << path << " = " << leafDataToString(value) << std::endl;
        }
    }
}
}

template <typename PathType>
std::string pathToString(const PathType& path)
{
    return boost::apply_visitor(pathToStringVisitor(), path);
}

void Interpreter::enforceRpcRestrictions(const std::variant<commit_, discard_, copy_, prepare_, switch_>& cmd) const {
    if (m_datastore.inputDatastorePath().has_value()) {
        std::string commandName = std::visit([](const auto& cmd) { return cmd.name; }, cmd);
        throw std::runtime_error("Can't execute `" + commandName + "` during an RPC/action execution.");
    }
}

void Interpreter::enforceRpcRestrictions(const std::variant<move_, set_, cd_, create_, delete_>& cmd) const
{
    if (auto inputPath = m_datastore.inputDatastorePath()) {
        dataPath_ commandPath = std::visit(overloaded {
                                               [](const move_& cmd) { return cmd.m_source; },
                                               [](const auto& cmd) { return cmd.m_path; }
                                           }, cmd);
        dataPath_ newPath = realPath(m_parser.currentPath(), commandPath);
        if (!pathToDataString(newPath, Prefixes::WhenNeeded).starts_with(*inputPath)) {
            std::string commandName = std::visit([](const auto& cmd) { return cmd.name; }, cmd);
            throw std::runtime_error("Can't execute `" + commandName + "` out of the RPC/action context.");
        }
    }
}

void Interpreter::operator()(const commit_& commit) const
{
    enforceRpcRestrictions(commit);
    m_datastore.commitChanges();
}

void Interpreter::operator()(const discard_& discard) const
{
    enforceRpcRestrictions(discard);
    m_datastore.discardChanges();
}

void Interpreter::operator()(const set_& set) const
{
    enforceRpcRestrictions(set);

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
    auto targetSwitcher = make_unique_resource([] {}, [this, oldTarget = m_datastore.target()] {
        m_datastore.setTarget(oldTarget);
    });

    if (get.m_dsTarget) {
        m_datastore.setTarget(*get.m_dsTarget);
    }

    auto items = m_datastore.getItems(pathToString(toCanonicalPath(get.m_path)));
    printTree(items);
}

void Interpreter::operator()(const cd_& cd) const
{
    enforceRpcRestrictions(cd);
    m_parser.changeNode(cd.m_path);
}

void Interpreter::operator()(const create_& create) const
{
    enforceRpcRestrictions(create);
    m_datastore.createItem(pathToString(toCanonicalPath(create.m_path)));
}

void Interpreter::operator()(const delete_& delet) const
{
    enforceRpcRestrictions(delet);
    m_datastore.deleteItem(pathToString(toCanonicalPath(delet.m_path)));
}

void Interpreter::operator()(const ls_& ls) const
{
    std::cout << "Possible nodes:" << std::endl;
    auto recursion{Recursion::NonRecursive};
    for (auto it : ls.m_options) {
        if (it == LsOption::Recursive) {
            recursion = Recursion::Recursive;
        }
    }

    auto toPrint = m_datastore.schema()->availableNodes(toCanonicalPath(ls.m_path), recursion);

    for (const auto& it : toPrint) {
        std::cout << (it.first ? *it.first + ":" : "") + it.second << std::endl;
    }
}

void Interpreter::operator()(const copy_& copy) const
{
    enforceRpcRestrictions(copy);
    m_datastore.copyConfig(copy.m_source, copy.m_destination);
}

std::string Interpreter::buildTypeInfo(const std::string& path) const
{
    std::ostringstream ss;
    std::string typeDescription;
    switch (m_datastore.schema()->nodeType(path)) {
    case yang::NodeTypes::Container:
        ss << "container";
        break;
    case yang::NodeTypes::PresenceContainer:
        ss << "presence container";
        break;
    case yang::NodeTypes::Leaf: {
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

        if (leafType.m_description) {
            typeDescription = "\nType description: " + *leafType.m_description;
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
    case yang::NodeTypes::Rpc:
        ss << "RPC";
        break;
    case yang::NodeTypes::LeafList:
        ss << "leaf-list";
        break;
    case yang::NodeTypes::Action:
        ss << "action";
        break;
    case yang::NodeTypes::AnyXml:
        throw std::logic_error("Sorry, anyxml isn't supported yet.");
    case yang::NodeTypes::Notification:
        throw std::logic_error("Sorry, notifications aren't supported yet.");
    }

    if (!m_datastore.schema()->isConfig(path)) {
        ss << " (ro)\n";
    }

    auto status = m_datastore.schema()->status(path);
    auto statusStr = status == yang::Status::Deprecated ? " (deprecated)" :
        status == yang::Status::Obsolete ? " (obsolete)" :
        "";

    ss << statusStr;

    if (auto description = m_datastore.schema()->description(path)) {
        ss << std::endl << *description << std::endl;
    }

    ss << typeDescription;
    return ss.str();
}

void Interpreter::operator()(const describe_& describe) const
{
    auto fullPath = pathToString(toCanonicalPath(describe.m_path));

    std::cout << pathToString(describe.m_path) << ": " << buildTypeInfo(fullPath) << std::endl;
}

void Interpreter::operator()(const move_& move) const
{
    enforceRpcRestrictions(move);
    m_datastore.moveItem(pathToDataString(move.m_source, Prefixes::WhenNeeded), move.m_destination);
}

void Interpreter::operator()(const dump_& dump) const
{
    std::cout << m_datastore.dump(dump.m_format) << "\n";
}

void Interpreter::operator()(const prepare_& prepare) const
{
    enforceRpcRestrictions(prepare);
    m_datastore.initiate(pathToString(toCanonicalPath(prepare.m_path)));
    m_parser.changeNode(prepare.m_path);
}

void Interpreter::operator()(const exec_& exec) const
{
    m_parser.changeNode({Scope::Absolute, {}});
    if (exec.m_path) {
        m_datastore.initiate(pathToString(toCanonicalPath(*exec.m_path)));
    }
    auto output = m_datastore.execute();
    std::cout << "RPC/action output:\n";
    printTree(output);
}

void Interpreter::operator()(const cancel_&) const
{
    m_parser.changeNode({Scope::Absolute, {}});
    m_datastore.cancel();
}

void Interpreter::operator()(const switch_& switch_cmd) const
{
    enforceRpcRestrictions(switch_cmd);
    m_datastore.setTarget(switch_cmd.m_target);
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
    if (help.m_cmd) {
        std::cout << boost::apply_visitor(commandLongHelpVisitor(), help.m_cmd.get()) << std::endl;
    } else {
        boost::mpl::for_each<CommandTypes, boost::type<boost::mpl::_>>([](auto cmd) {
            std::cout << commandShortHelpVisitor()(cmd) << std::endl;
        });
    }
}

void Interpreter::operator()(const quit_&) const
{
    // no operation
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
    [[nodiscard]] ReturnType impl(const PathType& suffix) const
    {
        PathType res = [this] {
            if constexpr (std::is_same<PathType, schemaPath_>()) {
                return dataPathToSchemaPath(m_parserPath);
            } else {
                return m_parserPath;
            }
        }();

        if (suffix.m_scope == Scope::Absolute) {
            res = {Scope::Absolute, {}};
        }

        for (const auto& fragment : suffix.m_nodes) {
            res.pushFragment(fragment);
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

Interpreter::Interpreter(Parser& parser, ProxyDatastore& datastore)
    : m_parser(parser)
    , m_datastore(datastore)
{
}
