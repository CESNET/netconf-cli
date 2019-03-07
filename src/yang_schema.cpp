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

bool YangSchema::isModule(const schemaPath_&, const std::string& name) const
{
    const auto set = modules();
    return set.find(name) != set.end();
}

bool YangSchema::isContainer(const schemaPath_& location, const ModuleNodePair& node) const
{
    const auto schemaNode = getSchemaNode(location, node);
    return schemaNode && schemaNode->nodetype() == LYS_CONTAINER;
}

bool YangSchema::isLeaf(const schemaPath_& location, const ModuleNodePair& node) const
{
    const auto schemaNode = getSchemaNode(location, node);
    return schemaNode && schemaNode->nodetype() == LYS_LEAF;
}

bool YangSchema::isList(const schemaPath_& location, const ModuleNodePair& node) const
{
    const auto schemaNode = getSchemaNode(location, node);
    return schemaNode && schemaNode->nodetype() == LYS_LIST;
}

bool YangSchema::isPresenceContainer(const schemaPath_& location, const ModuleNodePair& node) const
{
    if (!isContainer(location, node))
        return false;
    return libyang::Schema_Node_Container(getSchemaNode(location, node)).presence();
}

bool YangSchema::leafEnumHasValue(const schemaPath_& location, const ModuleNodePair& node, const std::string& value) const
{
    auto enums = enumValues(location, node);

    return std::any_of(enums.begin(), enums.end(), [=](const auto& x) { return x == value; });
}

const std::set<std::string> YangSchema::enumValues(const schemaPath_& location, const ModuleNodePair& node) const
{
    if (!isLeaf(location, node) || leafType(location, node) != yang::LeafDataTypes::Enum)
        return {};

    libyang::Schema_Node_Leaf leaf(getSchemaNode(location, node));
    auto type = leaf.type();
    auto enm = type->info()->enums()->enm();
    // The enum can be a derived type and enm() only returns values,
    // if that specific typedef changed the possible values. So we go
    // up the hierarchy until we find a typedef that defined these values.
    while (enm.empty()) {
        type = type->der()->type();
        enm = type->info()->enums()->enm();
    }

    std::set<std::string> enumSet;
    std::transform(enm.begin(), enm.end(), std::inserter(enumSet, enumSet.end()), [](auto it) { return it->name(); });
    return enumSet;
}

bool YangSchema::listHasKey(const schemaPath_& location, const ModuleNodePair& node, const std::string& key) const
{
    if (!isList(location, node))
        return false;
    const auto keys = listKeys(location, node);
    return keys.find(key) != keys.end();
}

bool YangSchema::nodeExists(const std::string& location, const std::string& node) const
{
    const auto absPath = location + "/" + node;
    const auto set = m_context->find_path(absPath.c_str());
    return set->number() == 1;
}

libyang::S_Set YangSchema::getNodeSet(const schemaPath_& location, const ModuleNodePair& node) const
{
    std::string absPath = location.m_nodes.empty() ? "" : "/";
    absPath += pathToAbsoluteSchemaString(location) + "/" + fullNodeName(location, node);
    return m_context->find_path(absPath.c_str());
}

libyang::S_Schema_Node YangSchema::getSchemaNode(const schemaPath_& location, const ModuleNodePair& node) const
{
    const auto set = getNodeSet(location, node);
    if (!set)
        return nullptr;
    const auto& schemaSet = set->schema();
    if (set->number() != 1)
        return nullptr;
    return *schemaSet.begin();
}

const std::set<std::string> YangSchema::listKeys(const schemaPath_& location, const ModuleNodePair& node) const
{
    std::set<std::string> keys;
    if (!isList(location, node))
        return keys;
    libyang::Schema_Node_List list(getSchemaNode(location, node));
    const auto& keysVec = list.keys();

    std::transform(keysVec.begin(), keysVec.end(), std::inserter(keys, keys.begin()),
            [] (const auto& it) {return it->name();});
    return keys;
}

yang::LeafDataTypes YangSchema::leafType(const schemaPath_& location, const ModuleNodePair& node) const
{
    using namespace std::string_literals;
    if (!isLeaf(location, node))
        throw InvalidSchemaQueryException(fullNodeName(location, node) + " is not a leaf");

    libyang::Schema_Node_Leaf leaf(getSchemaNode(location, node));
    switch (leaf.type()->base()) {
    case LY_TYPE_STRING:
        return yang::LeafDataTypes::String;
    case LY_TYPE_DEC64:
        return yang::LeafDataTypes::Decimal;
    case LY_TYPE_BOOL:
        return yang::LeafDataTypes::Bool;
    case LY_TYPE_INT32:
        return yang::LeafDataTypes::Int;
    case LY_TYPE_UINT32:
        return yang::LeafDataTypes::Uint;
    case LY_TYPE_ENUM:
        return yang::LeafDataTypes::Enum;
    case LY_TYPE_BINARY:
        return yang::LeafDataTypes::Binary;
    default:
        throw UnsupportedYangTypeException("the type of "s + fullNodeName(location, node) + " is not supported");
    }
}

std::set<std::string> YangSchema::modules() const
{
    const auto& modules = m_context->get_module_iter();

    std::set<std::string> res;
    std::transform(modules.begin(), modules.end(),
                   std::inserter(res, res.end()),
                   [] (const auto module) { return module->name(); });
    return res;
}

std::set<std::string> YangSchema::childNodes(const schemaPath_& path, const Recursion recursion) const
{
    using namespace std::string_view_literals;
    std::set<std::string> res;
    std::vector<libyang::S_Schema_Node> nodes;

    if (path.m_nodes.empty()) {
        nodes = m_context->data_instantiables(0);
    } else {
        const auto absolutePath = "/" + pathToAbsoluteSchemaString(path);
        const auto set = m_context->find_path(absolutePath.c_str());
        const auto schemaSet = set->schema();
        for (auto it = (*schemaSet.begin())->child(); it; it = it->next()) {
            nodes.push_back(it);
        }
    }

    for (const auto node : nodes) {
        if (node->module()->name() == "ietf-yang-library"sv)
            continue;
        if (recursion == Recursion::Recursive) {
            for (auto it : node->tree_dfs()) {
                res.insert(it->path(LYS_PATH_FIRST_PREFIX));
            }
        } else {
            std::string toInsert;
            if (path.m_nodes.empty() || path.m_nodes.front().m_prefix.get().m_name != node->module()->name()) {
                toInsert += node->module()->name();
                toInsert += ":";
            }
            toInsert += node->name();
            res.insert(toInsert);
        }
    }

    return res;
}

void YangSchema::loadModule(const std::string& moduleName)
{
    m_context->load_module(moduleName.c_str());
}

void YangSchema::registerModuleCallback(const std::function<std::string(const char*, const char*, const char*)>& clb)
{
    auto lambda = [clb](const char* mod_name, const char* mod_revision, const char* submod_name, const char* submod_revision) {
        (void)submod_revision;
        auto moduleSource = clb(mod_name, mod_revision, submod_name);
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
                                                0);
}

std::shared_ptr<libyang::Module> YangSchema::getYangModule(const std::string& name)
{
    return m_context->get_module(name.c_str(), nullptr, 0);
}
