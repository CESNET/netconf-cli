/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/

#include <libyang/Libyang.hpp>
#include <libyang/Tree_Data.hpp>
#include "netconf-client.h"
#include "netconf_access.hpp"
#include "utils.hpp"
#include "yang_schema.hpp"

leaf_data_ leafValueFromValue(const libyang::S_Value& value, LY_DATA_TYPE type)
{
    using namespace std::string_literals;
    switch (type) {
    case LY_TYPE_INT32:
        return value->int32();
    case LY_TYPE_UINT32:
        return value->uintu32();
    case LY_TYPE_BOOL:
        return value->bln();
    case LY_TYPE_STRING:
        return std::string(value->string());
    case LY_TYPE_ENUM:
        return std::string(value->enm()->name());
    default: // TODO: implement all types
        return "(can't print)"s;
    }
}

NetconfAccess::~NetconfAccess() = default;

std::map<std::string, leaf_data_> NetconfAccess::getItems(const std::string& path)
{
    using namespace std::string_literals;
    std::map<std::string, leaf_data_> res;

    auto fillMap = [&res](auto items) {
        for (auto it : items) {
            if (!it)
                continue;
            if (it->schema()->nodetype() == LYS_LEAF) {
                libyang::Data_Node_Leaf_List leaf(it);
                res.emplace(leaf.path(), leafValueFromValue(leaf.value(), leaf.leaf_type()->base()));
            }
        }
    };

    if (path == "/") {
        for (auto it : m_session->getConfig(NC_DATASTORE_RUNNING)->tree_for()) {
            fillMap(it->tree_dfs());
        }
    } else {
        auto data = m_session->getConfig(NC_DATASTORE_RUNNING, path);
        fillMap(data->tree_dfs());
    }

    return res;
}

NetconfAccess::NetconfAccess(const std::string& hostname, uint16_t port, const std::string& user, const std::string& pubKey, const std::string& privKey)
    : m_schema(new YangSchema())
{
    m_session = libnetconf::client::Session::connectPubkey(hostname, port, user, pubKey, privKey);
    m_schema->registerModuleCallback([this](const char* moduleName, const char* revision, const char* submodule) {
        return fetchSchema(moduleName, revision, submodule);
    });

    for (const auto& it : listImplementedSchemas()) {
        m_schema->loadModule(it);
    }
}

void NetconfAccess::setLeaf(const std::string& path, leaf_data_ value)
{
    auto data = m_schema->dataNodeFromPath(path, leafDataToString(value))->print_mem(LYD_XML, 0);
    m_session->editConfig(NC_DATASTORE_RUNNING, NC_RPC_EDIT_DFLTOP_MERGE, NC_RPC_EDIT_TESTOPT_UNKNOWN, NC_RPC_EDIT_ERROPT_STOP, data);
}

void NetconfAccess::createPresenceContainer(const std::string& path)
{
    auto data = m_schema->dataNodeFromPath(path)->print_mem(LYD_XML, 0);
    m_session->editConfig(NC_DATASTORE_RUNNING, NC_RPC_EDIT_DFLTOP_MERGE, NC_RPC_EDIT_TESTOPT_UNKNOWN, NC_RPC_EDIT_ERROPT_STOP, data);
}

void NetconfAccess::deletePresenceContainer(const std::string& path)
{
    auto node = m_schema->dataNodeFromPath(path);
    node->insert_attr(m_schema->getYangModule("ietf-netconf"), "operation", "delete");
    auto data = node->print_mem(LYD_XML, 0);
    m_session->editConfig(NC_DATASTORE_RUNNING, NC_RPC_EDIT_DFLTOP_MERGE, NC_RPC_EDIT_TESTOPT_UNKNOWN, NC_RPC_EDIT_ERROPT_STOP, data);
}

void NetconfAccess::commitChanges()
{
    m_session->commit();
}

void NetconfAccess::discardChanges()
{
    m_session->discard();
}

std::string NetconfAccess::fetchSchema(const char* module, const char* revision, const char*)
{
    return m_session->getSchema(module, revision);
}

std::vector<std::string> NetconfAccess::listImplementedSchemas()
{
    auto data = m_session->get("/ietf-netconf-monitoring:netconf-state/schemas");
    auto set = data->find_path("/ietf-netconf-monitoring:netconf-state/schemas/schema/identifier");

    std::vector<std::string> res;
    for (auto it : set->data()) {
        if (it->schema()->nodetype() == LYS_LEAF) {
            libyang::Data_Node_Leaf_List leaf(it);
            res.push_back(leaf.value_str());
        }
    }
    return res;
}

std::shared_ptr<Schema> NetconfAccess::schema()
{
    return m_schema;
}
