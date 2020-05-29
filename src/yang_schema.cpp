/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/

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
    ~YangLoadError() override = default;
};

class UnsupportedYangTypeException : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
    ~UnsupportedYangTypeException() override = default;
};

class InvalidSchemaQueryException : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
    ~InvalidSchemaQueryException() override = default;
};

template <typename T>
std::string pathToYangAbsSchemPath(const T& path)
{
    std::string res = "/";
    std::string currentModule;

    for (const auto& it : path.m_nodes) {
        const auto name = nodeToSchemaString(it);

        if (it.m_suffix.type() == typeid(module_)) {
            currentModule = name;
            continue;
        } else {
            res += currentModule + ":";
            res += name + "/";
        }
    }

    return res;
}

YangSchema::YangSchema()
    : m_context(std::make_shared<libyang::Context>(nullptr, LY_CTX_DISABLE_SEARCHDIRS | LY_CTX_DISABLE_SEARCHDIR_CWD))
{
}

YangSchema::YangSchema(std::shared_ptr<libyang::Context> lyCtx)
    : m_context(lyCtx)
{

}

YangSchema::~YangSchema() = default;

void YangSchema::addSchemaString(const char* schema)
{
    if (!m_context->parse_module_mem(schema, LYS_IN_YANG)) {
        throw YangLoadError("Couldn't load schema");
    }
}

void YangSchema::addSchemaDirectory(const char* directoryName)
{
    if (m_context->set_searchdir(directoryName)) {
        throw YangLoadError("Couldn't add schema search directory");
    }
}

void YangSchema::addSchemaFile(const char* filename)
{
    if (!m_context->parse_module_path(filename, LYS_IN_YANG)) {
        throw YangLoadError("Couldn't load schema");
    }
}

bool YangSchema::isModule(const std::string& name) const
{
    const auto set = modules();
    return set.find(name) != set.end();
}

bool YangSchema::listHasKey(const schemaPath_& listPath, const std::string& key) const
{
    const auto keys = listKeys(listPath);
    return keys.find(key) != keys.end();
}

bool YangSchema::leafIsKey(const std::string& leafPath) const
{
    auto node = getSchemaNode(leafPath);
    if (!node || node->nodetype() != LYS_LEAF)
        return false;

    return libyang::Schema_Node_Leaf{node}.is_key().get();
}

libyang::S_Schema_Node YangSchema::impl_getSchemaNode(const std::string& node) const
{
    // If no node is found find_path prints an error message, so we have to
    // disable logging
    // https://github.com/CESNET/libyang/issues/753
    {
        int oldOptions;
        auto logBlocker = make_unique_resource(
            [&oldOptions]() {
                oldOptions = libyang::set_log_options(0);
            },
            [&oldOptions]() {
                libyang::set_log_options(oldOptions);
            });
        return m_context->get_node(nullptr, node.c_str());
    }
}


libyang::S_Schema_Node YangSchema::getSchemaNode(const std::string& node) const
{
    return impl_getSchemaNode(node);
}

libyang::S_Schema_Node YangSchema::getSchemaNode(const schemaPath_& location, const ModuleNodePair& node) const
{
    std::string absPath = joinPaths(pathToSchemaString(location, Prefixes::Always), fullNodeName(location, node));

    return impl_getSchemaNode(absPath);
}

libyang::S_Schema_Node YangSchema::getSchemaNode(const schemaPath_& listPath) const
{
    std::string absPath = pathToSchemaString(listPath, Prefixes::Always);
    return impl_getSchemaNode(absPath);
}

const std::set<std::string> YangSchema::listKeys(const schemaPath_& listPath) const
{
    auto node = getSchemaNode(listPath);
    if (node->nodetype() != LYS_LIST) {
        return {};
    }

    auto list = std::make_shared<libyang::Schema_Node_List>(node);
    std::set<std::string> keys;
    const auto& keysVec = list->keys();

    std::transform(keysVec.begin(), keysVec.end(), std::inserter(keys, keys.begin()), [](const auto& it) { return it->name(); });
    return keys;
}

