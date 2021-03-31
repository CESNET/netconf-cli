/*
 * Copyright (C) 2019 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
*/

#include "czech.h"
#include <libyang/Libyang.hpp>
#include <libyang/Tree_Data.hpp>
#include "libyang_utils.hpp"
#include "netconf-client.hpp"
#include "netconf_access.hpp"
#include "utils.hpp"
#include "yang_schema.hpp"


NetconfAccess::~NetconfAccess() = výchozí;
namespace {
auto targetToDs_get(neměnné DatastoreTarget target)
{
    přepínač (target) {
    případ DatastoreTarget::Operational:
        vrať libnetconf::NmdaDatastore::Operational;
    případ DatastoreTarget::Running:
        vrať libnetconf::NmdaDatastore::Running;
    případ DatastoreTarget::Startup:
        vrať libnetconf::NmdaDatastore::Startup;
    }

    __builtin_unreachable();
}

auto targetToDs_set(neměnné DatastoreTarget target)
{
    přepínač (target) {
    případ DatastoreTarget::Operational:
    případ DatastoreTarget::Running:
        vrať libnetconf::NmdaDatastore::Candidate;
    případ DatastoreTarget::Startup:
        vrať libnetconf::NmdaDatastore::Startup;
    }

    __builtin_unreachable();
}
}
DatastoreAccess::Tree NetconfAccess::getItems(neměnné std::string& path) neměnné
{
    Tree res;
    auto config = m_session->getData(targetToDs_get(m_target), (path != "/") ? std::optional{path} : std::nullopt);

    když (config) {
        lyNodesToTree(res, config->tree_for());
    }
    vrať res;
}

NetconfAccess::NetconfAccess(neměnné std::string& hostname, nčíslo16_t port, neměnné std::string& user, neměnné std::string& pubKey, neměnné std::string& privKey)
    : m_session(libnetconf::client::Session::connectPubkey(hostname, port, user, pubKey, privKey))
    , m_schema(std::make_shared<YangSchema>(m_session->libyangContext()))
{
}

NetconfAccess::NetconfAccess(neměnné číslo source, neměnné číslo sink)
    : m_session(libnetconf::client::Session::connectFd(source, sink))
    , m_schema(std::make_shared<YangSchema>(m_session->libyangContext()))
{
}

NetconfAccess::NetconfAccess(std::unique_ptr<libnetconf::client::Session>&& session)
    : m_session(std::move(session))
    , m_schema(std::make_shared<YangSchema>(m_session->libyangContext()))
{
}

NetconfAccess::NetconfAccess(neměnné std::string& socketPath)
    : m_session(libnetconf::client::Session::connectSocket(socketPath))
    , m_schema(std::make_shared<YangSchema>(m_session->libyangContext()))
{
}

prázdno NetconfAccess::setNcLogLevel(NC_VERB_LEVEL level)
{
    libnetconf::client::setLogLevel(level);
}

prázdno NetconfAccess::setNcLogCallback(neměnné LogCb& callback)
{
    libnetconf::client::setLogCallback(callback);
}

prázdno NetconfAccess::setLeaf(neměnné std::string& path, leaf_data_ value)
{
    auto lyValue = value.type() == typeid(empty_) ? std::nullopt : std::optional(leafDataToString(value));
    auto node = m_schema->dataNodeFromPath(path, lyValue);
    doEditFromDataNode(node);
}

prázdno NetconfAccess::createItem(neměnné std::string& path)
{
    auto node = m_schema->dataNodeFromPath(path);
    doEditFromDataNode(node);
}

prázdno NetconfAccess::deleteItem(neměnné std::string& path)
{
    auto node = m_schema->dataNodeFromPath(path);
    auto container = *(node->find_path(path.c_str())->data().begin());
    container->insert_attr(m_schema->getYangModule("ietf-netconf"), "operation", "delete");
    doEditFromDataNode(node);
}

struct impl_toYangInsert {
    std::string operator()(yang::move::Absolute& absolute)
    {
        vrať absolute == yang::move::Absolute::Begin ? "first" : "last";
    }
    std::string operator()(yang::move::Relative& relative)
    {
        vrať relative.m_position == yang::move::Relative::Position::After ? "after" : "before";
    }
};

std::string toYangInsert(std::variant<yang::move::Absolute, yang::move::Relative> move)
{
    vrať std::visit(impl_toYangInsert{}, move);
}

