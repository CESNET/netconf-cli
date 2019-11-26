/*
 * Copyright (C) 2019 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
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
    case LY_TYPE_INT8:
        return value->int8();
    case LY_TYPE_INT16:
        return value->int16();
    case LY_TYPE_INT32:
        return value->int32();
    case LY_TYPE_INT64:
        return value->int64();
    case LY_TYPE_UINT8:
        return value->uint8();
    case LY_TYPE_UINT16:
        return value->uint16();
    case LY_TYPE_UINT32:
        return value->uintu32();
    case LY_TYPE_UINT64:
        return value->uint64();
    case LY_TYPE_BOOL:
        return bool(value->bln());
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

    // This is very similar to the fillMap lambda in SysrepoAccess, however,
    // Sysrepo returns a weird array-like structure, while libnetconf
    // returns libyang::Data_Node
    auto fillMap = [&res](auto items) {
        for (const auto& it : items) {
            if (!it)
                continue;
            if (it->schema()->nodetype() == LYS_LEAF) {
                libyang::Data_Node_Leaf_List leaf(it);
                res.emplace(leaf.path(), leafValueFromValue(leaf.value(), leaf.leaf_type()->base()));
            }
        }
    };

    auto config = m_session->getConfig(NC_DATASTORE_RUNNING, (path != "/") ? std::optional{path} : std::nullopt);

    if (config) {
        // libnetconf always returns libyang data trees, including all the mandatory nodes.
        // For example, if getting a leaf which is a key of a list, the tree will include the list itself and also all of its other keys.
        // This means I have to use find_path to filter out nodes I actually wanted.
        // This also means I use `path` twice to filter the results, but that's how sysrepo does it:
        // https://github.com/CESNET/Netopeer2/issues/500
        fillMap(config->find_path(path.c_str())->data());
    }
    return res;
}

void NetconfAccess::datastoreInit()
{
    m_schema->registerModuleCallback([this](const char* moduleName, const char* revision, const char* submodule, const char* submoduleRevision) {
        return fetchSchema(moduleName,
                           revision ? std::optional{revision} : std::nullopt,
                           submodule ? std::optional{submodule} : std::nullopt,
                           submoduleRevision ? std::optional{submoduleRevision} : std::nullopt);
    });

    for (const auto& it : listImplementedSchemas()) {
        m_schema->loadModule(it);
    }
}

NetconfAccess::NetconfAccess(const std::string& hostname, uint16_t port, const std::string& user, const std::string& pubKey, const std::string& privKey)
    : m_schema(new YangSchema())
{
    m_session = libnetconf::client::Session::connectPubkey(hostname, port, user, pubKey, privKey);
    datastoreInit();
}

NetconfAccess::NetconfAccess(const std::string& socketPath)
    : m_schema(new YangSchema())
{
    m_session = libnetconf::client::Session::connectSocket(socketPath);
    datastoreInit();
}

void NetconfAccess::setLeaf(const std::string& path, leaf_data_ value)
{
    auto node = m_schema->dataNodeFromPath(path, leafDataToString(value));
    doEditFromDataNode(node);
}

void NetconfAccess::createPresenceContainer(const std::string& path)
{
    auto node = m_schema->dataNodeFromPath(path);
    doEditFromDataNode(node);
}

void NetconfAccess::deletePresenceContainer(const std::string& path)
{
    auto node = m_schema->dataNodeFromPath(path);
    node->insert_attr(m_schema->getYangModule("ietf-netconf"), "operation", "delete");
    doEditFromDataNode(node);
}

void NetconfAccess::createListInstance(const std::string& path)
{
    auto node = m_schema->dataNodeFromPath(path);
    doEditFromDataNode(node);
}

void NetconfAccess::deleteListInstance(const std::string& path)
{
    auto node = m_schema->dataNodeFromPath(path);
    node->child()->insert_attr(m_schema->getYangModule("ietf-netconf"), "operation", "delete");
    doEditFromDataNode(node);
}

void NetconfAccess::doEditFromDataNode(std::shared_ptr<libyang::Data_Node> dataNode)
{
    auto data = dataNode->print_mem(LYD_XML, 0);
    m_session->editConfig(NC_DATASTORE_CANDIDATE, NC_RPC_EDIT_DFLTOP_MERGE, NC_RPC_EDIT_TESTOPT_TESTSET, NC_RPC_EDIT_ERROPT_STOP, data);
}

void NetconfAccess::commitChanges()
{
    m_session->commit();
}

void NetconfAccess::discardChanges()
{
    m_session->discard();
}

std::string NetconfAccess::fetchSchema(const std::string_view module, const
        std::optional<std::string_view> revision, const
        std::optional<std::string_view> submodule, const
        std::optional<std::string_view> submoduleRevision)
{
    if (submodule) {
        return m_session->getSchema(*submodule, submoduleRevision);
    }
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