namespace {
std::set<enum_> enumValues(const libyang::S_Type& typeArg)
{
    auto type = typeArg;
    auto enm = type->info()->enums()->enm();
    // The enum can be a derived type and enm() only returns values,
    // if that specific typedef changed the possible values. So we go
    // up the hierarchy until we find a typedef that defined these values.
    while (enm.empty()) {
        type = type->der()->type();
        enm = type->info()->enums()->enm();
    }

    std::vector<libyang::S_Type_Enum> enabled;
    std::copy_if(enm.begin(), enm.end(), std::back_inserter(enabled), [](const libyang::S_Type_Enum& it) {
        auto iffeatures = it->iffeature();
        return std::all_of(iffeatures.begin(), iffeatures.end(), [](auto it) { return it->value(); });
    });

    std::set<enum_> enumSet;
    std::transform(enabled.begin(), enabled.end(), std::inserter(enumSet, enumSet.end()), [](auto it) { return enum_{it->name()}; });
    return enumSet;
}

std::set<identityRef_> validIdentities(const libyang::S_Type& type)
{
    std::set<identityRef_> identSet;

    // auto topLevelModule = leaf->module();

    auto info = type->info();
    for (auto base : info->ident()->ref()) { // Iterate over all bases
        identSet.emplace(base->module()->name(), base->name());
        // Iterate over derived identities (this is recursive!)
        for (auto derived : base->der()->schema()) {
            identSet.emplace(derived->module()->name(), derived->name());
        }
    }

    return identSet;
}

std::string leafrefPath(const libyang::S_Type& type)
{
    return type->info()->lref()->target()->path(LYS_PATH_FIRST_PREFIX);
}
}

template <typename NodeType>
yang::TypeInfo YangSchema::impl_leafType(const libyang::S_Schema_Node& node) const
{
    using namespace std::string_literals;
    auto leaf = std::make_shared<NodeType>(node);
    auto leafUnits = leaf->units();
    std::function<yang::TypeInfo(std::shared_ptr<libyang::Type>)> resolveType;
    resolveType = [this, &resolveType, leaf, leafUnits] (std::shared_ptr<libyang::Type> type) -> yang::TypeInfo {
        yang::LeafDataType resType;
        switch (type->base()) {
        case LY_TYPE_STRING:
            resType.emplace<yang::String>();
            break;
        case LY_TYPE_DEC64:
            resType.emplace<yang::Decimal>();
            break;
        case LY_TYPE_BOOL:
            resType.emplace<yang::Bool>();
            break;
        case LY_TYPE_INT8:
            resType.emplace<yang::Int8>();
            break;
        case LY_TYPE_INT16:
            resType.emplace<yang::Int16>();
            break;
        case LY_TYPE_INT32:
            resType.emplace<yang::Int32>();
            break;
        case LY_TYPE_INT64:
            resType.emplace<yang::Int64>();
            break;
        case LY_TYPE_UINT8:
            resType.emplace<yang::Uint8>();
            break;
        case LY_TYPE_UINT16:
            resType.emplace<yang::Uint16>();
            break;
        case LY_TYPE_UINT32:
            resType.emplace<yang::Uint32>();
            break;
        case LY_TYPE_UINT64:
            resType.emplace<yang::Uint64>();
            break;
        case LY_TYPE_BINARY:
            resType.emplace<yang::Binary>();
            break;
        case LY_TYPE_EMPTY:
            resType.emplace<yang::Empty>();
            break;
        case LY_TYPE_ENUM:
            resType.emplace<yang::Enum>(enumValues(type));
            break;
        case LY_TYPE_IDENT:
            resType.emplace<yang::IdentityRef>(validIdentities(type));
            break;
        case LY_TYPE_LEAFREF:
            resType.emplace<yang::LeafRef>(::leafrefPath(type), std::make_unique<yang::TypeInfo>(leafType(::leafrefPath(type))));
            break;
        case LY_TYPE_UNION:
            {
                auto resUnion = yang::Union{};
                for (auto unionType : type->info()->uni()->types()) {
                    resUnion.m_unionTypes.push_back(resolveType(unionType));
                }
                resType.emplace<yang::Union>(std::move(resUnion));
                break;
            }
        default:
            using namespace std::string_literals;
            throw UnsupportedYangTypeException("the type of "s + leaf->name() + " is not supported: " + std::to_string(leaf->type()->base()));
        }

        std::optional<std::string> resUnits;

        if (leafUnits) {
            resUnits = leafUnits;
        } else {
            for (auto parentTypedef = type->der(); parentTypedef; parentTypedef = parentTypedef->type()->der()) {
                auto units = parentTypedef->units();
                if (units) {
                    resUnits = units;
                    break;
                }
            }
        }

        return yang::TypeInfo(resType, resUnits);
    };
    return resolveType(leaf->type());
}

