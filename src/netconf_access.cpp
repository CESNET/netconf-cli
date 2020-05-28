/*
 * Copyright (C) 2019 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
*/

#include <boost/algorithm/string/predicate.hpp>
#include <libyang/Libyang.hpp>
#include <libyang/Tree_Data.hpp>
#include "libyang_utils.hpp"
#include "netconf-client.hpp"
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
                res.emplace_back(stripXPathPrefix(it->path()), special_{SpecialValue::PresenceContainer});
            }
        }
        if (it->schema()->nodetype() == LYS_LIST) {
            res.push_back({stripXPathPrefix(it->path()), special_{SpecialValue::List}});
        }
        if (it->schema()->nodetype() == LYS_LEAF || it->schema()->nodetype() == LYS_LEAFLIST) {
            libyang::Data_Node_Leaf_List leaf(it);
            auto value = leafValueFromValue(leaf.value(), leaf.leaf_type()->base());
            res.emplace_back(stripXPathPrefix(it->path()), value);
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
        auto siblings = config->tree_for();
        for (auto it = siblings.begin(); it < siblings.end(); it++) {
            if ((*it)->schema()->nodetype() == LYS_LEAFLIST) {
                auto leafListPath = stripLeafListValueFromPath((*it)->path());
                res.emplace_back(leafListPath, special_{SpecialValue::LeafList});
                while (it != siblings.end() && boost::starts_with((*it)->path(), leafListPath)) {
                    fillMap(res, (*it)->tree_dfs());
                    it++;
                }
            } else {
                fillMap(res, (*it)->tree_dfs());
            }
        }
    }
    return res;
}

NetconfAccess::NetconfAccess(const std::string& hostname, uint16_t port, const std::string& user, const std::string& pubKey, const std::string& privKey)
    : m_session(libnetconf::client::Session::connectPubkey(hostname, port, user, pubKey, privKey))
    , m_schema(std::make_shared<YangSchema>(m_session->libyangContext()))
{
}

NetconfAccess::NetconfAccess(std::unique_ptr<libnetconf::client::Session>&& session)
    : m_session(std::move(session))
    , m_schema(std::make_shared<YangSchema>(m_session->libyangContext()))
{
}

NetconfAccess::NetconfAccess(const std::string& socketPath)
    : m_session(libnetconf::client::Session::connectSocket(socketPath))
    , m_schema(std::make_shared<YangSchema>(m_session->libyangContext()))
{
}

void NetconfAccess::setLeaf(const std::string& path, leaf_data_ value)
{
    auto lyValue = value.type() == typeid(empty_) ? std::nullopt : std::optional(leafDataToString(value));
    auto node = m_schema->dataNodeFromPath(path, lyValue);
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
    auto container = *(node->find_path(path.c_str())->data().begin());
    container->insert_attr(m_schema->getYangModule("ietf-netconf"), "operation", "delete");
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
    auto list = *(node->find_path(path.c_str())->data().begin());
    list->insert_attr(m_schema->getYangModule("ietf-netconf"), "operation", "delete");
    doEditFromDataNode(node);
}

void NetconfAccess::createLeafListInstance(const std::string& path)
{
    auto node = m_schema->dataNodeFromPath(path);
    doEditFromDataNode(node);
}
void NetconfAccess::deleteLeafListInstance(const std::string& path)
{
    auto node = m_schema->dataNodeFromPath(path);
    auto list = *(node->find_path(path.c_str())->data().begin());
    list->insert_attr(m_schema->getYangModule("ietf-netconf"), "operation", "delete");
    doEditFromDataNode(node);
}

void NetconfAccess::moveItem(const dataPath_& source, std::variant<Absolute, Relative> move)
{
    auto pathStr = pathToDataString(source, Prefixes::WhenNeeded);
    auto node = m_schema->dataNodeFromPath(pathStr);
    auto sourceNode = *(node->find_path(pathStr.c_str())->data().begin());
    if (std::holds_alternative<Absolute>(move)) {
        if (std::get<Absolute>(move) == Absolute::Begin) {
            sourceNode->insert_attr(m_schema->getYangModule("yang"), "insert", "first");
        } else {
            sourceNode->insert_attr(m_schema->getYangModule("yang"), "insert", "last");
        }
    }

    doEditFromDataNode(sourceNode);
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

NC_DATASTORE toNcDatastore(Datastore datastore)
{
    switch (datastore) {
    case Datastore::Running:
        return NC_DATASTORE_RUNNING;
    case Datastore::Startup:
        return NC_DATASTORE_STARTUP;
    }
    __builtin_unreachable();
}

void NetconfAccess::copyConfig(const Datastore source, const Datastore destination)
{
    m_session->copyConfig(toNcDatastore(source), toNcDatastore(destination));
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

std::vector<ListInstance> NetconfAccess::listInstances(const std::string& path)
{
    std::vector<ListInstance> res;
    auto list = m_schema->dataNodeFromPath(path);

    // This inserts selection nodes - I only want keys not other data
    // To get the keys, I have to call find_path here - otherwise I would get keys of a top-level node (which might not even be a list)
    auto keys = libyang::Schema_Node_List{(*(list->find_path(path.c_str())->data().begin()))->schema()}.keys();
    for (const auto& keyLeaf : keys) {
        // Have to call find_path here - otherwise I'll have the list, not the leaf inside it
        auto selectionLeaf = *(m_schema->dataNodeFromPath(keyLeaf->path())->find_path(keyLeaf->path().c_str())->data().begin());
        auto actualList = *(list->find_path(path.c_str())->data().begin());
        actualList->insert(selectionLeaf);
    }

    auto instances = m_session->get(list->print_mem(LYD_XML, 0));

    if (!instances) {
        return res;
    }

    for (const auto& instance : instances->find_path(path.c_str())->data()) {
        ListInstance instanceRes;

        // I take the first child here, because the first element (the parent of the child()) will be the list
        for (const auto& keyLeaf : instance->child()->tree_for()) {
            auto leafData = libyang::Data_Node_Leaf_List{keyLeaf};
            auto leafSchema = libyang::Schema_Node_Leaf{leafData.schema()};
            instanceRes.insert({ leafSchema.name(), leafValueFromValue(leafData.value(), leafSchema.type()->base())});
        }
        res.push_back(instanceRes);
    }

    return res;
}
