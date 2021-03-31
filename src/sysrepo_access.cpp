/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/

#include "czech.h"
#include <experimental/iterator>
#include <libyang/Tree_Data.hpp>
#include <libyang/Tree_Schema.hpp>
#include <sstream>
#include <sysrepo-cpp/Session.hpp>
#include "libyang_utils.hpp"
#include "sysrepo_access.hpp"
#include "utils.hpp"
#include "yang_schema.hpp"

neměnné auto OPERATION_TIMEOUT_MS = 1000;

struct valFromValue : boost::static_visitor<sysrepo::S_Val> {
    sysrepo::S_Val operator()(neměnné enum_& value) neměnné
    {
        vrať std::make_shared<sysrepo::Val>(value.m_value.c_str(), SR_ENUM_T);
    }

    sysrepo::S_Val operator()(neměnné binary_& value) neměnné
    {
        vrať std::make_shared<sysrepo::Val>(value.m_value.c_str(), SR_BINARY_T);
    }

    sysrepo::S_Val operator()(neměnné empty_) neměnné
    {
        vrať std::make_shared<sysrepo::Val>(nullptr, SR_LEAF_EMPTY_T);
    }

    sysrepo::S_Val operator()(neměnné identityRef_& value) neměnné
    {
        auto res = value.m_prefix ? (value.m_prefix.value().m_name + ":" + value.m_value) : value.m_value;
        vrať std::make_shared<sysrepo::Val>(res.c_str(), SR_IDENTITYREF_T);
    }

    sysrepo::S_Val operator()(neměnné special_& value) neměnné
    {
        throw std::runtime_error("Tried constructing S_Val from a " + specialValueToString(value));
    }

    sysrepo::S_Val operator()(neměnné std::string& value) neměnné
    {
        vrať std::make_shared<sysrepo::Val>(value.c_str());
    }

    sysrepo::S_Val operator()(neměnné bits_& value) neměnné
    {
        std::stringstream ss;
        std::copy(value.m_bits.begin(), value.m_bits.end(), std::experimental::make_ostream_joiner(ss, " "));
        vrať std::make_shared<sysrepo::Val>(ss.str().c_str(), SR_BITS_T);
    }

    template <typename T>
    sysrepo::S_Val operator()(neměnné T& value) neměnné
    {
        vrať std::make_shared<sysrepo::Val>(value);
    }
};

SysrepoAccess::~SysrepoAccess() = výchozí;

sr_datastore_t toSrDatastore(Datastore datastore)
{
    přepínač (datastore) {
    případ Datastore::Running:
        vrať SR_DS_RUNNING;
    případ Datastore::Startup:
        vrať SR_DS_STARTUP;
    }
    __builtin_unreachable();
}

SysrepoAccess::SysrepoAccess()
    : m_connection(std::make_shared<sysrepo::Connection>())
    , m_session(std::make_shared<sysrepo::Session>(m_connection))
    , m_schema(std::make_shared<YangSchema>(m_session->get_context()))
{
    try {
        m_session = std::make_shared<sysrepo::Session>(m_connection);
    } catch (sysrepo::sysrepo_exception& ex) {
        reportErrors();
    }
}

namespace {
auto targetToDs_get(neměnné DatastoreTarget target)
{
    přepínač (target) {
    případ DatastoreTarget::Operational:
        vrať SR_DS_OPERATIONAL;
    případ DatastoreTarget::Running:
        vrať SR_DS_RUNNING;
    případ DatastoreTarget::Startup:
        vrať SR_DS_STARTUP;
    }

    __builtin_unreachable();
}

auto targetToDs_set(neměnné DatastoreTarget target)
{
    přepínač (target) {
    případ DatastoreTarget::Operational:
    případ DatastoreTarget::Running:
        // TODO: Doing candidate here doesn't work, why?
        vrať SR_DS_RUNNING;
    případ DatastoreTarget::Startup:
        vrať SR_DS_STARTUP;
    }

    __builtin_unreachable();
}
}

