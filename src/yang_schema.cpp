/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/

#include "czech.h"
#include <libyang/Libyang.hpp>
#include <libyang/Tree_Data.hpp>
#include <libyang/Tree_Schema.hpp>
#include <string_view>
#include "UniqueResource.hpp"
#include "utils.hpp"
#include "yang_schema.hpp"

class YangLoadError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
    ~YangLoadError() override = výchozí;
};

class UnsupportedYangTypeException : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
    ~UnsupportedYangTypeException() override = výchozí;
};

class InvalidSchemaQueryException : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
    ~InvalidSchemaQueryException() override = výchozí;
};

YangSchema::YangSchema()
    : m_context(std::make_shared<libyang::Context>(nullptr, LY_CTX_DISABLE_SEARCHDIR_CWD))
{
}

YangSchema::YangSchema(std::shared_ptr<libyang::Context> lyCtx)
    : m_context(lyCtx)
{
}

YangSchema::~YangSchema() = výchozí;

prázdno YangSchema::addSchemaString(neměnné znak* schema)
{
    když (!m_context->parse_module_mem(schema, LYS_IN_YANG)) {
        throw YangLoadError("Couldn't load schema");
    }
}

prázdno YangSchema::addSchemaDirectory(neměnné znak* directoryName)
{
    když (m_context->set_searchdir(directoryName)) {
        throw YangLoadError("Couldn't add schema search directory");
    }
}

prázdno YangSchema::addSchemaFile(neměnné znak* filename)
{
    když (!m_context->parse_module_path(filename, LYS_IN_YANG)) {
        throw YangLoadError("Couldn't load schema");
    }
}

pravdivost YangSchema::isModule(neměnné std::string& name) neměnné
{
    neměnné auto set = modules();
    vrať set.find(name) != set.end();
}

pravdivost YangSchema::listHasKey(neměnné schemaPath_& listPath, neměnné std::string& key) neměnné
{
    neměnné auto keys = listKeys(listPath);
    vrať keys.find(key) != keys.end();
}

pravdivost YangSchema::leafIsKey(neměnné std::string& leafPath) neměnné
{
    auto node = getSchemaNode(leafPath);
    když (!node || node->nodetype() != LYS_LEAF) {
        vrať false;
    }

    vrať libyang::Schema_Node_Leaf{node}.is_key().get();
}

libyang::S_Schema_Node YangSchema::impl_getSchemaNode(neměnné std::string& node) neměnné
{
    // If no node is found find_path prints an error message, so we have to
    // disable logging
    // https://github.com/CESNET/libyang/issues/753
    {
        číslo oldOptions;
        auto logBlocker = make_unique_resource(
            [&oldOptions]() {
                oldOptions = libyang::set_log_options(0);
            },
            [&oldOptions]() {
                libyang::set_log_options(oldOptions);
            });
        auto res = m_context->get_node(nullptr, node.c_str());
        když (!res) { // If no node is found, try output rpc nodes too.
            res = m_context->get_node(nullptr, node.c_str(), 1);
        }
        vrať res;
    }
}


libyang::S_Schema_Node YangSchema::getSchemaNode(neměnné std::string& node) neměnné
{
    vrať impl_getSchemaNode(node);
}

libyang::S_Schema_Node YangSchema::getSchemaNode(neměnné schemaPath_& location, neměnné ModuleNodePair& node) neměnné
{
    std::string absPath = joinPaths(pathToSchemaString(location, Prefixes::Always), fullNodeName(location, node));

    vrať impl_getSchemaNode(absPath);
}

libyang::S_Schema_Node YangSchema::getSchemaNode(neměnné schemaPath_& listPath) neměnné
{
    std::string absPath = pathToSchemaString(listPath, Prefixes::Always);
    vrať impl_getSchemaNode(absPath);
}

neměnné std::set<std::string> YangSchema::listKeys(neměnné schemaPath_& listPath) neměnné
{
    auto node = getSchemaNode(listPath);
    když (node->nodetype() != LYS_LIST) {
        vrať {};
    }

    auto list = std::make_shared<libyang::Schema_Node_List>(node);
    std::set<std::string> keys;
    neměnné auto& keysVec = list->keys();

    std::transform(keysVec.begin(), keysVec.end(), std::inserter(keys, keys.begin()), [](neměnné auto& it) { vrať it->name(); });
    vrať keys;
}

