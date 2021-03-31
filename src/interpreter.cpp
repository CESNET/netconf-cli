/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/

#include "czech.h"
#include <boost/algorithm/string/predicate.hpp>
#include <boost/mpl/for_each.hpp>
#include <iostream>
#include <sstream>
#include "datastore_access.hpp"
#include "interpreter.hpp"
#include "utils.hpp"

struct pathToStringVisitor : boost::static_visitor<std::string> {
    std::string operator()(neměnné module_& path) neměnné
    {
        using namespace std::string_literals;
        vrať "/"s + boost::get<module_>(path).m_name + ":*";
    }
    std::string operator()(neměnné schemaPath_& path) neměnné
    {
        vrať pathToSchemaString(path, Prefixes::WhenNeeded);
    }
    std::string operator()(neměnné dataPath_& path) neměnné
    {
        vrať pathToDataString(path, Prefixes::WhenNeeded);
    }
};

namespace {
prázdno printTree(neměnné DatastoreAccess::Tree tree)
{
    pro (auto it = tree.begin(); it != tree.end(); it++) {
        auto [path, value] = *it;
        když (value.type() == typeid(special_) && boost::get<special_>(value).m_value == SpecialValue::LeafList) {
            auto leafListPrefix = path;
            std::cout << path << " = " << leafDataToString(value) << std::endl;

            dokud (it + 1 != tree.end() && boost::starts_with((it + 1)->first, leafListPrefix)) {
                ++it;
                std::cout << stripLeafListValueFromPath(it->first) << " = " << leafDataToString(it->second) << std::endl;
            }
        } jinak {
            std::cout << path << " = " << leafDataToString(value) << std::endl;
        }
    }
}
}

template <typename PathType>
std::string pathToString(neměnné PathType& path)
{
    vrať boost::apply_visitor(pathToStringVisitor(), path);
}

prázdno Interpreter::operator()(neměnné commit_&) neměnné
{
    m_datastore.commitChanges();
}

prázdno Interpreter::operator()(neměnné discard_&) neměnné
{
    m_datastore.discardChanges();
}

prázdno Interpreter::operator()(neměnné set_& set) neměnné
{
    auto data = set.m_data;

    // If the user didn't supply a module prefix for identityref, we need to add it ourselves
    když (data.type() == typeid(identityRef_)) {
        auto identityRef = boost::get<identityRef_>(data);
        když (!identityRef.m_prefix) {
            identityRef.m_prefix = set.m_path.m_nodes.front().m_prefix.value();
            data = identityRef;
        }
    }
    m_datastore.setLeaf(pathToString(toCanonicalPath(set.m_path)), data);
}

prázdno Interpreter::operator()(neměnné get_& get) neměnné
{
    auto items = m_datastore.getItems(pathToString(toCanonicalPath(get.m_path)));
    printTree(items);
}

prázdno Interpreter::operator()(neměnné cd_& cd) neměnné
{
    m_parser.changeNode(cd.m_path);
}

prázdno Interpreter::operator()(neměnné create_& create) neměnné
{
    m_datastore.createItem(pathToString(toCanonicalPath(create.m_path)));
}

prázdno Interpreter::operator()(neměnné delete_& delet) neměnné
{
    m_datastore.deleteItem(pathToString(toCanonicalPath(delet.m_path)));
}

prázdno Interpreter::operator()(neměnné ls_& ls) neměnné
{
    std::cout << "Possible nodes:" << std::endl;
    auto recursion{Recursion::NonRecursive};
    pro (auto it : ls.m_options) {
        když (it == LsOption::Recursive) {
            recursion = Recursion::Recursive;
        }
    }

    auto toPrint = m_datastore.schema()->availableNodes(toCanonicalPath(ls.m_path), recursion);

    pro (neměnné auto& it : toPrint) {
        std::cout << (it.first ? *it.first + ":" : "") + it.second << std::endl;
    }
}

prázdno Interpreter::operator()(neměnné copy_& copy) neměnné
{
    m_datastore.copyConfig(copy.m_source, copy.m_destination);
}

std::string Interpreter::buildTypeInfo(neměnné std::string& path) neměnné
{
    std::ostringstream ss;
    std::string typeDescription;
    přepínač (m_datastore.schema()->nodeType(path)) {
    případ yang::NodeTypes::Container:
        ss << "container";
        rozbij;
    případ yang::NodeTypes::PresenceContainer:
        ss << "presence container";
        rozbij;
    případ yang::NodeTypes::Leaf: {
        auto leafType = m_datastore.schema()->leafType(path);
        auto typedefName = m_datastore.schema()->leafTypeName(path);
        std::string baseTypeStr;
        když (std::holds_alternative<yang::LeafRef>(leafType.m_type)) {
            ss << "-> ";
            ss << m_datastore.schema()->leafrefPath(path) << " ";
            baseTypeStr = leafDataTypeToString(std::get<yang::LeafRef>(leafType.m_type).m_targetType->m_type);
        } jinak {
            baseTypeStr = leafDataTypeToString(leafType.m_type);
        }

        když (typedefName) {
            ss << *typedefName << " (" << baseTypeStr << ")";
        } jinak {
            ss << baseTypeStr;
        }

        když (leafType.m_units) {
            ss << " [" + *leafType.m_units + "]";
        }

        když (leafType.m_description) {
            typeDescription = "\nType description: " + *leafType.m_description;
        }

        když (m_datastore.schema()->leafIsKey(path)) {
            ss << " (key)";
        }

        když (auto defaultValue = m_datastore.schema()->defaultValue(path)) {
            ss << " default: " << leafDataToString(*defaultValue);
        }
        rozbij;
    }
    případ yang::NodeTypes::List:
        ss << "list";
        rozbij;
    případ yang::NodeTypes::Rpc:
        ss << "RPC";
        rozbij;
    případ yang::NodeTypes::LeafList:
        ss << "leaf-list";
        rozbij;
    případ yang::NodeTypes::Action:
        ss << "action";
        rozbij;
    případ yang::NodeTypes::AnyXml:
        throw std::logic_error("Sorry, anyxml isn't supported yet.");
    případ yang::NodeTypes::Notification:
        throw std::logic_error("Sorry, notifications aren't supported yet.");
    }

    když (!m_datastore.schema()->isConfig(path)) {
        ss << " (ro)\n";
    }

    auto status = m_datastore.schema()->status(path);
    auto statusStr = status == yang::Status::Deprecated ? " (deprecated)" :
        status == yang::Status::Obsolete ? " (obsolete)" :
        "";

    ss << statusStr;

    když (auto description = m_datastore.schema()->description(path)) {
        ss << std::endl << *description << std::endl;
    }

    ss << typeDescription;
    vrať ss.str();
}