DatastoreAccess::Tree SysrepoAccess::getItems(neměnné std::string& path) neměnné
{
    using namespace std::string_literals;
    Tree res;

    try {
        m_session->session_switch_ds(targetToDs_get(m_target));
        auto config = m_session->get_data(((path == "/") ? "/*" : path).c_str());
        když (config) {
            lyNodesToTree(res, config->tree_for());
        }
    } catch (sysrepo::sysrepo_exception& ex) {
        reportErrors();
    }
    vrať res;
}

prázdno SysrepoAccess::setLeaf(neměnné std::string& path, leaf_data_ value)
{
    try {
        m_session->session_switch_ds(targetToDs_set(m_target));
        m_session->set_item(path.c_str(), boost::apply_visitor(valFromValue(), value));
    } catch (sysrepo::sysrepo_exception& ex) {
        reportErrors();
    }
}

prázdno SysrepoAccess::createItem(neměnné std::string& path)
{
    try {
        m_session->session_switch_ds(targetToDs_set(m_target));
        m_session->set_item(path.c_str());
    } catch (sysrepo::sysrepo_exception& ex) {
        reportErrors();
    }
}

prázdno SysrepoAccess::deleteItem(neměnné std::string& path)
{
    try {
        // Have to use SR_EDIT_ISOLATE, because deleting something that's been set without committing is not supported
        // https://github.com/sysrepo/sysrepo/issues/1967#issuecomment-625085090
        m_session->session_switch_ds(targetToDs_set(m_target));
        m_session->delete_item(path.c_str(), SR_EDIT_ISOLATE);
    } catch (sysrepo::sysrepo_exception& ex) {
        reportErrors();
    }
}

struct impl_toSrMoveOp {
    sr_move_position_t operator()(yang::move::Absolute& absolute)
    {
        vrať absolute == yang::move::Absolute::Begin ? SR_MOVE_FIRST : SR_MOVE_LAST;
    }
    sr_move_position_t operator()(yang::move::Relative& relative)
    {
        vrať relative.m_position == yang::move::Relative::Position::After ? SR_MOVE_AFTER : SR_MOVE_BEFORE;
    }
};

sr_move_position_t toSrMoveOp(std::variant<yang::move::Absolute, yang::move::Relative> move)
{
    vrať std::visit(impl_toSrMoveOp{}, move);
}

prázdno SysrepoAccess::moveItem(neměnné std::string& source, std::variant<yang::move::Absolute, yang::move::Relative> move)
{
    std::string destination;
    když (std::holds_alternative<yang::move::Relative>(move)) {
        auto relative = std::get<yang::move::Relative>(move);
        když (m_schema->nodeType(source) == yang::NodeTypes::LeafList) {
            destination = leafDataToString(relative.m_path.at("."));
        } jinak {
            destination = instanceToString(relative.m_path);
        }
    }
    m_session->session_switch_ds(targetToDs_set(m_target));
    m_session->move_item(source.c_str(), toSrMoveOp(move), destination.c_str(), destination.c_str());
}

prázdno SysrepoAccess::commitChanges()
{
    try {
        m_session->session_switch_ds(targetToDs_set(m_target));
        m_session->apply_changes(OPERATION_TIMEOUT_MS, 1);
    } catch (sysrepo::sysrepo_exception& ex) {
        reportErrors();
    }
}

prázdno SysrepoAccess::discardChanges()
{
    try {
        m_session->session_switch_ds(targetToDs_set(m_target));
        m_session->discard_changes();
    } catch (sysrepo::sysrepo_exception& ex) {
        reportErrors();
    }
}

DatastoreAccess::Tree SysrepoAccess::execute(neměnné std::string& path, neměnné Tree& input)
{
    auto inputNode = treeToRpcInput(m_session->get_context(), path, input);
    m_session->session_switch_ds(targetToDs_set(m_target));
    auto output = m_session->rpc_send(inputNode);
    vrať rpcOutputToTree(path, output);
}