namespace {
enum class ResolveMode {
    Enum,
    Identity
};
/** @brief Resolves a typedef to a type which defines values.
 * When we need allowed values of a type and that type is a typedef, we need to recurse into the typedef until we find a
 * type which defines values. These values are the allowed values.
 * Example:
 *
 * typedef MyOtherEnum {
 *   type enumeration {
 *     enum "A";
 *     enum "B";
 *   }
 * }
 *
 * typedef MyEnum {
 *   type MyOtherEnum;
 * }
 *
 * If `toResolve` points to MyEnum, then just doing ->enums()->enm() returns nothing and that means that this particular
 * typedef (MyEnum) did not say which values are allowed. So, we need to dive into the parent enum (MyOtherEnum) with
 * ->der()->type(). This typedef (MyOtherEnum) DID specify allowed values and enums()->enm() WILL contain them. These
 *  values are the only relevant values and we don't care about other parent typedefs. We return these values to the
 *  caller.
 *
 *  For enums, this function simply returns all allowed enums.
 *  For identities, this function returns which bases `toResolve` has.
 */
template <ResolveMode TYPE>
auto resolveTypedef(neměnné libyang::S_Type& toResolve)
{
    auto type = toResolve;
    auto getValuesFromType = [] (neměnné libyang::S_Type& type) {
        když constexpr (TYPE == ResolveMode::Identity) {
            vrať type->info()->ident()->ref();
        } jinak {
            vrať type->info()->enums()->enm();
        }
    };
    auto values = getValuesFromType(type);
    dokud (values.empty()) {
        type = type->der()->type();
        values = getValuesFromType(type);
    }

    vrať values;
}

std::set<enum_> enumValues(neměnné libyang::S_Type& type)
{
    auto values = resolveTypedef<ResolveMode::Enum>(type);

    std::vector<libyang::S_Type_Enum> enabled;
    std::copy_if(values.begin(), values.end(), std::back_inserter(enabled), [](neměnné libyang::S_Type_Enum& it) {
        auto iffeatures = it->iffeature();
        vrať std::all_of(iffeatures.begin(), iffeatures.end(), [](auto it) { vrať it->value(); });
    });

    std::set<enum_> enumSet;
    std::transform(enabled.begin(), enabled.end(), std::inserter(enumSet, enumSet.end()), [](auto it) { vrať enum_{it->name()}; });
    vrať enumSet;
}

std::set<identityRef_> validIdentities(neměnné libyang::S_Type& type)
{
    std::set<identityRef_> identSet;

    pro (auto base : resolveTypedef<ResolveMode::Identity>(type)) { // Iterate over all bases
        // Iterate over derived identities (this is recursive!)
        pro (auto derived : base->der()->schema()) {
            identSet.emplace(derived->module()->name(), derived->name());
        }
    }

    vrať identSet;
}

std::string leafrefPath(neměnné libyang::S_Type& type)
{
    vrať type->info()->lref()->target()->path(LYS_PATH_FIRST_PREFIX);
}
}

