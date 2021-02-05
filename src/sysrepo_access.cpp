/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/

#include <experimental/iterator>
#include <libyang/Tree_Data.hpp>
#include <libyang/Tree_Schema.hpp>
#include <sstream>
#include <sysrepo-cpp/Session.hpp>
#include "libyang_utils.hpp"
#include "sysrepo_access.hpp"
#include "utils.hpp"
#include "yang_schema.hpp"

const auto OPERATION_TIMEOUT_MS = 1000;

struct valFromValue : boost::static_visitor<sysrepo::S_Val> {
    sysrepo::S_Val operator()(const enum_& value) const
    {
        return std::make_shared<sysrepo::Val>(value.m_value.c_str(), SR_ENUM_T);
    }

    sysrepo::S_Val operator()(const binary_& value) const
    {
        return std::make_shared<sysrepo::Val>(value.m_value.c_str(), SR_BINARY_T);
    }

    sysrepo::S_Val operator()(const empty_) const
    {
        return std::make_shared<sysrepo::Val>(nullptr, SR_LEAF_EMPTY_T);
    }

    sysrepo::S_Val operator()(const identityRef_& value) const
    {
        auto res = value.m_prefix ? (value.m_prefix.value().m_name + ":" + value.m_value) : value.m_value;
        return std::make_shared<sysrepo::Val>(res.c_str(), SR_IDENTITYREF_T);
    }

    sysrepo::S_Val operator()(const special_& value) const
    {
        throw std::runtime_error("Tried constructing S_Val from a " + specialValueToString(value));
    }

    sysrepo::S_Val operator()(const std::string& value) const
    {
        return std::make_shared<sysrepo::Val>(value.c_str());
    }

    sysrepo::S_Val operator()(const bits_& value) const
    {
        std::stringstream ss;
        std::copy(value.m_bits.begin(), value.m_bits.end(), std::experimental::make_ostream_joiner(ss, " "));
        return std::make_shared<sysrepo::Val>(ss.str().c_str(), SR_BITS_T);
    }

    template <typename T>
    sysrepo::S_Val operator()(const T& value) const
    {
        return std::make_shared<sysrepo::Val>(value);
    }
};

SysrepoAccess::~SysrepoAccess() = default;

sr_datastore_t toSrDatastore(Datastore datastore)
{
    switch (datastore) {
    case Datastore::Running:
        return SR_DS_RUNNING;
    case Datastore::Startup:
        return SR_DS_STARTUP;
    }
    __builtin_unreachable();
}

SysrepoAccess::SysrepoAccess(const Datastore datastore)
    : m_connection(std::make_shared<sysrepo::Connection>())
    , m_session(std::make_shared<sysrepo::Session>(m_connection))
    , m_schema(std::make_shared<YangSchema>(m_session->get_context()))
{
    try {
        m_session = std::make_shared<sysrepo::Session>(m_connection, toSrDatastore(datastore));
    } catch (sysrepo::sysrepo_exception& ex) {
        reportErrors();
    }
}

DatastoreAccess::Tree SysrepoAccess::getItems(const std::string& path) const
{
    using namespace std::string_literals;
    Tree res;

    try {
        auto oldDs = m_session->session_get_ds();
        m_session->session_switch_ds(SR_DS_OPERATIONAL);
        auto config = m_session->get_data(((path == "/") ? "/*" : path).c_str());
        m_session->session_switch_ds(oldDs);
        if (config) {
            lyNodesToTree(res, config->tree_for());
        }
    } catch (sysrepo::sysrepo_exception& ex) {
        reportErrors();
    }
    return res;
}

void SysrepoAccess::setLeaf(const std::string& path, leaf_data_ value)
{
    try {
        m_session->set_item(path.c_str(), boost::apply_visitor(valFromValue(), value));
    } catch (sysrepo::sysrepo_exception& ex) {
        reportErrors();
    }
}

void SysrepoAccess::createItem(const std::string& path)
{
    try {
        m_session->set_item(path.c_str());
    } catch (sysrepo::sysrepo_exception& ex) {
        reportErrors();
    }
}

void SysrepoAccess::deleteItem(const std::string& path)
{
    try {
        // Have to use SR_EDIT_ISOLATE, because deleting something that's been set without committing is not supported
        // https://github.com/sysrepo/sysrepo/issues/1967#issuecomment-625085090
        m_session->delete_item(path.c_str(), SR_EDIT_ISOLATE);
    } catch (sysrepo::sysrepo_exception& ex) {
        reportErrors();
    }
}

struct impl_toSrMoveOp {
    sr_move_position_t operator()(yang::move::Absolute& absolute)
    {
        return absolute == yang::move::Absolute::Begin ? SR_MOVE_FIRST : SR_MOVE_LAST;
    }
    sr_move_position_t operator()(yang::move::Relative& relative)
    {
        return relative.m_position == yang::move::Relative::Position::After ? SR_MOVE_AFTER : SR_MOVE_BEFORE;
    }
};

