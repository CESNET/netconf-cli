/*
 * Copyright (C) 2019 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
*/

#include "libyang_utils.hpp"
#include "netconf-client.hpp"
#include "netconf_access.hpp"
#include "utils.hpp"
#include "yang_schema.hpp"


NetconfAccess::~NetconfAccess() = default;
namespace {
auto targetToDs_get(const DatastoreTarget target)
{
    switch (target) {
    case DatastoreTarget::Operational:
        return libnetconf::NmdaDatastore::Operational;
    case DatastoreTarget::Running:
        return libnetconf::NmdaDatastore::Running;
    case DatastoreTarget::Startup:
        return libnetconf::NmdaDatastore::Startup;
    }

    __builtin_unreachable();
}

auto targetToDs_set(const DatastoreTarget target)
{
    switch (target) {
    case DatastoreTarget::Operational:
    case DatastoreTarget::Running:
        return libnetconf::NmdaDatastore::Candidate;
    case DatastoreTarget::Startup:
        return libnetconf::NmdaDatastore::Startup;
    }

    __builtin_unreachable();
}
}
DatastoreAccess::Tree NetconfAccess::getItems(const std::string& path) const
{
    Tree res;
    auto config = [this, &path] {
        if (m_serverHasNMDA) {
            return m_session->getData(targetToDs_get(m_target), (path != "/") ? std::optional{path} : std::nullopt);
        }

        return m_session->get((path != "/") ? std::optional{path} : std::nullopt);
    }();

    if (config) {
        lyNodesToTree(res, config->siblings());
    }
    return res;
}

NetconfAccess::NetconfAccess(const std::string& hostname, uint16_t port, const std::string& user, const std::string& pubKey, const std::string& privKey)
    : m_session(libnetconf::client::Session::connectPubkey(hostname, port, user, pubKey, privKey))
    , m_schema(std::make_shared<YangSchema>(m_session->libyangContext()))
{
    checkNMDA();
}

NetconfAccess::NetconfAccess(const int source, const int sink)
    : m_session(libnetconf::client::Session::connectFd(source, sink))
    , m_schema(std::make_shared<YangSchema>(m_session->libyangContext()))
{
    checkNMDA();
}

NetconfAccess::NetconfAccess(std::unique_ptr<libnetconf::client::Session>&& session)
    : m_session(std::move(session))
    , m_schema(std::make_shared<YangSchema>(m_session->libyangContext()))
{
    checkNMDA();
}

NetconfAccess::NetconfAccess(const std::string& socketPath)
    : m_session(libnetconf::client::Session::connectSocket(socketPath))
    , m_schema(std::make_shared<YangSchema>(m_session->libyangContext()))
{
    checkNMDA();
}

void NetconfAccess::checkNMDA()
{
    auto nmdaMod = m_schema->getYangModule("ietf-netconf-nmda");
    m_serverHasNMDA = nmdaMod && nmdaMod->implemented();
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
    // FIXME: this does not work leafs... because they NEED a value
    // https://github.com/CESNET/libyang/issues/1726
    auto node = m_schema->dataNodeFromPath(path);
    auto container = *(node.findPath(path.c_str()));

    container.newMeta(*m_schema->getYangModule("ietf-netconf"), "operation", "delete");
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
    auto sourceNode = *(node.findPath(source.c_str()));
    auto yangModule = *m_schema->getYangModule("yang");
    sourceNode.newMeta(yangModule, "insert", toYangInsert(move).c_str());

    if (std::holds_alternative<yang::move::Relative>(move)) {
        auto relative = std::get<yang::move::Relative>(move);
        if (m_schema->nodeType(source) == yang::NodeTypes::LeafList) {
            sourceNode.newMeta(yangModule, "value", leafDataToString(relative.m_path.at(".")).c_str());
        } else {
            sourceNode.newMeta(yangModule, "key", instanceToString(relative.m_path, std::string{node.schema().module().name()}).c_str());
        }
    }
    doEditFromDataNode(sourceNode);
}

void NetconfAccess::doEditFromDataNode(libyang::DataNode dataNode)
{
    // auto data = dataNode.print_mem(LYD_XML, 0);
    auto data = dataNode.printStr(libyang::DataFormat::XML, libyang::PrintFlags::WithSiblings); // FIXME: ... is this correct?
    if (m_serverHasNMDA) {
        m_session->editData(targetToDs_set(m_target), std::string{data});
    } else {
        m_session->editConfig(NC_DATASTORE_CANDIDATE, NC_RPC_EDIT_DFLTOP_MERGE, NC_RPC_EDIT_TESTOPT_TESTSET, NC_RPC_EDIT_ERROPT_STOP, std::string{data});
    }
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
    auto inputNode = treeToRpcInput(m_session->libyangContext(), path, input);
    // auto data = inputNode->print_mem(LYD_XML, 0);
    auto data = inputNode.printStr(libyang::DataFormat::XML, libyang::PrintFlags::WithSiblings); // FIXME: is this correct?

    auto output = m_session->rpc_or_action(std::string{data});
    if (!output) {
        return {};
    }
    return rpcOutputToTree(path, *output);
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
    // auto keys = libyang::Schema_Node_List{(*(list->find_path(path.c_str())->data().begin()))->schema()}.keys();
    auto keys = list.findPath(path.c_str())->schema().asList().keys();
    for (const auto& keyLeaf : keys) {
        // Have to call find_path here - otherwise I'll have the list, not the leaf inside it
        // auto selectionLeaf = *(m_schema->dataNodeFromPath(keyLeaf->path())->find_path(keyLeaf->path().c_str())->data().begin());
        // FIXME: get rid of this .get().get()
        auto selectionLeaf = m_schema->dataNodeFromPath(keyLeaf.path().get().get()).findPath(keyLeaf.path().get().get());
        auto actualList = *list.findPath(path.c_str());
        // actualList->insert(selectionLeaf);
        // FIXME: implement insert_child in libyang-cpp
    }

    auto instances = m_session->get(std::string{list.printStr(libyang::DataFormat::XML, libyang::PrintFlags::WithSiblings)});

    if (!instances) {
        return res;
    }

    //TODO: this algorithm relies on find_path taking a list path with no keys, need to implement that in libyang-cpp
    for (const auto& instance : instances->findXPath(path.c_str())) {
        ListInstance instanceRes;

        // I take the first child here, because the first element (the parent of the child()) will be the list
        for (const auto& keyLeaf : instance.childrenDfs().begin()->childrenDfs()) {// FIXME will tis work?
            // FIXME: even though we specified we only want the key leafs, Netopeer disregards that and sends more data,
            // even lists and other stuff. We only want keys, so filter out non-leafs and non-keys
            // https://github.com/CESNET/netopeer2/issues/825
            if (keyLeaf.schema().nodeType() != libyang::NodeType::Leaf) {
                continue;
            }
            if (!keyLeaf.schema().asLeaf().isKey()) {
                continue;
            }

            auto leafData = keyLeaf.asTerm();
            instanceRes.insert({std::string{leafData.schema().name()}, leafValueFromNode(leafData)});
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
    return std::string{config->printStr(format == DataFormat::Xml ? libyang::DataFormat::XML : libyang::DataFormat::JSON, libyang::PrintFlags::WithSiblings)};
}