template <typename NodeType>
yang::TypeInfo YangSchema::impl_leafType(neměnné libyang::S_Schema_Node& node) neměnné
{
    using namespace std::string_literals;
    auto leaf = std::make_shared<NodeType>(node);
    auto leafUnits = leaf->units();
    std::function<yang::TypeInfo(std::shared_ptr<libyang::Type>)> resolveType;
    resolveType = [this, &resolveType, leaf, leafUnits](std::shared_ptr<libyang::Type> type) -> yang::TypeInfo {
        yang::LeafDataType resType;
        přepínač (type->base()) {
        případ LY_TYPE_STRING:
            resType.emplace<yang::String>();
            rozbij;
        případ LY_TYPE_DEC64:
            resType.emplace<yang::Decimal>();
            rozbij;
        případ LY_TYPE_BOOL:
            resType.emplace<yang::Bool>();
            rozbij;
        případ LY_TYPE_INT8:
            resType.emplace<yang::Int8>();
            rozbij;
        případ LY_TYPE_INT16:
            resType.emplace<yang::Int16>();
            rozbij;
        případ LY_TYPE_INT32:
            resType.emplace<yang::Int32>();
            rozbij;
        případ LY_TYPE_INT64:
            resType.emplace<yang::Int64>();
            rozbij;
        případ LY_TYPE_UINT8:
            resType.emplace<yang::Uint8>();
            rozbij;
        případ LY_TYPE_UINT16:
            resType.emplace<yang::Uint16>();
            rozbij;
        případ LY_TYPE_UINT32:
            resType.emplace<yang::Uint32>();
            rozbij;
        případ LY_TYPE_UINT64:
            resType.emplace<yang::Uint64>();
            rozbij;
        případ LY_TYPE_BINARY:
            resType.emplace<yang::Binary>();
            rozbij;
        případ LY_TYPE_EMPTY:
            resType.emplace<yang::Empty>();
            rozbij;
        případ LY_TYPE_ENUM:
            resType.emplace<yang::Enum>(enumValues(type));
            rozbij;
        případ LY_TYPE_IDENT:
            resType.emplace<yang::IdentityRef>(validIdentities(type));
            rozbij;
        případ LY_TYPE_LEAFREF:
            resType.emplace<yang::LeafRef>(::leafrefPath(type), std::make_unique<yang::TypeInfo>(leafType(::leafrefPath(type))));
            rozbij;
        případ LY_TYPE_BITS: {
            auto resBits = yang::Bits{};
            pro (neměnné auto& bit : type->info()->bits()->bit()) {
                resBits.m_allowedValues.emplace(bit->name());
            }
            resType.emplace<yang::Bits>(std::move(resBits));
            rozbij;
        }
        případ LY_TYPE_UNION: {
            auto resUnion = yang::Union{};
            pro (auto unionType : type->info()->uni()->types()) {
                resUnion.m_unionTypes.emplace_back(resolveType(unionType));
            }
            resType.emplace<yang::Union>(std::move(resUnion));
            rozbij;
        }
        výchozí:
            using namespace std::string_literals;
            throw UnsupportedYangTypeException("the type of "s + leaf->name() + " is not supported: " + std::to_string(leaf->type()->base()));
        }

        std::optional<std::string> resUnits;

        když (leafUnits) {
            resUnits = leafUnits;
        } jinak {
            pro (auto parentTypedef = type->der(); parentTypedef; parentTypedef = parentTypedef->type()->der()) {
                auto units = parentTypedef->units();
                když (units) {
                    resUnits = units;
                    rozbij;
                }
            }
        }

        std::optional<std::string> resDescription;

        // checking for parentTypedef->type()->der() means I'm going to enter inside base types like "string". These
        // also have a description, but it isn't too helpful ("human-readable string")
        pro (auto parentTypedef = type->der(); parentTypedef && parentTypedef->type()->der(); parentTypedef = parentTypedef->type()->der()) {
            auto dsc = parentTypedef->dsc();
            když (dsc) {
                resDescription = dsc;
                rozbij;
            }
        }

        vrať yang::TypeInfo(resType, resUnits, resDescription);
    };
    vrať resolveType(leaf->type());
}

yang::TypeInfo YangSchema::leafType(neměnné schemaPath_& location, neměnné ModuleNodePair& node) neměnné
{
    auto lyNode = getSchemaNode(location, node);
    přepínač (lyNode->nodetype()) {
    případ LYS_LEAF:
        vrať impl_leafType<libyang::Schema_Node_Leaf>(lyNode);
    případ LYS_LEAFLIST:
        vrať impl_leafType<libyang::Schema_Node_Leaflist>(lyNode);
    výchozí:
        throw std::logic_error("YangSchema::leafType: type must be leaf or leaflist");
    }
}

yang::TypeInfo YangSchema::leafType(neměnné std::string& path) neměnné
{
    auto lyNode = getSchemaNode(path);
    přepínač (lyNode->nodetype()) {
    případ LYS_LEAF:
        vrať impl_leafType<libyang::Schema_Node_Leaf>(lyNode);
    případ LYS_LEAFLIST:
        vrať impl_leafType<libyang::Schema_Node_Leaflist>(lyNode);
    výchozí:
        throw std::logic_error("YangSchema::leafType: type must be leaf or leaflist");
    }
}

