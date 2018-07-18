/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/

#include <libyang/Libyang.hpp>
#include <libyang/Tree_Schema.hpp>
#include "utils.hpp"
#include "yang_schema.hpp"


class YangLoadError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
    ~YangLoadError() override = default;
};

std::string pathToYangAbsSchemPath(const path_& path)
{
    std::string res = "/";
    std::string currentModule;

    for (const auto& it : path.m_nodes) {
        auto name = nodeToSchemaString(it);

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
{
    m_context = std::make_shared<Context>();
}

void YangSchema::addSchemaString(const char* schema)
{
    auto m_moduleTEMP = m_context->parse_module_mem(schema, LYS_IN_YANG);
    if (!m_moduleTEMP) {
        throw YangLoadError("Couldn't load schema");
    }
}

void YangSchema::addSchemaDirectory(const char* directoryName)
{
    if (m_context->set_searchdir(directoryName) == EXIT_FAILURE) {
        throw YangLoadError("Couldn't add schema search directory");
    }
}

void YangSchema::addSchemaFile(const char* filename)
{
    auto m_moduleTEMP = m_context->parse_module_path(filename, LYS_IN_YANG);
    if (!m_moduleTEMP) {
        throw YangLoadError("Couldn't load schema");
    }
}

bool YangSchema::isModule(const path_&, const std::string& name) const
{
    auto set = modules();
    return set.find(name) != set.end();
}

bool YangSchema::isContainer(const path_& location, const ModuleNodePair& node) const
{
    auto set = getNodeSet(location, node);
    if (!set)
        return false;
    return set->number() == 1 && (*(set->schema()->begin()))->nodetype() == LYS_CONTAINER;
}

bool YangSchema::isLeaf(const path_& location, const ModuleNodePair& node) const
{
    auto set = getNodeSet(location, node);
    if (!set)
        return false;
    return set->number() == 1 && (*(set->schema()->begin()))->nodetype() == LYS_LEAF;
}

bool YangSchema::isList(const path_& location, const ModuleNodePair& node) const
{
    auto set = getNodeSet(location, node);
    if (!set)
        return false;
    return set->number() == 1 && (*(set->schema()->begin()))->nodetype() == LYS_LIST;
}

bool YangSchema::isPresenceContainer(const path_& location, const ModuleNodePair& node) const
{
    if (!isContainer(location, node))
        return false;
    auto set = getNodeSet(location, node);
    auto container = std::make_shared<Schema_Node_Container>(set->schema()->back());
    return container->presence();
}

bool YangSchema::leafEnumHasValue(const path_& location, const ModuleNodePair& node, const std::string& value) const
{
    if (!isLeaf(location, node) || leafType(location, node) != yang::LeafDataTypes::Enum)
        return false;

    auto set = getNodeSet(location, node);
    auto schemaSet = std::shared_ptr<std::vector<std::shared_ptr<Schema_Node>>>(set->schema());
    auto leaf = std::make_shared<Schema_Node_Leaf>(set->schema()->back());
    auto enm = std::shared_ptr<std::vector<S_Type_Enum>>(leaf->type()->info()->enums()->enm());
    for (auto it : *enm) {
        if (it->name() == value)
            return true;
    }
    return false;
}

bool YangSchema::listHasKey(const path_& location, const ModuleNodePair& node, const std::string& key) const
{
    if (!isList(location, node))
        return false;
    auto keys = listKeys(location, node);
    return keys.find(key) != keys.end();
}

bool YangSchema::nodeExists(const std::string& location, const std::string& node) const
{
    auto absPath = location + "/" + node;
    auto set = m_context->find_path(absPath.c_str());
    return set->number() == 1;
}

S_Set YangSchema::getNodeSet(const path_& location, const ModuleNodePair& node) const
{
    std::string absPath = location.m_nodes.empty() ? "" : "/";
    absPath += pathToAbsoluteSchemaString(location) + "/" + fullNodeName(location, node);
    auto set = m_context->find_path(absPath.c_str());
    return set;
}

const std::set<std::string> YangSchema::listKeys(const path_& location, const ModuleNodePair& node) const
{
    std::set<std::string> keys;
    auto set = getNodeSet(location, node);
    auto list = std::make_shared<Schema_Node_List>(set->schema()->back());
    for (auto it : *(list->keys())) {
        keys.insert(it->name());
    }
    return keys;
}

yang::LeafDataTypes YangSchema::leafType(const path_& location, const ModuleNodePair& node) const
{
    auto set = getNodeSet(location, node);
    auto leaf = std::make_shared<Schema_Node_Leaf>(set->schema()->back());

    switch (leaf->type()->base()) {
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
    default:
        return yang::LeafDataTypes::Unknown;
    }
}

std::set<std::string> YangSchema::modules() const
{
    auto modules = m_context->get_module_iter();

    std::set<std::string> res;
    std::transform(modules->begin(), modules->end(),
                   std::inserter(res, res.end()),
                   [] (auto module) { return module->name(); });
    return res;
}

std::set<std::string> YangSchema::childNodes(const path_& path) const
{
    std::set<std::string> res;
    if (path.m_nodes.empty()) {
        auto lol = m_context->data_instantiables(0);
        for (auto it : *lol) {
            if (std::string(it->module()->name()) == "ietf-yang-library")
                continue;
            std::string fullname = it->module()->name();
            fullname += ":";
            fullname += it->name();
            res.insert(fullname);
        }
    } else {
        S_Schema_Node it;
        auto absolutePath = "/" + pathToAbsoluteSchemaString(path);
        auto set = m_context->find_path(absolutePath.c_str());
        it = (*(set->schema()->begin()))->child();
        for (; it; it = it->next()) {
            std::string fullname = it->module()->name();
            fullname += ":";
            fullname += it->name();
            res.insert(fullname);
        }
    }
    return res;
}
