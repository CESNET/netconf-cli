/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/

#include <algorithm>
#include <libyang-cpp/Enum.hpp>
#include <libyang-cpp/Utils.hpp>
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

YangSchema::YangSchema()
    : m_context(std::nullopt, libyang::ContextOptions::DisableSearchDirs | libyang::ContextOptions::SetPrivParsed)
{
}

YangSchema::YangSchema(libyang::Context lyCtx)
    : m_context(lyCtx)
{
}

YangSchema::~YangSchema() = default;

void YangSchema::addSchemaString(const char* schema)
{
    m_context.parseModule(std::string{schema}, libyang::SchemaFormat::YANG);
}

void YangSchema::addSchemaDirectory(const std::filesystem::path& directory)
{
    m_context.setSearchDir(directory);
}

void YangSchema::addSchemaFile(const std::filesystem::path& filename)
{
    m_context.parseModule(filename, libyang::SchemaFormat::YANG);
}

bool YangSchema::isModule(const std::string& name) const
{
    return m_context.getModuleImplemented(name).has_value();
}

bool YangSchema::listHasKey(const schemaPath_& listPath, const std::string& key) const
{
    const auto keys = listKeys(listPath);
    return keys.find(key) != keys.end();
}

bool YangSchema::leafIsKey(const std::string& leafPath) const
{
    auto node = getSchemaNode(leafPath);
    if (!node || node->nodeType() != libyang::NodeType::Leaf) {
        return false;
    }

    return node->asLeaf().isKey();
}

std::optional<libyang::SchemaNode> YangSchema::impl_getSchemaNode(const std::string& node) const
{
    // libyang::Context::findPath throws an exception, when no matching schema node is found. This exception has the
    // ValidationFailure error code. We will catch that exception (and rethrow if it's not the correct error code.
    //
    // Also, we need to use findPath twice if we're trying to find output nodes.
    try {
        return m_context.findPath(node);
    } catch (libyang::ErrorWithCode& err) {
        if (err.code() != libyang::ErrorCode::ValidationFailure) {
            throw;
        }
    }
    try {
        return m_context.findPath(node, libyang::InputOutputNodes::Output);
    } catch (libyang::ErrorWithCode& err) {
        if (err.code() != libyang::ErrorCode::ValidationFailure) {
            throw;
        }
    }

    // We didn't find a matching node.
    return std::nullopt;
}


std::optional<libyang::SchemaNode> YangSchema::getSchemaNode(const std::string& node) const
{
    return impl_getSchemaNode(node);
}

std::optional<libyang::SchemaNode> YangSchema::getSchemaNode(const schemaPath_& location, const ModuleNodePair& node) const
{
    std::string absPath = joinPaths(pathToSchemaString(location, Prefixes::Always), fullNodeName(location, node));

    return impl_getSchemaNode(absPath);
}

std::optional<libyang::SchemaNode> YangSchema::getSchemaNode(const schemaPath_& listPath) const
{
    std::string absPath = pathToSchemaString(listPath, Prefixes::Always);
    return impl_getSchemaNode(absPath);
}

const std::set<std::string> YangSchema::listKeys(const schemaPath_& listPath) const
{
    auto node = getSchemaNode(listPath);
    if (node->nodeType() != libyang::NodeType::List) {
        return {};
    }

    std::set<std::string> keys;
    auto keysVec = node->asList().keys();

    std::transform(keysVec.begin(), keysVec.end(), std::inserter(keys, keys.begin()), [](const auto& it) { return it.name(); });
    return keys;
}

std::set<enum_> enumValues(const libyang::types::Type& type)
{
    auto enums = type.asEnum().items();
    std::set<enum_> enumSet;
    std::transform(enums.begin(), enums.end(), std::inserter(enumSet, enumSet.end()), [](auto it) { return enum_{it.name}; });
    return enumSet;
}

std::set<identityRef_> validIdentities(const libyang::types::Type& type)
{
    std::set<identityRef_> identSet;

    std::function<void(const std::vector<libyang::Identity>&)> impl = [&identSet, &impl] (const std::vector<libyang::Identity>& idents) {
        if (idents.empty()) {
            return;
        }

        for (const auto& ident : idents) {
            identSet.emplace(ident.module().name(), ident.name());
            impl(ident.derived());
        }
    };

    for (const auto& base : type.asIdentityRef().bases()) {
        impl(base.derived());
    }

    return identSet;
}

