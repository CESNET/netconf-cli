/*
 * Copyright (C) 2019 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
*/

#include <libyang/Libyang.hpp>
#include <libyang/Tree_Data.hpp>
#include "libyang_utils.hpp"
#include "netconf-client.h"
#include "netconf_access.hpp"
#include "utils.hpp"
#include "yang_schema.hpp"

namespace {

// This is very similar to the fillMap lambda in SysrepoAccess, however,
// Sysrepo returns a weird array-like structure, while libnetconf
// returns libyang::Data_Node
void fillMap(DatastoreAccess::Tree& res, const std::vector<std::shared_ptr<libyang::Data_Node>> items, std::optional<std::string> ignoredXPathPrefix = std::nullopt)
{
    auto stripXPathPrefix = [&ignoredXPathPrefix] (auto path) {
        return ignoredXPathPrefix ? path.substr(ignoredXPathPrefix->size()) : path;
    };

    for (const auto& it : items) {
        if (!it)
            continue;
        if (it->schema()->nodetype() == LYS_CONTAINER) {
            if (libyang::Schema_Node_Container{it->schema()}.presence()) {
                // The fact that the container is included in the data tree
                // means that it is present and I don't need to check any
                // value.
                res.emplace(stripXPathPrefix(it->path()), special_{SpecialValue::PresenceContainer});
            }
        }
        if (it->schema()->nodetype() == LYS_LIST) {
            res.emplace(stripXPathPrefix(it->path()), special_{SpecialValue::List});
        }
        if (it->schema()->nodetype() == LYS_LEAF) {
            libyang::Data_Node_Leaf_List leaf(it);
            res.emplace(stripXPathPrefix(it->path()), leafValueFromValue(leaf.value(), leaf.leaf_type()->base()));
        }
    }
}
}


NetconfAccess::~NetconfAccess() = default;

DatastoreAccess::Tree NetconfAccess::getItems(const std::string& path)
{
    Tree res;
    auto config = m_session->get((path != "/") ? std::optional{path} : std::nullopt);

    if (config) {
        for (auto it : config->tree_for()) {
            fillMap(res, it->tree_dfs());
        }
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

NetconfAccess::NetconfAccess(std::unique_ptr<libnetconf::client::Session>&& session)
    : m_session(std::move(session))
    , m_schema(new YangSchema())
{
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

DatastoreAccess::Tree NetconfAccess::executeRpc(const std::string& path, const Tree& input)
{
    auto root = m_schema->dataNodeFromPath(path);
    for (const auto& [k, v] : input) {
        auto node = m_schema->dataNodeFromPath(joinPaths(path, k), leafDataToString(v));
        root->merge(node, 0);
    }
    auto data = root->print_mem(LYD_XML, 0);

    Tree res;
    auto output = m_session->rpc(data);
    if (output) {
        for (auto it : output->tree_for()) {
            fillMap(res, it->tree_dfs(), joinPaths(path, "/"));
        }
    }
    return res;
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