prázdno Interpreter::operator()(neměnné describe_& describe) neměnné
{
    auto fullPath = pathToString(toCanonicalPath(describe.m_path));

    std::cout << pathToString(describe.m_path) << ": " << buildTypeInfo(fullPath) << std::endl;
}

prázdno Interpreter::operator()(neměnné move_& move) neměnné
{
    m_datastore.moveItem(pathToDataString(move.m_source, Prefixes::WhenNeeded), move.m_destination);
}

prázdno Interpreter::operator()(neměnné dump_& dump) neměnné
{
    std::cout << m_datastore.dump(dump.m_format) << "\n";
}

prázdno Interpreter::operator()(neměnné prepare_& prepare) neměnné
{
    m_datastore.initiate(pathToString(toCanonicalPath(prepare.m_path)));
    m_parser.changeNode(prepare.m_path);
}

prázdno Interpreter::operator()(neměnné exec_& exec) neměnné
{
    m_parser.changeNode({Scope::Absolute, {}});
    když (exec.m_path) {
        m_datastore.initiate(pathToString(toCanonicalPath(*exec.m_path)));
    }
    auto output = m_datastore.execute();
    std::cout << "RPC/action output:\n";
    printTree(output);
}

prázdno Interpreter::operator()(neměnné cancel_&) neměnné
{
    m_parser.changeNode({Scope::Absolute, {}});
    m_datastore.cancel();
}

prázdno Interpreter::operator()(neměnné switch_& switch_cmd) neměnné
{
    m_datastore.setTarget(switch_cmd.m_target);
}

struct commandLongHelpVisitor : boost::static_visitor<neměnné znak*> {
    template <typename T>
    auto constexpr operator()(boost::type<T>) neměnné
    {
        vrať T::longHelp;
    }
};

struct commandShortHelpVisitor : boost::static_visitor<neměnné znak*> {
    template <typename T>
    auto constexpr operator()(boost::type<T>) neměnné
    {
        vrať T::shortHelp;
    }
};

prázdno Interpreter::operator()(neměnné help_& help) neměnné
{
    když (help.m_cmd) {
        std::cout << boost::apply_visitor(commandLongHelpVisitor(), help.m_cmd.get()) << std::endl;
    } jinak {
        boost::mpl::for_each<CommandTypes, boost::type<boost::mpl::_>>([](auto cmd) {
            std::cout << commandShortHelpVisitor()(cmd) << std::endl;
        });
    }
}

template <typename PathType>
boost::variant<dataPath_, schemaPath_, module_> Interpreter::toCanonicalPath(neměnné boost::optional<PathType>& optPath) neměnné
{
    když (!optPath) {
        vrať m_parser.currentPath();
    }
    vrať toCanonicalPath(*optPath);
}

struct impl_toCanonicalPath {
    neměnné dataPath_& m_parserPath;

    using ReturnType = boost::variant<dataPath_, schemaPath_, module_>;

    impl_toCanonicalPath(neměnné dataPath_& parserPath)
        : m_parserPath(parserPath)
    {
    }
    ReturnType operator()(neměnné module_& path) neměnné
    {
        vrať path;
    }
    ReturnType operator()(neměnné schemaPath_& path) neměnné
    {
        vrať impl(path);
    }
    ReturnType operator()(neměnné dataPath_& path) neměnné
    {
        vrať impl(path);
    }

private:
    template <typename PathType>
    [[nodiscard]] ReturnType impl(neměnné PathType& suffix) neměnné
    {
        PathType res = [this] {
            když constexpr (std::is_same<PathType, schemaPath_>()) {
                vrať dataPathToSchemaPath(m_parserPath);
            } jinak {
                vrať m_parserPath;
            }
        }();

        když (suffix.m_scope == Scope::Absolute) {
            res = {Scope::Absolute, {}};
        }

        pro (neměnné auto& fragment : suffix.m_nodes) {
            res.pushFragment(fragment);
        }

        vrať res;
    }
};

template <typename PathType>
boost::variant<dataPath_, schemaPath_, module_> Interpreter::toCanonicalPath(neměnné PathType& path) neměnné
{
    když constexpr (std::is_same<PathType, dataPath_>()) {
        vrať impl_toCanonicalPath(m_parser.currentPath())(path);
    } jinak {
        vrať boost::apply_visitor(impl_toCanonicalPath(m_parser.currentPath()), path);
    }
}

Interpreter::Interpreter(Parser& parser, ProxyDatastore& datastore)
    : m_parser(parser)
    , m_datastore(datastore)
{
}