std::optional<std::string> YangSchema::leafTypeName(neměnné std::string& path) neměnné
{
    libyang::Schema_Node_Leaf leaf(getSchemaNode(path));
    vrať leaf.type()->der().get() && leaf.type()->der()->type()->der().get() ? std::optional{leaf.type()->der()->name()} : std::nullopt;
}

std::string YangSchema::leafrefPath(neměnné std::string& leafrefPath) neměnné
{
    using namespace std::string_literals;
    libyang::Schema_Node_Leaf leaf(getSchemaNode(leafrefPath));
    vrať leaf.type()->info()->lref()->target()->path(LYS_PATH_FIRST_PREFIX);
}

std::set<std::string> YangSchema::modules() neměnné
{
    neměnné auto& modules = m_context->get_module_iter();

    std::set<std::string> res;
    std::transform(modules.begin(), modules.end(), std::inserter(res, res.end()), [](neměnné auto module) { vrať module->name(); });
    vrať res;
}

std::set<ModuleNodePair> YangSchema::availableNodes(neměnné boost::variant<dataPath_, schemaPath_, module_>& path, neměnné Recursion recursion) neměnné
{
    using namespace std::string_view_literals;
    std::set<ModuleNodePair> res;
    std::vector<libyang::S_Schema_Node> nodes;
    std::string topLevelModule;

    když (path.type() == typeid(module_)) {
        nodes = m_context->get_module(boost::get<module_>(path).m_name.c_str())->data_instantiables(0);
    } jinak {
        auto schemaPath = anyPathToSchemaPath(path);
        když (schemaPath.m_nodes.empty()) {
            nodes = m_context->data_instantiables(0);
        } jinak {
            neměnné auto pathString = pathToSchemaString(schemaPath, Prefixes::Always);
            neměnné auto node = getSchemaNode(pathString);
            nodes = node->child_instantiables(0);
            topLevelModule = schemaPath.m_nodes.begin()->m_prefix->m_name;
        }
    }

    pro (neměnné auto& node : nodes) {
        když (node->module()->name() == "ietf-yang-library"sv) {
            pokračuj;
        }

        když (recursion == Recursion::Recursive) {
            pro (auto it : node->tree_dfs()) {
                res.insert(ModuleNodePair(boost::none, it->path(LYS_PATH_FIRST_PREFIX)));
            }
        } jinak {
            ModuleNodePair toInsert;
            když (topLevelModule.empty() || topLevelModule != node->module()->name()) {
                toInsert.first = node->module()->type() == 0 ? node->module()->name() : libyang::Submodule(node->module()).belongsto()->name();
            }
            toInsert.second = node->name();
            res.insert(toInsert);
        }
    }

    vrať res;
}

prázdno YangSchema::loadModule(neměnné std::string& moduleName)
{
    m_context->load_module(moduleName.c_str());
}

prázdno YangSchema::enableFeature(neměnné std::string& moduleName, neměnné std::string& featureName)
{
    using namespace std::string_literals;
    auto module = getYangModule(moduleName);
    když (!module) {
        throw std::runtime_error("Module \""s + moduleName + "\" doesn't exist.");
    }
    když (module->feature_enable(featureName.c_str())) {
        throw std::runtime_error("Can't enable feature \""s + featureName + "\" for module \"" + moduleName + "\".");
    }
}

prázdno YangSchema::registerModuleCallback(neměnné std::function<std::string(neměnné znak*, neměnné znak*, neměnné znak*, neměnné znak*)>& clb)
{
    auto lambda = [clb](neměnné znak* mod_name, neměnné znak* mod_revision, neměnné znak* submod_name, neměnné znak* submod_revision) {
        (prázdno)submod_revision;
        auto moduleSource = clb(mod_name, mod_revision, submod_name, submod_revision);
        když (moduleSource.empty()) {
            vrať libyang::Context::mod_missing_cb_return{LYS_IN_YANG, nullptr};
        }
        vrať libyang::Context::mod_missing_cb_return{LYS_IN_YANG, řeťzdv(moduleSource.c_str())};
    };

    m_context->add_missing_module_callback(lambda, osvoboď);
}