yang::TypeInfo YangSchema::leafType(const schemaPath_& location, const ModuleNodePair& node) const
{
    auto lyNode = getSchemaNode(location, node);
    switch (lyNode->nodetype()) {
    case LYS_LEAF:
        return impl_leafType<libyang::Schema_Node_Leaf>(lyNode);
    case LYS_LEAFLIST:
        return impl_leafType<libyang::Schema_Node_Leaflist>(lyNode);
    default:
        throw std::logic_error("YangSchema::leafType: type must be leaf or leaflist");
    }
}

yang::TypeInfo YangSchema::leafType(const std::string& path) const
{
    auto lyNode = getSchemaNode(path);
    switch (lyNode->nodetype()) {
    case LYS_LEAF:
        return impl_leafType<libyang::Schema_Node_Leaf>(lyNode);
    case LYS_LEAFLIST:
        return impl_leafType<libyang::Schema_Node_Leaflist>(lyNode);
    default:
        throw std::logic_error("YangSchema::leafType: type must be leaf or leaflist");
    }
}

std::optional<std::string> YangSchema::leafTypeName(const std::string& path) const
{
    libyang::Schema_Node_Leaf leaf(getSchemaNode(path));
    return leaf.type()->der().get() && leaf.type()->der()->type()->der().get() ? std::optional{leaf.type()->der()->name()} : std::nullopt;
}

std::string YangSchema::leafrefPath(const std::string& leafrefPath) const
{
    using namespace std::string_literals;
    libyang::Schema_Node_Leaf leaf(getSchemaNode(leafrefPath));
    return leaf.type()->info()->lref()->target()->path(LYS_PATH_FIRST_PREFIX);
}

std::set<std::string> YangSchema::modules() const
{
    const auto& modules = m_context->get_module_iter();

    std::set<std::string> res;
    std::transform(modules.begin(), modules.end(), std::inserter(res, res.end()), [](const auto module) { return module->name(); });
    return res;
}

std::set<ModuleNodePair> YangSchema::availableNodes(const boost::variant<dataPath_, schemaPath_, module_>& path, const Recursion recursion) const
{
    using namespace std::string_view_literals;
    std::set<ModuleNodePair> res;
    std::vector<libyang::S_Schema_Node> nodes;
    std::string topLevelModule;

    if (path.type() == typeid(module_)) {
        nodes = m_context->get_module(boost::get<module_>(path).m_name.c_str())->data_instantiables(0);
    } else {
        auto schemaPath = anyPathToSchemaPath(path);
        if (schemaPath.m_nodes.empty()) {
            nodes = m_context->data_instantiables(0);
        } else {
            const auto pathString = pathToSchemaString(schemaPath, Prefixes::Always);
            const auto node = getSchemaNode(pathString);
            nodes = node->child_instantiables(0);
            topLevelModule = schemaPath.m_nodes.begin()->m_prefix->m_name;
        }
    }

    for (const auto& node : nodes) {
        if (node->module()->name() == "ietf-yang-library"sv)
            continue;

        if (recursion == Recursion::Recursive) {
            for (auto it : node->tree_dfs()) {
                res.insert(ModuleNodePair(boost::none, it->path(LYS_PATH_FIRST_PREFIX)));
            }
        } else {
            ModuleNodePair toInsert;
            if (topLevelModule.empty() || topLevelModule != node->module()->name()) {
                toInsert.first = node->module()->type() == 0 ? node->module()->name() : libyang::Submodule(node->module()).belongsto()->name();
            }
            toInsert.second = node->name();
            res.insert(toInsert);
        }
    }

    return res;
}

