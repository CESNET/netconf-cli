/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/

#include <experimental/iterator>
#include <sstream>
#include <sysrepo-cpp/Session.hpp>
#include <sysrepo-cpp/utils/exception.hpp>
#include "libyang_utils.hpp"
#include "sysrepo_access.hpp"
#include "utils.hpp"
#include "yang_schema.hpp"

const auto OPERATION_TIMEOUT_MS = std::chrono::milliseconds{1000};

SysrepoAccess::~SysrepoAccess() = default;

sysrepo::Datastore toSrDatastore(Datastore datastore)
{
    switch (datastore) {
    case Datastore::Running:
        return sysrepo::Datastore::Running;
    case Datastore::Startup:
        return sysrepo::Datastore::Startup;
    }
    __builtin_unreachable();
}

SysrepoAccess::SysrepoAccess()
    : m_connection()
    , m_session(m_connection.sessionStart())
    , m_schema(std::make_shared<YangSchema>(m_session.getContext()))
{
    // try {
    //     m_session = std::make_shared<sysrepo::Session>(m_connection);
    // } catch (sysrepo::Error& ex) {
    //     reportErrors();
    // }
}

namespace {
auto targetToDs_get(const DatastoreTarget target)
{
    switch (target) {
    case DatastoreTarget::Operational:
        return sysrepo::Datastore::Operational;
    case DatastoreTarget::Running:
        return sysrepo::Datastore::Running;
    case DatastoreTarget::Startup:
        return sysrepo::Datastore::Startup;
    }

    __builtin_unreachable();
}

auto targetToDs_set(const DatastoreTarget target)
{
    switch (target) {
    case DatastoreTarget::Operational:
    case DatastoreTarget::Running:
        // TODO: Doing candidate here doesn't work, why?
        return sysrepo::Datastore::Running;
    case DatastoreTarget::Startup:
        return sysrepo::Datastore::Startup;
    }

    __builtin_unreachable();
}
}

DatastoreAccess::Tree SysrepoAccess::getItems(const std::string& path) const
{
    using namespace std::string_literals;
    Tree res;

    try {
        m_session.switchDatastore(targetToDs_get(m_target));
        auto config = m_session.getData(((path == "/") ? "/*" : path).c_str());
        if (config) {
            lyNodesToTree(res, config->siblings());
        }
    } catch (sysrepo::Error& ex) {
        reportErrors();
    }
    return res;
}

void SysrepoAccess::setLeaf(const std::string& path, leaf_data_ value)
{
    try {
        m_session.switchDatastore(targetToDs_set(m_target));
        auto lyValue = value.type() == typeid(empty_) ? "" : leafDataToString(value);
        m_session.setItem(path.c_str(), lyValue.c_str());
    } catch (sysrepo::Error& ex) {
        reportErrors();
    }
}

void SysrepoAccess::createItem(const std::string& path)
{
    try {
        m_session.switchDatastore(targetToDs_set(m_target));
        m_session.setItem(path.c_str(), nullptr);
    } catch (sysrepo::Error& ex) {
        reportErrors();
    }
}

void SysrepoAccess::deleteItem(const std::string& path)
{
    try {
        // Have to use sysrepo::EditOptions::Isolate, because deleting something that's been set without committing is
        // not supported.
        // https://github.com/sysrepo/sysrepo/issues/1967#issuecomment-625085090
        m_session.switchDatastore(targetToDs_set(m_target));
        m_session.deleteItem(path.c_str(), sysrepo::EditOptions::Isolate);
    } catch (sysrepo::Error& ex) {
        reportErrors();
    }
}

struct impl_toSrMoveOp {
    sysrepo::MovePosition operator()(yang::move::Absolute& absolute)
    {
        return absolute == yang::move::Absolute::Begin ? sysrepo::MovePosition::First : sysrepo::MovePosition::Last;
    }
    sysrepo::MovePosition operator()(yang::move::Relative& relative)
    {
        return relative.m_position == yang::move::Relative::Position::After ? sysrepo::MovePosition::After : sysrepo::MovePosition::Before;
    }
};

sysrepo::MovePosition toSrMoveOp(std::variant<yang::move::Absolute, yang::move::Relative> move)
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
    m_session.switchDatastore(targetToDs_set(m_target));
    m_session.moveItem(source.c_str(), toSrMoveOp(move), destination.c_str());
}

void SysrepoAccess::commitChanges()
{
    try {
        m_session.switchDatastore(targetToDs_set(m_target));
        m_session.applyChanges(OPERATION_TIMEOUT_MS);
    } catch (sysrepo::Error& ex) {
        reportErrors();
    }
}

void SysrepoAccess::discardChanges()
{
    try {
        m_session.switchDatastore(targetToDs_set(m_target));
        m_session.discardChanges();
    } catch (sysrepo::Error& ex) {
        reportErrors();
    }
}

DatastoreAccess::Tree SysrepoAccess::execute(const std::string& path, const Tree& input)
{
    auto inputNode = treeToRpcInput(m_session.getContext(), path, input);
    m_session.switchDatastore(targetToDs_set(m_target));
    auto output = m_session.sendRPC(inputNode);
    return rpcOutputToTree(output);
}

void SysrepoAccess::copyConfig(const Datastore source, const Datastore destination)
{
    m_session.switchDatastore(toSrDatastore(destination));
    m_session.copyConfig(toSrDatastore(source), nullptr, OPERATION_TIMEOUT_MS);
}

std::shared_ptr<Schema> SysrepoAccess::schema()
{
    return m_schema;
}

[[noreturn]] void SysrepoAccess::reportErrors() const
{
    // TODO: implement this
    // // I only use get_error to get error info, since the error code from
    // // sysrepo_exception doesn't really give any meaningful information. For
    // // example an "invalid argument" error could mean a node isn't enabled, or
    // // it could mean something totally different and there is no documentation
    // // for that, so it's better to just use the message sysrepo gives me.
    // auto srErrors = m_session->get_error();
    std::vector<DatastoreError> res;

    // for (size_t i = 0; i < srErrors->error_cnt(); i++) {
    //     res.emplace_back(srErrors->message(i), srErrors->xpath(i) ? std::optional<std::string>{srErrors->xpath(i)} : std::nullopt);
    // }

    throw DatastoreException(res);
}

std::vector<ListInstance> SysrepoAccess::listInstances(const std::string& path)
{
    std::vector<ListInstance> res;
    auto lists = m_session.getData(path.c_str());
    if (!lists) {
        return res;
    }

    auto instances = lists->findXPath(path.c_str());
    if (instances.begin() == instances.end()) { // TODO: implement empty
        return res;
    }

    auto keys = instances.front().schema().asList().keys();

    // Creating a full tree at the same time from the values sysrepo gives me
    // would be a pain (and after sysrepo switches to libyang meaningless), so
    // I just use this algorithm to create data nodes one by one and get the
    // key values from them.
    for (const auto& instance : instances) {
        ListInstance instanceRes;
        for (const auto& key : keys) {
            auto leaf = instance.findPath(key.name().data());
            instanceRes.emplace(std::string{leaf->schema().name()}, leafValueFromNode(leaf->asTerm()));
        }
        res.emplace_back(instanceRes);
    }

    return res;
}

std::string SysrepoAccess::dump(const DataFormat format) const
{
    auto root = m_session.getData("/*");
    auto str = root->printStr(format == DataFormat::Xml ? libyang::DataFormat::XML : libyang::DataFormat::JSON, libyang::PrintFlags::WithSiblings);
    if (!str) {
        return "";
    }

    return std::string{*str};
}
