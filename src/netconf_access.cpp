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
    : m_context(nullptr, libyang::ContextOptions::SetPrivParsed)
    , m_session(libnetconf::client::Session::connectPubkey(hostname, port, user, pubKey, privKey, m_context))
    , m_schema(std::make_shared<YangSchema>(m_context))
{
    checkNMDA();
}

NetconfAccess::NetconfAccess(const int source, const int sink)
    : m_context(nullptr, libyang::ContextOptions::SetPrivParsed)
    , m_session(libnetconf::client::Session::connectFd(source, sink, m_context))
    , m_schema(std::make_shared<YangSchema>(m_context))
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
    : m_context(nullptr, libyang::ContextOptions::SetPrivParsed)
    , m_session(libnetconf::client::Session::connectSocket(socketPath, m_context))
    , m_schema(std::make_shared<YangSchema>(m_context))
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
    auto nodes = m_schema->dataNodeFromPath(path, lyValue);
    doEditFromDataNode(*nodes.createdParent);
}

void NetconfAccess::createItem(const std::string& path)
{
    auto nodes = m_schema->dataNodeFromPath(path);
    doEditFromDataNode(*nodes.createdParent);
}

void NetconfAccess::deleteItem(const std::string& path)
{
    auto nodes = m_schema->dataNodeFromPath(path);

    // When deleting leafs, `nodes.newNode` is opaque, because the leaf does not have a value. We need to use
    // newAttrOpaqueJSON for opaque leafs.
    if (nodes.createdNode->isOpaque()) {
        nodes.createdNode->newAttrOpaqueJSON("ietf-netconf", "ietf-netconf:operation", "delete");
    } else {
        nodes.createdNode->newMeta(*m_schema->getYangModule("ietf-netconf"), "operation", "delete");
    }
    doEditFromDataNode(*nodes.createdParent);
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
    auto nodes = m_schema->dataNodeFromPath(source);
    auto sourceNode = *(nodes.createdNode->findPath(source.c_str()));
    auto yangModule = *m_schema->getYangModule("yang");
    sourceNode.newMeta(yangModule, "insert", toYangInsert(move).c_str());

    if (std::holds_alternative<yang::move::Relative>(move)) {
        auto relative = std::get<yang::move::Relative>(move);
        if (m_schema->nodeType(source) == yang::NodeTypes::LeafList) {
            sourceNode.newMeta(yangModule, "value", leafDataToString(relative.m_path.at(".")).c_str());
        } else {
            sourceNode.newMeta(yangModule, "key", instanceToString(relative.m_path, std::string{nodes.createdNode->schema().module().name()}).c_str());
        }
    }
    doEditFromDataNode(sourceNode);
}

void NetconfAccess::doEditFromDataNode(libyang::DataNode dataNode)
{
    auto data = dataNode.printStr(libyang::DataFormat::XML, libyang::PrintFlags::WithSiblings);
    if (m_serverHasNMDA) {
        m_session->editData(targetToDs_set(m_target), std::string{*data});
    } else {
        m_session->editConfig(NC_DATASTORE_CANDIDATE, NC_RPC_EDIT_DFLTOP_MERGE, NC_RPC_EDIT_TESTOPT_TESTSET, NC_RPC_EDIT_ERROPT_STOP, std::string{*data});
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
    auto data = inputNode.printStr(libyang::DataFormat::XML, libyang::PrintFlags::WithSiblings);

    auto output = m_session->rpc_or_action(std::string{*data});
    if (!output) {
        return {};
    }
    return rpcOutputToTree(*output);
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
    auto keys = m_session->libyangContext().findXPath(path.c_str()).front().asList().keys();
    auto nodes = m_session->libyangContext().newPath2(path.c_str(), nullptr, libyang::CreationOptions::Opaque);

    // Here we create a tree with "selection leafs" for all they keys of our wanted list. These leafs tell NETCONF, that
    // we only want the list's keys and not any other data.
    for (const auto& keyLeaf : keys) {
        // Selection leafs need to be inserted directly to the list using relative paths, that's why `newNode` is used
        // here.
        nodes.createdNode->newPath(keyLeaf.name().data(), nullptr, libyang::CreationOptions::Opaque);
    }

    // Have to use `newParent` in case our wanted list is a nested list. With `newNode` I would only send the inner
    // nested list and not the whole tree.
    auto instances = m_session->get(std::string{*nodes.createdParent->printStr(libyang::DataFormat::XML, libyang::PrintFlags::WithSiblings)});

    if (!instances) {
        return res;
    }

    for (const auto& instance : instances->findXPath(path.c_str())) {
        ListInstance instanceRes;

        for (const auto& keyLeaf : instance.child()->siblings()) {
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
    auto str = config->printStr(format == DataFormat::Xml ? libyang::DataFormat::XML : libyang::DataFormat::JSON, libyang::PrintFlags::WithSiblings);
    if (!str) {
        return "";
    }

    return std::string{*str};
}