std::string leafrefPath(const libyang::types::Type& type)
{
    return type.asLeafRef().path();
}

template <typename NodeType>
yang::TypeInfo YangSchema::impl_leafType(const NodeType& node) const
{
    using namespace std::string_literals;
    auto leaf = std::make_shared<NodeType>(node);
    auto leafUnits = leaf->units();
    std::function<yang::TypeInfo(const libyang::types::Type&)> resolveType;
    resolveType = [&resolveType, leaf, leafUnits](const libyang::types::Type& type) -> yang::TypeInfo {
        yang::LeafDataType resType;
        switch (type.base()) {
        case libyang::LeafBaseType::String:
            resType.emplace<yang::String>();
            break;
        case libyang::LeafBaseType::Dec64:
            resType.emplace<yang::Decimal>();
            break;
        case libyang::LeafBaseType::Bool:
            resType.emplace<yang::Bool>();
            break;
        case libyang::LeafBaseType::Int8:
            resType.emplace<yang::Int8>();
            break;
        case libyang::LeafBaseType::Int16:
            resType.emplace<yang::Int16>();
            break;
        case libyang::LeafBaseType::Int32:
            resType.emplace<yang::Int32>();
            break;
        case libyang::LeafBaseType::Int64:
            resType.emplace<yang::Int64>();
            break;
        case libyang::LeafBaseType::Uint8:
            resType.emplace<yang::Uint8>();
            break;
        case libyang::LeafBaseType::Uint16:
            resType.emplace<yang::Uint16>();
            break;
        case libyang::LeafBaseType::Uint32:
            resType.emplace<yang::Uint32>();
            break;
        case libyang::LeafBaseType::Uint64:
            resType.emplace<yang::Uint64>();
            break;
        case libyang::LeafBaseType::Binary:
            resType.emplace<yang::Binary>();
            break;
        case libyang::LeafBaseType::Empty:
            resType.emplace<yang::Empty>();
            break;
        case libyang::LeafBaseType::Enum:
            resType.emplace<yang::Enum>(enumValues(type));
            break;
        case libyang::LeafBaseType::IdentityRef:
            resType.emplace<yang::IdentityRef>(validIdentities(type));
            break;
        case libyang::LeafBaseType::Leafref:
            resType.emplace<yang::LeafRef>(::leafrefPath(type), std::make_unique<yang::TypeInfo>(resolveType(type.asLeafRef().resolvedType())));
            break;
        case libyang::LeafBaseType::Bits: {
            auto resBits = yang::Bits{};
            for (const auto& bit : type.asBits().items()) {
                resBits.m_allowedValues.emplace(bit.name);
            }
            resType.emplace<yang::Bits>(std::move(resBits));
            break;
        }
        case libyang::LeafBaseType::Union: {
            auto resUnion = yang::Union{};
            for (auto unionType : type.asUnion().types()) {
                resUnion.m_unionTypes.emplace_back(resolveType(unionType));
            }
            resType.emplace<yang::Union>(std::move(resUnion));
            break;
        }
        case libyang::LeafBaseType::InstanceIdentifier: {
            resType.emplace<yang::InstanceIdentifier>();
            break;
        }
        default:
            using namespace std::string_literals;
            throw UnsupportedYangTypeException("the type of "s +
                    leaf->name() +
                    " is not supported: " +
                    std::to_string(std::underlying_type_t<libyang::LeafBaseType>(leaf->valueType().base())));
        }
        // note https://gcc.gnu.org/bugzilla/show_bug.cgi?id=109434
        std::optional<std::string> typeDesc;

        try {
            typeDesc = type.description();
        } catch (libyang::ParsedInfoUnavailable&) {
            // libyang context doesn't have the parsed info.
        }

        return yang::TypeInfo(resType, std::optional<std::string>{leafUnits}, typeDesc);
    };
    return resolveType(leaf->valueType());
}

