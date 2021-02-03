/*
 * Copyright (C) 2019 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
*/

#include <libyang/Libyang.hpp>
#include <libyang/Tree_Data.hpp>
#include "libyang_utils.hpp"
#include "netconf-client.hpp"
#include "netconf_access.hpp"
#include "utils.hpp"
#include "yang_schema.hpp"


NetconfAccess::~NetconfAccess() = default;

DatastoreAccess::Tree NetconfAccess::getItems(const std::string& path) const
{
    Tree res;
    auto config = m_session->getData(libnetconf::NmdaDatastore::Operational, (path != "/") ? std::optional{path} : std::nullopt);

    if (config) {
        lyNodesToTree(res, config->tree_for());
    }
    return res;
}

NetconfAccess::NetconfAccess(const std::string& hostname, uint16_t port, const std::string& user, const std::string& pubKey, const std::string& privKey)
    : m_session(libnetconf::client::Session::connectPubkey(hostname, port, user, pubKey, privKey))
    , m_schema(std::make_shared<YangSchema>(m_session->libyangContext()))
{
}

NetconfAccess::NetconfAccess(const int source, const int sink)
    : m_session(libnetconf::client::Session::connectFd(source, sink))
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

void NetconfAccess::setNcLogLevel(NC_VERB_LEVEL level)
{
    libnetconf::client::setLogLevel(level);
}

void NetconfAccess::setNcLogCallback(const LogCb& callback)
{
    libnetconf::client::setLogCallback(callback);
}

void NetconfAccess::setLeaf(const std::string& path, leaf_data_ value)
{
    auto lyValue = value.type() == typeid(empty_) ? std::nullopt : std::optional(leafDataToString(value));
    auto node = m_schema->dataNodeFromPath(path, lyValue);
    doEditFromDataNode(node);
}

void NetconfAccess::createItem(const std::string& path)
{
    auto node = m_schema->dataNodeFromPath(path);
    doEditFromDataNode(node);
}

void NetconfAccess::deleteItem(const std::string& path)
{
    auto node = m_schema->dataNodeFromPath(path);
    auto container = *(node->find_path(path.c_str())->data().begin());
    container->insert_attr(m_schema->getYangModule("ietf-netconf"), "operation", "delete");
    doEditFromDataNode(node);
}

struct impl_toYangInsert {
    std::string operator()(yang::move::Absolute& absolute)
    {
        return absolute == yang::move::Absolute::Begin ? "first" : "last";
    }
    std::string operator()(yang::move::Relative& relative)
    {
        return relative.m_position == yang::move::Relative::Position::After ? "after" : "before";
    }
};

std::string toYangInsert(std::variant<yang::move::Absolute, yang::move::Relative> move)
{
    return std::visit(impl_toYangInsert{}, move);
}

void NetconfAccess::moveItem(const std::string& source, std::variant<yang::move::Absolute, yang::move::Relative> move)
{
    auto node = m_schema->dataNodeFromPath(source);
    auto sourceNode = *(node->find_path(source.c_str())->data().begin());
    auto yangModule = m_schema->getYangModule("yang");
    sourceNode->insert_attr(yangModule, "insert", toYangInsert(move).c_str());

    if (std::holds_alternative<yang::move::Relative>(move)) {
        auto relative = std::get<yang::move::Relative>(move);
        if (m_schema->nodeType(source) == yang::NodeTypes::LeafList) {
            sourceNode->insert_attr(yangModule, "value", leafDataToString(relative.m_path.at(".")).c_str());
        } else {
            sourceNode->insert_attr(yangModule, "key", instanceToString(relative.m_path, node->node_module()->name()).c_str());
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

DatastoreAccess::Tree NetconfAccess::execute(const std::string& path, const Tree& input)
{
    auto root = m_schema->dataNodeFromPath(path);
    for (const auto& [k, v] : input) {
        root->new_path(m_session->libyangContext(), k.c_str(), leafDataToString(v).c_str(), LYD_ANYDATA_CONSTSTRING, LYD_PATH_OPT_UPDATE);
    }
    auto data = root->print_mem(LYD_XML, 0);

    Tree res;
    auto output = m_session->rpc_or_action(data);
    if (output) {
        // If there's output, it will be a top-level node. In case of action, the output can be nested so we need to use
        // find_path to get to the actual output. Also, our `path` is fully prefixed, but the output paths aren't. So
        // we use outputNode->path() to get the unprefixed path.

        auto outputNode = output->find_path(path.c_str())->data().front();
        lyNodesToTree(res, {outputNode}, joinPaths(outputNode->path(), "/"));
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
            res.emplace_back(leaf.value_str());
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
            // FIXME: even though we specified we only want the key leafs, Netopeer disregards that and sends more data,
            // even lists and other stuff. We only want keys, so filter out non-leafs and non-keys
            // https://github.com/CESNET/netopeer2/issues/825
            if (keyLeaf->schema()->nodetype() != LYS_LEAF) {
                continue;
            }
            if (!std::make_shared<libyang::Schema_Node_Leaf>(keyLeaf->schema())->is_key()) {
                continue;
            }

            auto leafData = std::make_shared<libyang::Data_Node_Leaf_List>(keyLeaf);
            instanceRes.insert({leafData->schema()->name(), leafValueFromNode(leafData)});
        }
        res.emplace_back(instanceRes);
    }

    return res;
}

std::string NetconfAccess::dump(const DataFormat format) const
{
    auto config = m_session->get();
    if (!config) {
        return "";
    }
    return config->print_mem(format == DataFormat::Xml ? LYD_XML : LYD_JSON, LYP_WITHSIBLINGS | LYP_FORMAT);
}