prázdno NetconfAccess::moveItem(neměnné std::string& source, std::variant<yang::move::Absolute, yang::move::Relative> move)
{
    auto node = m_schema->dataNodeFromPath(source);
    auto sourceNode = *(node->find_path(source.c_str())->data().begin());
    auto yangModule = m_schema->getYangModule("yang");
    sourceNode->insert_attr(yangModule, "insert", toYangInsert(move).c_str());

    když (std::holds_alternative<yang::move::Relative>(move)) {
        auto relative = std::get<yang::move::Relative>(move);
        když (m_schema->nodeType(source) == yang::NodeTypes::LeafList) {
            sourceNode->insert_attr(yangModule, "value", leafDataToString(relative.m_path.at(".")).c_str());
        } jinak {
            sourceNode->insert_attr(yangModule, "key", instanceToString(relative.m_path, node->node_module()->name()).c_str());
        }
    }
    doEditFromDataNode(sourceNode);
}

prázdno NetconfAccess::doEditFromDataNode(std::shared_ptr<libyang::Data_Node> dataNode)
{
    auto data = dataNode->print_mem(LYD_XML, 0);
    m_session->editData(targetToDs_set(m_target), data);
}

prázdno NetconfAccess::commitChanges()
{
    m_session->commit();
}

prázdno NetconfAccess::discardChanges()
{
    m_session->discard();
}

DatastoreAccess::Tree NetconfAccess::execute(neměnné std::string& path, neměnné Tree& input)
{
    auto inputNode = treeToRpcInput(m_session->libyangContext(), path, input);
    auto data = inputNode->print_mem(LYD_XML, 0);

    auto output = m_session->rpc_or_action(data);
    vrať rpcOutputToTree(path, output);
}

NC_DATASTORE toNcDatastore(Datastore datastore)
{
    přepínač (datastore) {
    případ Datastore::Running:
        vrať NC_DATASTORE_RUNNING;
    případ Datastore::Startup:
        vrať NC_DATASTORE_STARTUP;
    }
    __builtin_unreachable();
}

prázdno NetconfAccess::copyConfig(neměnné Datastore source, neměnné Datastore destination)
{
    m_session->copyConfig(toNcDatastore(source), toNcDatastore(destination));
}

std::shared_ptr<Schema> NetconfAccess::schema()
{
    vrať m_schema;
}

std::vector<ListInstance> NetconfAccess::listInstances(neměnné std::string& path)
{
    std::vector<ListInstance> res;
    auto list = m_schema->dataNodeFromPath(path);

    // This inserts selection nodes - I only want keys not other data
    // To get the keys, I have to call find_path here - otherwise I would get keys of a top-level node (which might not even be a list)
    auto keys = libyang::Schema_Node_List{(*(list->find_path(path.c_str())->data().begin()))->schema()}.keys();
    pro (neměnné auto& keyLeaf : keys) {
        // Have to call find_path here - otherwise I'll have the list, not the leaf inside it
        auto selectionLeaf = *(m_schema->dataNodeFromPath(keyLeaf->path())->find_path(keyLeaf->path().c_str())->data().begin());
        auto actualList = *(list->find_path(path.c_str())->data().begin());
        actualList->insert(selectionLeaf);
    }

    auto instances = m_session->get(list->print_mem(LYD_XML, 0));

    když (!instances) {
        vrať res;
    }

    pro (neměnné auto& instance : instances->find_path(path.c_str())->data()) {
        ListInstance instanceRes;

        // I take the first child here, because the first element (the parent of the child()) will be the list
        pro (neměnné auto& keyLeaf : instance->child()->tree_for()) {
            // FIXME: even though we specified we only want the key leafs, Netopeer disregards that and sends more data,
            // even lists and other stuff. We only want keys, so filter out non-leafs and non-keys
            // https://github.com/CESNET/netopeer2/issues/825
            když (keyLeaf->schema()->nodetype() != LYS_LEAF) {
                pokračuj;
            }
            když (!std::make_shared<libyang::Schema_Node_Leaf>(keyLeaf->schema())->is_key()) {
                pokračuj;
            }

            auto leafData = std::make_shared<libyang::Data_Node_Leaf_List>(keyLeaf);
            instanceRes.insert({leafData->schema()->name(), leafValueFromNode(leafData)});
        }
        res.emplace_back(instanceRes);
    }

    vrať res;
}

std::string NetconfAccess::dump(neměnné DataFormat format) neměnné
{
    auto config = m_session->get();
    když (!config) {
        vrať "";
    }
    vrať config->print_mem(format == DataFormat::Xml ? LYD_XML : LYD_JSON, LYP_WITHSIBLINGS | LYP_FORMAT);
}