sr_move_position_t toSrMoveOp(std::variant<yang::move::Absolute, yang::move::Relative> move)
{
    return std::visit(impl_toSrMoveOp{}, move);
}

void SysrepoAccess::moveItem(const std::string& source, std::variant<yang::move::Absolute, yang::move::Relative> move)
{
    std::string destination;
    if (std::holds_alternative<yang::move::Relative>(move)) {
        auto relative = std::get<yang::move::Relative>(move);
        if (m_schema->nodeType(source) == yang::NodeTypes::LeafList) {
            destination = leafDataToString(relative.m_path.at("."));
        } else {
            destination = instanceToString(relative.m_path);
        }
    }
    m_session->move_item(source.c_str(), toSrMoveOp(move), destination.c_str(), destination.c_str());
}

void SysrepoAccess::commitChanges()
{
    try {
        m_session->apply_changes(OPERATION_TIMEOUT_MS, 1);
    } catch (sysrepo::sysrepo_exception& ex) {
        reportErrors();
    }
}

void SysrepoAccess::discardChanges()
{
    try {
        m_session->discard_changes();
    } catch (sysrepo::sysrepo_exception& ex) {
        reportErrors();
    }
}

DatastoreAccess::Tree SysrepoAccess::execute(const std::string& path, const Tree& input)
{
    auto inputNode = treeToRpcInput(m_session->get_context(), path, input);
    auto output = m_session->rpc_send(inputNode);
    return rpcOutputToTree(path, output);
}

void SysrepoAccess::copyConfig(const Datastore source, const Datastore destination)
{
    auto oldDs = m_session->session_get_ds();
    m_session->session_switch_ds(toSrDatastore(destination));
    m_session->copy_config(toSrDatastore(source), nullptr, OPERATION_TIMEOUT_MS, 1);
    m_session->session_switch_ds(oldDs);
}

std::shared_ptr<Schema> SysrepoAccess::schema()
{
    return m_schema;
}

[[noreturn]] void SysrepoAccess::reportErrors() const
{
    // I only use get_error to get error info, since the error code from
    // sysrepo_exception doesn't really give any meaningful information. For
    // example an "invalid argument" error could mean a node isn't enabled, or
    // it could mean something totally different and there is no documentation
    // for that, so it's better to just use the message sysrepo gives me.
    auto srErrors = m_session->get_error();
    std::vector<DatastoreError> res;

    for (size_t i = 0; i < srErrors->error_cnt(); i++) {
        res.emplace_back(srErrors->message(i), srErrors->xpath(i) ? std::optional<std::string>{srErrors->xpath(i)} : std::nullopt);
    }

    throw DatastoreException(res);
}

std::vector<ListInstance> SysrepoAccess::listInstances(const std::string& path)
{
    std::vector<ListInstance> res;
    auto lists = getItems(path);

    decltype(lists) instances;
    auto wantedTree = *(m_schema->dataNodeFromPath(path)->find_path(path.c_str())->data().begin());
    std::copy_if(lists.begin(), lists.end(), std::inserter(instances, instances.end()), [this, pathToCheck = wantedTree->schema()->path()](const auto& item) {
        // This filters out non-instances.
        if (item.second.type() != typeid(special_) || boost::get<special_>(item.second).m_value != SpecialValue::List) {
            return false;
        }

        // Now, getItems is recursive: it gives everything including nested lists. So I try create a tree from the instance...
        auto instanceTree = *(m_schema->dataNodeFromPath(item.first)->find_path(item.first.c_str())->data().begin());
        // And then check if its schema path matches the list we actually want. This filters out lists which are not the ones I requested.
        return instanceTree->schema()->path() == pathToCheck;
    });

    // If there are no instances, then just return
    if (instances.empty()) {
        return res;
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
    for (const auto& instance : instances) {
        auto wantedList = *(m_schema->dataNodeFromPath(instance.first)->find_path(path.c_str())->data().begin());
        ListInstance instanceRes;
        for (const auto& key : keys) {
            auto vec = wantedList->find_path(key->name())->data();
            auto leaf = std::make_shared<libyang::Data_Node_Leaf_List>(*(vec.begin()));
            instanceRes.emplace(key->name(), leafValueFromNode(leaf));
        }
        res.emplace_back(instanceRes);
    }

    return res;
}

std::string SysrepoAccess::dump(const DataFormat format) const
{
    auto root = m_session->get_data("/*");
    return root->print_mem(format == DataFormat::Xml ? LYD_XML : LYD_JSON, LYP_WITHSIBLINGS | LYP_FORMAT);
}