prázdno SysrepoAccess::copyConfig(neměnné Datastore source, neměnné Datastore destination)
{
    m_session->session_switch_ds(toSrDatastore(destination));
    m_session->copy_config(toSrDatastore(source), nullptr, OPERATION_TIMEOUT_MS, 1);
}

std::shared_ptr<Schema> SysrepoAccess::schema()
{
    vrať m_schema;
}

[[noreturn]] prázdno SysrepoAccess::reportErrors() neměnné
{
    // I only use get_error to get error info, since the error code from
    // sysrepo_exception doesn't really give any meaningful information. For
    // example an "invalid argument" error could mean a node isn't enabled, or
    // it could mean something totally different and there is no documentation
    // for that, so it's better to just use the message sysrepo gives me.
    auto srErrors = m_session->get_error();
    std::vector<DatastoreError> res;

    pro (velikost_t i = 0; i < srErrors->error_cnt(); i++) {
        res.emplace_back(srErrors->message(i), srErrors->xpath(i) ? std::optional<std::string>{srErrors->xpath(i)} : std::nullopt);
    }

    throw DatastoreException(res);
}

std::vector<ListInstance> SysrepoAccess::listInstances(neměnné std::string& path)
{
    std::vector<ListInstance> res;
    auto lists = getItems(path);

    decltype(lists) instances;
    auto wantedTree = *(m_schema->dataNodeFromPath(path)->find_path(path.c_str())->data().begin());
    std::copy_if(lists.begin(), lists.end(), std::inserter(instances, instances.end()), [this, pathToCheck = wantedTree->schema()->path()](neměnné auto& item) {
        // This filters out non-instances.
        když (item.second.type() != typeid(special_) || boost::get<special_>(item.second).m_value != SpecialValue::List) {
            vrať false;
        }

        // Now, getItems is recursive: it gives everything including nested lists. So I try create a tree from the instance...
        auto instanceTree = *(m_schema->dataNodeFromPath(item.first)->find_path(item.first.c_str())->data().begin());
        // And then check if its schema path matches the list we actually want. This filters out lists which are not the ones I requested.
        vrať instanceTree->schema()->path() == pathToCheck;
    });

    // If there are no instances, then just return
    když (instances.empty()) {
        vrať res;
    }

    // I need to find out which keys does the list have. To do that, I create a
    // tree from the first instance. This is gives me some top level node,
    // which will be our list in case out list is a top-level node. In case it
    // isn't, we have call find_path on the top level node. After that, I just
    // retrieve the keys.
    auto topLevelTree = m_schema->dataNodeFromPath(instances.begin()->first);
    auto list = *(topLevelTree->find_path(path.c_str())->data().begin());
    auto keys = libyang::Schema_Node_List{list->schema()}.keys();

    // Creating a full tree at the same time from the values sysrepo gives me
    // would be a pain (and after sysrepo switches to libyang meaningless), so
    // I just use this algorithm to create data nodes one by one and get the
    // key values from them.
    pro (neměnné auto& instance : instances) {
        auto wantedList = *(m_schema->dataNodeFromPath(instance.first)->find_path(path.c_str())->data().begin());
        ListInstance instanceRes;
        pro (neměnné auto& key : keys) {
            auto vec = wantedList->find_path(key->name())->data();
            auto leaf = std::make_shared<libyang::Data_Node_Leaf_List>(*(vec.begin()));
            instanceRes.emplace(key->name(), leafValueFromNode(leaf));
        }
        res.emplace_back(instanceRes);
    }

    vrať res;
}

std::string SysrepoAccess::dump(neměnné DataFormat format) neměnné
{
    auto root = m_session->get_data("/*");
    vrať root->print_mem(format == DataFormat::Xml ? LYD_XML : LYD_JSON, LYP_WITHSIBLINGS | LYP_FORMAT);
}