void YangSchema::loadModule(const std::string& moduleName)
{
    m_context->load_module(moduleName.c_str());
}

void YangSchema::enableFeature(const std::string& moduleName, const std::string& featureName)
{
    m_context->get_module(moduleName.c_str())->feature_enable(featureName.c_str());
}

void YangSchema::registerModuleCallback(const std::function<std::string(const char*, const char*, const char*, const char*)>& clb)
{
    auto lambda = [clb](const char* mod_name, const char* mod_revision, const char* submod_name, const char* submod_revision) {
        (void)submod_revision;
        auto moduleSource = clb(mod_name, mod_revision, submod_name, submod_revision);
        if (moduleSource.empty()) {
            return libyang::Context::mod_missing_cb_return{LYS_IN_YANG, nullptr};
        }
        return libyang::Context::mod_missing_cb_return{LYS_IN_YANG, strdup(moduleSource.c_str())};
    };

    auto deleter = [](void* data) {
        free(data);
    };
    m_context->add_missing_module_callback(lambda, deleter);
}

std::shared_ptr<libyang::Data_Node> YangSchema::dataNodeFromPath(const std::string& path, const std::optional<const std::string> value) const
{
    return std::make_shared<libyang::Data_Node>(m_context,
                                                path.c_str(),
                                                value ? value.value().c_str() : nullptr,
                                                LYD_ANYDATA_CONSTSTRING,
                                                LYD_PATH_OPT_EDIT);
}

std::shared_ptr<libyang::Module> YangSchema::getYangModule(const std::string& name)
{
    return m_context->get_module(name.c_str(), nullptr, 0);
}

namespace {
yang::NodeTypes impl_nodeType(const libyang::S_Schema_Node& node)
{
    if (!node) {
        throw InvalidNodeException();
    }
    switch (node->nodetype()) {
    case LYS_CONTAINER:
        return libyang::Schema_Node_Container{node}.presence() ? yang::NodeTypes::PresenceContainer : yang::NodeTypes::Container;
    case LYS_LEAF:
        return yang::NodeTypes::Leaf;
    case LYS_LIST:
        return yang::NodeTypes::List;
    case LYS_RPC:
        return yang::NodeTypes::Rpc;
    case LYS_ACTION:
        return yang::NodeTypes::Action;
    case LYS_NOTIF:
        return yang::NodeTypes::Notification;
    case LYS_ANYXML:
        return yang::NodeTypes::AnyXml;
    case LYS_LEAFLIST:
        return yang::NodeTypes::LeafList;
    default:
        throw InvalidNodeException(); // FIXME: Implement all types.
    }
}
}

yang::NodeTypes YangSchema::nodeType(const schemaPath_& location, const ModuleNodePair& node) const
{
    return impl_nodeType(getSchemaNode(location, node));
}

yang::NodeTypes YangSchema::nodeType(const std::string& path) const
{
    return impl_nodeType(getSchemaNode(path));
}

std::optional<std::string> YangSchema::description(const std::string& path) const
{
    auto node = getSchemaNode(path.c_str());
    return node->dsc() ? std::optional{node->dsc()} : std::nullopt;
}

yang::Status YangSchema::status(const std::string& location) const
{
    auto node = getSchemaNode(location.c_str());
    if (node->flags() & LYS_STATUS_DEPRC) {
        return yang::Status::Deprecated;
    } else if (node->flags() & LYS_STATUS_OBSLT) {
        return yang::Status::Obsolete;
    } else {
        return yang::Status::Current;
    }
}

bool YangSchema::isConfig(const std::string& path) const
{
    return getSchemaNode(path.c_str())->flags() & LYS_CONFIG_W;
}

std::optional<std::string> YangSchema::defaultValue(const std::string& leafPath) const
{
    libyang::Schema_Node_Leaf leaf(getSchemaNode(leafPath));

    if (auto leafDefault = leaf.dflt()) {
        return leafDefault;
    }

    for (auto type = leaf.type()->der(); type != nullptr; type = type->type()->der()) {
        if (auto defaultValue = type->dflt()) {
            return defaultValue;
        }
    }

    return std::nullopt;
}
