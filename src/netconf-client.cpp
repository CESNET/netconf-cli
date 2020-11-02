/*
 * Copyright (C) 2019 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 * Written by Jan Kundrát <jan.kundrat@cesnet.cz>
 *
*/

#include <cstring>
#include <libyang/Tree_Data.hpp>
#include <mutex>
extern "C" {
#include <nc_client.h>
}
#include <sstream>
#include "netconf-client.hpp"
#include "UniqueResource.hpp"

namespace libnetconf {

namespace impl {

/** @short Initialization of the libnetconf2 library client

Just a safe wrapper over nc_client_init and nc_client_destroy, really.
*/
class ClientInit {
    ClientInit()
    {
        nc_client_init();
        nc_verbosity(NC_VERB_DEBUG);
    }

    ~ClientInit()
    {
        nc_client_destroy();
    }

public:
    static ClientInit& instance()
    {
        static ClientInit lib;
        return lib;
    }

    ClientInit(const ClientInit&) = delete;
    ClientInit(ClientInit&&) = delete;
    ClientInit& operator=(const ClientInit&) = delete;
    ClientInit& operator=(ClientInit&&) = delete;
};

static std::mutex clientOptions;

inline void custom_free_nc_reply_data(nc_reply_data* reply)
{
    nc_reply_free(reinterpret_cast<nc_reply*>(reply));
}
inline void custom_free_nc_reply_error(nc_reply_error* reply)
{
    nc_reply_free(reinterpret_cast<nc_reply*>(reply));
}

char *ssh_auth_interactive_cb(const char *auth_name, const char *instruction, const char *prompt, int echo, void *priv)
{
    const auto cb = static_cast<const client::KbdInteractiveCb*>(priv);
    auto res = (*cb)(auth_name, instruction, prompt, echo);
    return ::strdup(res.c_str());
}

template <typename Type> using deleter_type_for = void (*)(Type*);
template <typename Type> deleter_type_for<Type> deleter_for;

template <> const auto deleter_for<nc_rpc> = nc_rpc_free;
template <> const auto deleter_for<nc_reply> = nc_reply_free;
template <> const auto deleter_for<nc_reply_data> = custom_free_nc_reply_data;
template <> const auto deleter_for<nc_reply_error> = custom_free_nc_reply_error;

template <typename T>
using unique_ptr_for = std::unique_ptr<T, decltype(deleter_for<T>)>;

template <typename T>
auto guarded(T* ptr)
{
    return unique_ptr_for<T>(ptr, deleter_for<T>);
}

unique_ptr_for<struct nc_reply> do_rpc(client::Session* session, unique_ptr_for<struct nc_rpc>&& rpc)
{
    uint64_t msgid;
    NC_MSG_TYPE msgtype;

    msgtype = nc_send_rpc(session->session_internal(), rpc.get(), 1000, &msgid);
    if (msgtype == NC_MSG_ERROR) {
        throw std::runtime_error{"Failed to send RPC"};
    }
    if (msgtype == NC_MSG_WOULDBLOCK) {
        throw std::runtime_error{"Timeout sending an RPC"};
    }

    struct nc_reply* raw_reply;
    while (true) {
        msgtype = nc_recv_reply(session->session_internal(), rpc.get(), msgid, 20000, LYD_OPT_DESTRUCT | LYD_OPT_NOSIBLINGS, &raw_reply);
        auto reply = guarded(raw_reply);
        raw_reply = nullptr;

        switch (msgtype) {
        case NC_MSG_ERROR:
            throw std::runtime_error{"Failed to receive an RPC reply"};
        case NC_MSG_WOULDBLOCK:
            throw std::runtime_error{"Timed out waiting for RPC reply"};
        case NC_MSG_REPLY_ERR_MSGID:
            throw std::runtime_error{"Received a wrong reply -- msgid mismatch"};
        case NC_MSG_NOTIF:
            continue;
        default:
            return reply;
        }
    }
    __builtin_unreachable();
}

client::ReportedError make_error(unique_ptr_for<struct nc_reply>&& reply)
{
    if (reply->type != NC_RPL_ERROR) {
        throw std::logic_error{"Cannot extract an error from a non-error server reply"};
    }

    auto errorReply = guarded(reinterpret_cast<struct nc_reply_error*>(reply.release()));

    // TODO: capture the error details, not just that human-readable string
    std::ostringstream ss;
    ss << "Server error:" << std::endl;
    for (uint32_t i = 0; i < errorReply->count; ++i) {
        const auto e = errorReply->err[i];
        ss << " #" << i << ": " << e.message;
        if (e.path) {
            ss << " (XPath " << e.path << ")";
        }
        ss << std::endl;
    }
    return client::ReportedError{ss.str()};
}

std::optional<unique_ptr_for<struct nc_reply_data>> do_rpc_data_or_ok(client::Session* session, unique_ptr_for<struct nc_rpc>&& rpc)
{
    auto x = do_rpc(session, std::move(rpc));

    switch (x->type) {
    case NC_RPL_DATA:
        return guarded(reinterpret_cast<struct nc_reply_data*>(x.release()));
    case NC_RPL_OK:
        return std::nullopt;
    case NC_RPL_ERROR:
        throw make_error(std::move(x));
    default:
        throw std::runtime_error{"Unhandled reply type"};
    }

}

unique_ptr_for<struct nc_reply_data> do_rpc_data(client::Session* session, unique_ptr_for<struct nc_rpc>&& rpc)
{
    auto x = do_rpc_data_or_ok(session, std::move(rpc));
    if (!x) {
        throw std::runtime_error{"Received OK instead of a data reply"};
    }
    return std::move(*x);
}

void do_rpc_ok(client::Session* session, unique_ptr_for<struct nc_rpc>&& rpc)
{
    auto x = do_rpc_data_or_ok(session, std::move(rpc));
    if (x) {
        throw std::runtime_error{"Unexpected DATA reply"};
    }
}
}

namespace client {

struct nc_session* Session::session_internal()
{
    return m_session;
}

libyang::S_Context Session::libyangContext()
{
    return std::make_shared<libyang::Context>(nc_session_get_ctx(m_session), nullptr);
}

Session::Session(struct nc_session* session)
    : m_session(session)
{
    impl::ClientInit::instance();
}

Session::~Session()
{
    ::nc_session_free(m_session, nullptr);
}

std::unique_ptr<Session> Session::connectPubkey(const std::string& host, const uint16_t port, const std::string& user, const std::string& pubPath, const std::string& privPath)
{
    impl::ClientInit::instance();

    {
        // FIXME: this is still horribly not enough. libnetconf *must* provide us with something better.
        std::lock_guard lk(impl::clientOptions);
        nc_client_ssh_set_username(user.c_str());
        nc_client_ssh_set_auth_pref(NC_SSH_AUTH_PUBLICKEY, 5);
        nc_client_ssh_add_keypair(pubPath.c_str(), privPath.c_str());
    }
    auto session = std::make_unique<Session>(nc_connect_ssh(host.c_str(), port, nullptr));
    if (!session->m_session) {
        throw std::runtime_error{"nc_connect_ssh failed"};
    }
    return session;
}

std::unique_ptr<Session> Session::connectKbdInteractive(const std::string& host, const uint16_t port, const std::string& user, const KbdInteractiveCb& callback)
{
    impl::ClientInit::instance();

    std::lock_guard lk(impl::clientOptions);
    auto cb_guard = make_unique_resource([user, &callback]() {
        nc_client_ssh_set_username(user.c_str());
        nc_client_ssh_set_auth_pref(NC_SSH_AUTH_INTERACTIVE, 5);
        nc_client_ssh_set_auth_interactive_clb(impl::ssh_auth_interactive_cb, static_cast<void *>(&const_cast<KbdInteractiveCb&>(callback)));
    }, []() {
        nc_client_ssh_set_auth_interactive_clb(nullptr, nullptr);
        nc_client_ssh_set_username(nullptr);
    });

    auto session = std::make_unique<Session>(nc_connect_ssh(host.c_str(), port, nullptr));
    if (!session->m_session) {
        throw std::runtime_error{"nc_connect_ssh failed"};
    }
    return session;
}

std::unique_ptr<Session> Session::connectFd(const int source, const int sink)
{
    impl::ClientInit::instance();

    auto session = std::make_unique<Session>(nc_connect_inout(source, sink, nullptr));
    if (!session->m_session) {
        throw std::runtime_error{"nc_connect_inout failed"};
    }
    return session;
}

std::unique_ptr<Session> Session::connectSocket(const std::string& path)
{
    impl::ClientInit::instance();

    auto session = std::make_unique<Session>(nc_connect_unix(path.c_str(), nullptr));
    if (!session->m_session) {
        throw std::runtime_error{"nc_connect_unix failed"};
    }
    return session;
}

std::vector<std::string_view> Session::capabilities() const
{
    std::vector<std::string_view> res;
    auto caps = nc_session_get_cpblts(m_session);
    while (*caps) {
        res.emplace_back(*caps);
        ++caps;
    }
    return res;
}

std::shared_ptr<libyang::Data_Node> Session::get(const std::optional<std::string>& filter)
{
    auto rpc = impl::guarded(nc_rpc_get(filter ? filter->c_str() : nullptr, NC_WD_ALL, NC_PARAMTYPE_CONST));
    if (!rpc) {
        throw std::runtime_error("Cannot create get RPC");
    }
    auto reply = impl::do_rpc_data(this, std::move(rpc));
    auto dataNode = libyang::create_new_Data_Node(reply->data);
    // TODO: can we do without copying?
    // If we just default-construct a new node (or use the create_new_Data_Node) and then set reply->data to nullptr,
    // there are mem leaks and even libnetconf2 complains loudly.
    return dataNode ? dataNode->dup_withsiblings(1) : nullptr;
}

std::string Session::getSchema(const std::string_view identifier, const std::optional<std::string_view> version)
{
    auto rpc = impl::guarded(nc_rpc_getschema(identifier.data(), version ? version.value().data() : nullptr, nullptr, NC_PARAMTYPE_CONST));
    if (!rpc) {
        throw std::runtime_error("Cannot create get-schema RPC");
    }
    auto reply = impl::do_rpc_data(this, std::move(rpc));

    auto node = libyang::create_new_Data_Node(reply->data)->dup_withsiblings(1);
    auto set = node->find_path("data");
    for (auto node : set->data()) {
        if (node->schema()->nodetype() == LYS_ANYXML) {
            libyang::Data_Node_Anydata anydata(node);
            return anydata.value().str;
        }
    }
    throw std::runtime_error("Got a reply to get-schema, but it didn't contain the required schema");
}

std::shared_ptr<libyang::Data_Node> Session::getConfig(const NC_DATASTORE datastore, const std::optional<const std::string> filter)
{
    auto rpc = impl::guarded(nc_rpc_getconfig(datastore, filter ? filter->c_str() : nullptr, NC_WD_ALL, NC_PARAMTYPE_CONST));
    if (!rpc) {
        throw std::runtime_error("Cannot create get-config RPC");
    }
    auto reply = impl::do_rpc_data(this, std::move(rpc));
    auto dataNode = libyang::create_new_Data_Node(reply->data);
    // TODO: can we do without copying? See Session::get() for details.
    return dataNode ? dataNode->dup_withsiblings(1) : nullptr;
}

void Session::editConfig(const NC_DATASTORE datastore,
                         const NC_RPC_EDIT_DFLTOP defaultOperation,
                         const NC_RPC_EDIT_TESTOPT testOption,
                         const NC_RPC_EDIT_ERROPT errorOption,
                         const std::string& data)
{
    auto rpc = impl::guarded(nc_rpc_edit(datastore, defaultOperation, testOption, errorOption, data.c_str(), NC_PARAMTYPE_CONST));
    if (!rpc) {
        throw std::runtime_error("Cannot create edit-config RPC");
    }
    impl::do_rpc_ok(this, std::move(rpc));
}

void Session::copyConfigFromString(const NC_DATASTORE target, const std::string& data)
{
    auto rpc = impl::guarded(nc_rpc_copy(target, nullptr, target /* yeah, cannot be 0... */, data.c_str(), NC_WD_UNKNOWN, NC_PARAMTYPE_CONST));
    if (!rpc) {
        throw std::runtime_error("Cannot create copy-config RPC");
    }
    impl::do_rpc_ok(this, std::move(rpc));
}

void Session::commit()
{
    auto rpc = impl::guarded(nc_rpc_commit(1, /* "Optional confirm timeout" how do you optional an uint32_t? */ 0, nullptr, nullptr, NC_PARAMTYPE_CONST));
    if (!rpc) {
        throw std::runtime_error("Cannot create commit RPC");
    }
    impl::do_rpc_ok(this, std::move(rpc));
}

void Session::discard()
{
    auto rpc = impl::guarded(nc_rpc_discard());
    if (!rpc) {
        throw std::runtime_error("Cannot create discard RPC");
    }
    impl::do_rpc_ok(this, std::move(rpc));
}

std::shared_ptr<libyang::Data_Node> Session::rpc_or_action(const std::string& xmlData)
{
    auto rpc = impl::guarded(nc_rpc_act_generic_xml(xmlData.c_str(), NC_PARAMTYPE_CONST));
    if (!rpc) {
        throw std::runtime_error("Cannot create generic RPC");
    }
    auto reply = impl::do_rpc_data_or_ok(this, std::move(rpc));
    if (reply) {
        auto dataNode = libyang::create_new_Data_Node((*reply)->data);
        return dataNode->dup_withsiblings(1);
    } else {
        return nullptr;
    }
}

void Session::copyConfig(const NC_DATASTORE source, const NC_DATASTORE destination)
{
    auto rpc = impl::guarded(nc_rpc_copy(destination, nullptr, source, nullptr, NC_WD_UNKNOWN, NC_PARAMTYPE_CONST));
    if (!rpc) {
        throw std::runtime_error("Cannot create copy-config RPC");
    }
    impl::do_rpc_ok(this, std::move(rpc));
}

ReportedError::ReportedError(const std::string& what)
    : std::runtime_error(what)
{
}

ReportedError::~ReportedError() = default;
}
}