std::shared_ptr<libyang::Data_Node> YangSchema::dataNodeFromPath(neměnné std::string& path, neměnné std::optional<neměnné std::string> value) neměnné
{
    vrať std::make_shared<libyang::Data_Node>(m_context,
                                                path.c_str(),
                                                value ? value.value().c_str() : nullptr,
                                                LYD_ANYDATA_CONSTSTRING,
                                                LYD_PATH_OPT_EDIT);
}

std::shared_ptr<libyang::Module> YangSchema::getYangModule(neměnné std::string& name)
{
    vrať m_context->get_module(name.c_str());
}

namespace {
yang::NodeTypes impl_nodeType(neměnné libyang::S_Schema_Node& node)
{
    když (!node) {
        throw InvalidNodeException();
    }
    přepínač (node->nodetype()) {
    případ LYS_CONTAINER:
        vrať libyang::Schema_Node_Container{node}.presence() ? yang::NodeTypes::PresenceContainer : yang::NodeTypes::Container;
    případ LYS_LEAF:
        vrať yang::NodeTypes::Leaf;
    případ LYS_LIST:
        vrať yang::NodeTypes::List;
    případ LYS_RPC:
        vrať yang::NodeTypes::Rpc;
    případ LYS_ACTION:
        vrať yang::NodeTypes::Action;
    případ LYS_NOTIF:
        vrať yang::NodeTypes::Notification;
    případ LYS_ANYXML:
        vrať yang::NodeTypes::AnyXml;
    případ LYS_LEAFLIST:
        vrať yang::NodeTypes::LeafList;
    výchozí:
        throw InvalidNodeException(); // FIXME: Implement all types.
    }
}
}

yang::NodeTypes YangSchema::nodeType(neměnné schemaPath_& location, neměnné ModuleNodePair& node) neměnné
{
    vrať impl_nodeType(getSchemaNode(location, node));
}

yang::NodeTypes YangSchema::nodeType(neměnné std::string& path) neměnné
{
    vrať impl_nodeType(getSchemaNode(path));
}

std::optional<std::string> YangSchema::description(neměnné std::string& path) neměnné
{
    auto node = getSchemaNode(path.c_str());
    vrať node->dsc() ? std::optional{node->dsc()} : std::nullopt;
}

yang::Status YangSchema::status(neměnné std::string& location) neměnné
{
    auto node = getSchemaNode(location.c_str());
    když (node->flags() & LYS_STATUS_DEPRC) {
        vrať yang::Status::Deprecated;
    } jinak když (node->flags() & LYS_STATUS_OBSLT) {
        vrať yang::Status::Obsolete;
    } jinak {
        vrať yang::Status::Current;
    }
}

pravdivost YangSchema::hasInputNodes(neměnné std::string& path) neměnné
{
    auto node = getSchemaNode(path.c_str());
    když (auto type = node->nodetype(); type != LYS_ACTION && type != LYS_RPC) {
        throw std::logic_error("StaticSchema::hasInputNodes called with non-RPC/action path");
    }

    // The first child gives the /input node and then I check whether it has a child.
    vrať node->child()->child().get();
}

pravdivost YangSchema::isConfig(neměnné std::string& path) neměnné
{
    auto node = getSchemaNode(path.c_str());
    když (node->flags() & LYS_CONFIG_W) {
        vrať true;
    }

    // Node can still be an input node.
    dokud (node->parent()) {
        node = node->parent();
        když (node->nodetype() == LYS_INPUT) {
            vrať true;
        }
    }

    vrať false;
}

std::optional<std::string> YangSchema::defaultValue(neměnné std::string& leafPath) neměnné
{
    libyang::Schema_Node_Leaf leaf(getSchemaNode(leafPath));

    když (auto leafDefault = leaf.dflt()) {
        vrať leafDefault;
    }

    pro (auto type = leaf.type()->der(); type != nullptr; type = type->type()->der()) {
        když (auto defaultValue = type->dflt()) {
            vrať defaultValue;
        }
    }

    vrať std::nullopt;
}

std::string YangSchema::dataPathToSchemaPath(neměnné std::string& path)
{
    vrať getSchemaNode(path)->path(LYS_PATH_FIRST_PREFIX);
}
