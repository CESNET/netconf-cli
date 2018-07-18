/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/

#include <iostream>
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

    if (path.m_nodes.back().m_suffix.type() == typeid(module_)) {
        res += currentModule + ":*";
    }

    std::cout << "pathToYangAbsolutePath res = " << res << std::endl;

    return res;
}

YangSchema::YangSchema()
{


    /*
    {
        std::cout << "hledam /a/a2" << std::endl;
        auto set = ctx->find_path("/example-schema:a/example-schema:a2");
        std::cout << "pocet nalezenych: " << set->number() << std::endl;
        std::cout << "prvni: " << set->schema()->back()->path() << std::endl;
        if (set->schema()->back()->nodetype() == LYS_CONTAINER)
            std::cout << "je to kontejner" << std::endl;

        std::cout << "1st child: " << set->schema()->back()->child()->path() << std::endl;
    }

        std::cout << std::endl;

    {
        std::cout << "hledam /list" << std::endl;
        auto set = ctx->find_path("/example-schema:_list");
        std::cout << "nalezeno: " << set->number() << std::endl;
        if (set->schema()->back()->nodetype() == LYS_LIST)
            std::cout << "je to list" << std::endl;

        auto list = std::make_shared<Schema_Node_List>(set->schema()->back());

        std::cout << "keys of the list: " << list->keys_str() << std::endl;


        std::cout << "1st child: " << set->schema()->back()->child()->path() << std::endl;
    }

    {
        std::cout << "hledam /twoKeyList" << std::endl;
        auto set = ctx->find_path("/example-schema:twoKeyList");
        std::cout << "nalezeno: " << set->number() << std::endl;
        if (set->schema()->back()->nodetype() == LYS_LIST)
            std::cout << "je to list" << std::endl;

        auto list = std::make_shared<Schema_Node_List>(set->schema()->back());

        std::cout << "keys of the list: " << list->keys_str() << std::endl;


        std::cout << "1st child: " << set->schema()->back()->child()->path() << std::endl;
    }*/
}

YangSchema::YangSchema(const char* filename)
{
    set_log_verbosity(LY_LLDBG);
    m_context = std::make_shared<Context>();
    if (!m_context) {
        throw YangLoadError("Couldn't load schema");
    }

    m_moduleTEMP = m_context->parse_module_path(filename, LYS_IN_YANG);
    if (!m_moduleTEMP) {
        throw YangLoadError("Couldn't load schema");
    }
}

bool YangSchema::isModule(const path_& location, const std::string& name) const
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
    return false;
}

bool YangSchema::isList(const path_& location, const ModuleNodePair& node) const
{
    return false;
}

bool YangSchema::isPresenceContainer(const path_& location, const ModuleNodePair& node) const
{
    return false;
}

bool YangSchema::leafEnumHasValue(const path_& location, const ModuleNodePair& node, const std::string& value) const
{
    return false;
}

bool YangSchema::listHasKey(const path_& location, const ModuleNodePair& node, const std::string& key) const
{
    return false;
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

const std::set<std::string>& YangSchema::listKeys(const path_& location, const ModuleNodePair& node) const
{
    return std::set<std::string>();
}

yang::LeafDataTypes YangSchema::leafType(const path_& location, const ModuleNodePair& node) const
{
    return yang::LeafDataTypes::String;
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
    if (path.m_nodes.empty())
        return modules();

    auto absolutePath = "/" + pathToAbsoluteSchemaString(path);
    auto set = m_context->find_path(absolutePath.c_str());

    std::set<std::string> res;

    for (auto it = (*(set->schema()->begin()))->child(); it; it = it->next())
        res.insert(it->name());

    return res;
}