yang::TypeInfo YangSchema::leafType(const schemaPath_& location, const ModuleNodePair& node) const
{
    auto lyNode = getSchemaNode(location, node);
    switch (lyNode->nodeType()) {
    case libyang::NodeType::Leaf:
        return impl_leafType(lyNode->asLeaf());
    case libyang::NodeType::Leaflist:
        return impl_leafType(lyNode->asLeafList());
    default:
        throw std::logic_error("YangSchema::leafType: type must be leaf or leaflist");
    }
}

yang::TypeInfo YangSchema::leafType(const std::string& path) const
{
    auto lyNode = getSchemaNode(path);
    switch (lyNode->nodeType()) {
    case libyang::NodeType::Leaf:
        return impl_leafType(lyNode->asLeaf());
    case libyang::NodeType::Leaflist:
        return impl_leafType(lyNode->asLeafList());
    default:
        throw std::logic_error("YangSchema::leafType: type must be leaf or leaflist");
    }
}

std::optional<std::string> YangSchema::leafTypeName(const std::string& path) const
{
    auto leaf = getSchemaNode(path)->asLeaf();
    try {
        return leaf.valueType().name();
    } catch (libyang::ParsedInfoUnavailable&) {
        return std::nullopt;
    }
}

std::string YangSchema::leafrefPath(const std::string& leafrefPath) const
{
    using namespace std::string_literals;
    return ::leafrefPath(getSchemaNode(leafrefPath)->asLeaf().valueType());
}

std::set<std::string> YangSchema::modules() const
{
    const auto& modules = m_context.modules();

    std::set<std::string> res;
    std::transform(modules.begin(), modules.end(), std::inserter(res, res.end()), [](const auto module) { return module.name(); });
    return res;
}

std::set<ModuleNodePair> YangSchema::availableNodes(const boost::variant<dataPath_, schemaPath_, module_>& path, const Recursion recursion) const
{
    std::set<ModuleNodePair> res;
    std::vector<libyang::ChildInstanstiables> nodeCollections;
    std::string topLevelModule;

    if (path.type() == typeid(module_)) {
        nodeCollections.emplace_back(m_context.getModuleLatest(boost::get<module_>(path).m_name)->childInstantiables());
    } else {
        auto schemaPath = anyPathToSchemaPath(path);
        if (schemaPath.m_nodes.empty()) {
            for (const auto& module : m_context.modules()) {
                if (module.implemented()) {
                    nodeCollections.emplace_back(module.childInstantiables());
                }
            }
        } else {
            const auto pathString = pathToSchemaString(schemaPath, Prefixes::Always);
            const auto node = getSchemaNode(pathString);
            nodeCollections.emplace_back(node->childInstantiables());
            topLevelModule = schemaPath.m_nodes.begin()->m_prefix->m_name;
        }
    }

    for (const auto& coll : nodeCollections) {
        for (const auto& node : coll) {
            if (node.module().name() == "ietf-yang-library") {
                continue;
            }

            if (node.module().name() == "ietf-yang-schema-mount") {
                continue;
            }

            if (recursion == Recursion::Recursive) {
                for (auto it : node.childrenDfs()) {
                    res.insert(ModuleNodePair(boost::none, it.path()));
                }
            } else {
                ModuleNodePair toInsert;
                if (topLevelModule.empty() || topLevelModule != node.module().name()) {
                    toInsert.first = node.module().name();
                }
                toInsert.second = node.name();
                res.insert(toInsert);
            }
        }
    }

    return res;
}

void YangSchema::loadModule(const std::string& moduleName)
{
    m_context.loadModule(moduleName);
}

void YangSchema::setEnabledFeatures(const std::string& moduleName, const std::vector<std::string>& features)
{
    using namespace std::string_literals;
    auto module = getYangModule(moduleName);
    if (!module) {
        throw std::runtime_error("Module \""s + moduleName + "\" doesn't exist.");
    }
    try {
        module->setImplemented(features);
    } catch (libyang::ErrorWithCode&) {
        throw std::runtime_error("Can't enable features for module \"" + moduleName + "\".");
    }
}

void YangSchema::registerModuleCallback(const std::function<std::string(const std::string&, const std::optional<std::string>&, const std::optional<std::string>&, const std::optional<std::string>&)>& clb)
{
    auto lambda = [clb](const auto mod_name, const auto mod_revision, const auto submod_name, const auto submod_revision) -> std::optional<libyang::ModuleInfo> {
        (void)submod_revision;
        auto moduleSource = clb(mod_name, mod_revision, submod_name, submod_revision);
        if (moduleSource.empty()) {
            return std::nullopt;
        }
        return libyang::ModuleInfo {
            .data = moduleSource,
            .format = libyang::SchemaFormat::YANG

        };
    };

    m_context.registerModuleCallback(lambda);
}

libyang::CreatedNodes YangSchema::dataNodeFromPath(const std::string& path, const std::optional<const std::string> value) const
{
    auto options = [this, &path, &value] {
        // If we're creating a node without a value and it's not the "empty" type, then we also need the Opaque flag.
        auto schema = getSchemaNode(path);
        if (schema->nodeType() == libyang::NodeType::Leaf &&
            schema->asLeaf().valueType().base() != libyang::LeafBaseType::Empty &&
            !value) {
            return std::optional<libyang::CreationOptions>{libyang::CreationOptions::Opaque};
        }

        return std::optional<libyang::CreationOptions>{};
    }();
    return m_context.newPath2(path, value, options);
}

std::optional<libyang::Module> YangSchema::getYangModule(const std::string& name)
{
    return m_context.getModuleImplemented(name);
}

namespace {
yang::NodeTypes impl_nodeType(const libyang::SchemaNode& node)
{
    switch (node.nodeType()) {
    case libyang::NodeType::Container:
        return node.asContainer().isPresence() ? yang::NodeTypes::PresenceContainer : yang::NodeTypes::Container;
    case libyang::NodeType::Leaf:
        return yang::NodeTypes::Leaf;
    case libyang::NodeType::List:
        return yang::NodeTypes::List;
    case libyang::NodeType::RPC:
        return yang::NodeTypes::Rpc;
    case libyang::NodeType::Action:
        return yang::NodeTypes::Action;
    case libyang::NodeType::Notification:
        return yang::NodeTypes::Notification;
    case libyang::NodeType::AnyXML:
        return yang::NodeTypes::AnyXml;
    case libyang::NodeType::Leaflist:
        return yang::NodeTypes::LeafList;
    default:
        throw InvalidNodeException(); // FIXME: Implement all types.
    }
}
}

yang::NodeTypes YangSchema::nodeType(const schemaPath_& location, const ModuleNodePair& node) const
{
    return impl_nodeType(*getSchemaNode(location, node));
}

yang::NodeTypes YangSchema::nodeType(const std::string& path) const
{
    return impl_nodeType(*getSchemaNode(path));
}

std::optional<std::string> YangSchema::description(const std::string& path) const
{
    auto desc = getSchemaNode(path.c_str())->description();
    return desc ? std::optional<std::string>{desc} : std::nullopt;

}

yang::Status YangSchema::status(const std::string& location) const
{
    auto node = getSchemaNode(location.c_str());
    switch (node->status()) {
    case libyang::Status::Deprecated:
        return yang::Status::Deprecated;
    case libyang::Status::Obsolete:
        return yang::Status::Obsolete;
    case libyang::Status::Current:
        return yang::Status::Current;
    }

    __builtin_unreachable();
}

bool YangSchema::hasInputNodes(const std::string& path) const
{
    auto node = getSchemaNode(path.c_str());
    if (auto type = node->nodeType(); type != libyang::NodeType::Action && type != libyang::NodeType::RPC) {
        throw std::logic_error("StaticSchema::hasInputNodes called with non-RPC/action path");
    }

    // The first child gives the /input node and then I check whether it has a child.
    return node->child()->child().has_value();
}

bool YangSchema::isConfig(const std::string& path) const
{
    auto node = getSchemaNode(path.c_str());
    if (node->isInput()) {
        return true;
    }

    try {
        if (node->config() == libyang::Config::True) {
            return true;
        }
    } catch (libyang::Error&) {
        // For non-data nodes (like `rpc`), the config value can't be retrieved. In this case, we'll just default to
        // "false".
    }

    return false;
}

std::optional<std::string> YangSchema::defaultValue(const std::string& leafPath) const
{
    return std::optional<std::string>{getSchemaNode(leafPath)->asLeaf().defaultValueStr()};
}

std::string YangSchema::dataPathToSchemaPath(const std::string& path)
{
    return getSchemaNode(path)->path();
}
