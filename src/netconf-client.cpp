#include <libyang/Tree_Data.hpp>
#include <mutex>
extern "C" {
#include <nc_client.h>
}
#include <spdlog/spdlog.h>
#include <sstream>
#include <tuple>
#include "netconf-client.h"

#define NETCONF_CLIENT_TRACE_FUNC \
    spdlog::get("netconf-client")->trace("{}", __func__)

namespace libnetconf {

namespace impl {

void log_via_spdlog(NC_VERB_LEVEL level, const char *message)
{
    auto log = spdlog::get("libnetconf2");
    switch (level) {
    case NC_VERB_ERROR:
        log->error("{}", message);
        break;
    case NC_VERB_WARNING:
        log->warn("{}", message);
        break;
    case NC_VERB_VERBOSE:
        log->info("{}", message);
        break;
    case NC_VERB_DEBUG:
        log->debug("{}", message);
        break;
    }
}

/** @short Initialization of the libnetconf2 library client

Just a safe wrapper over nc_client_init and nc_client_destroy, really.
*/
class ClientInit {
    ClientInit()
    {
        nc_client_init();
        nc_set_print_clb(log_via_spdlog);
        nc_verbosity(NC_VERB_DEBUG);
    }

    ~ClientInit()
    {
        nc_client_destroy();
    }

public:
    static ClientInit& instance() {
        static ClientInit lib;
        return lib;
    }

    ClientInit(const ClientInit&) = delete;
    ClientInit(ClientInit&&) = delete;
    ClientInit& operator=(const ClientInit&) = delete;
    ClientInit& operator=(ClientInit&&) = delete;
};

static std::mutex clientOptions;

int dummy_ssh_host_key_check(const char *hostname, ssh_session session, void *priv)
{
    NETCONF_CLIENT_TRACE_FUNC;
    std::ignore = hostname;
    std::ignore = session;
    std::ignore = priv;
    return 0;
}
//char *dummy_ssh_auth_password_cb(const char *username, const char *hostname, void *priv)
//{
//    NETCONF_CLIENT_TRACE_FUNC;
//    std::ignore = username;
//    std::ignore = hostname;
//    std::ignore = priv;
//    return ::strdup("");
//}
//char *dummy_ssh_auth_interactive_cb(const char *auth_name, const char *instruction, const char *prompt, int echo, void *priv)
//{
//    NETCONF_CLIENT_TRACE_FUNC;
//    std::ignore = auth_name;
//    std::ignore = instruction;
//    std::ignore = prompt;
//    std::ignore = echo;
//    std::ignore = priv;
//    return ::strdup("");
//}

inline void custom_free_nc_reply_data(nc_reply_data* reply)
{
    NETCONF_CLIENT_TRACE_FUNC;
    nc_reply_free(reinterpret_cast<nc_reply*>(reply));
}
inline void custom_free_nc_reply_error(nc_reply_error* reply)
{
    NETCONF_CLIENT_TRACE_FUNC;
    nc_reply_free(reinterpret_cast<nc_reply*>(reply));
}

template <auto fn>
using deleter_from_fn = std::integral_constant<decltype(fn), fn>;

template <typename T>
struct deleter_type_for {
    using func_type = void(*)(T*);
};

template <typename T>
struct deleter_for {};

template<> struct deleter_for<struct nc_rpc> {
    static constexpr void(*free)(struct nc_rpc*) = deleter_from_fn<nc_rpc_free>();
};
template<> struct deleter_for<struct nc_reply> {
    static constexpr void(*free)(struct nc_reply*) = deleter_from_fn<nc_reply_free>();
};
template<> struct deleter_for<struct nc_reply_data> {
    static constexpr void(*free)(struct nc_reply_data*) = deleter_from_fn<custom_free_nc_reply_data>();
};
template<> struct deleter_for<struct nc_reply_error> {
    static constexpr void(*free)(struct nc_reply_error*) = deleter_from_fn<custom_free_nc_reply_error>();
};

template <typename T>
using unique_ptr_for = std::unique_ptr<T, decltype(deleter_for<T>::free)>;

template <typename T>
auto guarded(T* ptr)
{
    return unique_ptr_for<T>(ptr, deleter_for<T>::free);
}

unique_ptr_for<struct nc_reply> do_rpc(client::Session *session, unique_ptr_for<struct nc_rpc>&& rpc)
{
    NETCONF_CLIENT_TRACE_FUNC;
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

unique_ptr_for<struct nc_reply_data> do_rpc_data(client::Session *session, unique_ptr_for<struct nc_rpc>&& rpc)
{
    NETCONF_CLIENT_TRACE_FUNC;
    auto x = do_rpc(session, std::move(rpc));

    switch (x->type) {
    case NC_RPL_DATA:
        return guarded(reinterpret_cast<struct nc_reply_data*>(x.release()));
    case NC_RPL_OK:
        throw std::runtime_error{"Received OK instead of a daat reply"};
    case NC_RPL_ERROR:
        throw make_error(std::move(x));
    default:
        throw std::runtime_error{"Unhandled reply type"};
    }
}

void do_rpc_ok(client::Session *session, unique_ptr_for<struct nc_rpc>&& rpc)
{
    NETCONF_CLIENT_TRACE_FUNC;
    auto x = do_rpc(session, std::move(rpc));

    switch (x->type) {
    case NC_RPL_DATA:
        throw std::runtime_error{"Unexpected DATA reply"};
    case NC_RPL_OK:
        return;
    case NC_RPL_ERROR:
        throw make_error(std::move(x));
    default:
        throw std::runtime_error{"Unhandled reply type"};
    }
}
}

namespace client {

struct nc_session* Session::session_internal()
{
    return m_session;
}

Session::Session(struct nc_session* session)
    : m_session(session)
{
    impl::ClientInit::instance();
}

Session::~Session()
{
    NETCONF_CLIENT_TRACE_FUNC;
    ::nc_session_free(m_session, nullptr);
}

std::unique_ptr<Session> Session::connectPubkey(const std::string& host, const uint16_t port, const std::string& user, const std::string& pubPath, const std::string& privPath)
{
    impl::ClientInit::instance();

    NETCONF_CLIENT_TRACE_FUNC;
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

//std::unique_ptr<Session> Session::viaSSH(const std::string& host, const uint16_t port, const std::string& user)
//{
//    impl::ClientInit::instance();
//
//    NETCONF_CLIENT_TRACE_FUNC;
//    {
//        // FIXME: this is still horribly not enough. libnetconf *must* provide us with something better.
//        std::lock_guard lk(impl::clientOptions);
//        nc_client_ssh_set_username(user.c_str());
//        nc_client_ssh_set_auth_hostkey_check_clb(impl::dummy_ssh_host_key_check, nullptr);
//        nc_client_ssh_set_auth_password_clb(impl::dummy_ssh_auth_password_cb, nullptr);
//        nc_client_ssh_set_auth_interactive_clb(impl::dummy_ssh_auth_interactive_cb, nullptr);
//    }
//    auto session = std::make_unique<Session>(nc_connect_ssh(host.c_str(), port, nullptr));
//    if (!session->m_session) {
//        throw std::runtime_error{"nc_connect_ssh failed"};
//    }
//    return session;
//}

std::vector<std::string_view> Session::capabilities() const
{
    NETCONF_CLIENT_TRACE_FUNC;
    std::vector<std::string_view> res;
    auto caps = nc_session_get_cpblts(m_session);
    while (*caps) {
        res.emplace_back(*caps);
        ++caps;
    }
    return res;
}

std::shared_ptr<libyang::Data_Node> Session::get(const std::string& filter)
{
    NETCONF_CLIENT_TRACE_FUNC;
    auto rpc = impl::guarded(nc_rpc_get(filter.c_str(), NC_WD_ALL, NC_PARAMTYPE_CONST));
    if (!rpc) {
        throw std::runtime_error("Cannot create get RPC");
    }
    auto reply = impl::do_rpc_data(this, std::move(rpc));
    // TODO: can we do without copying?
    // If we just default-construct a new node (or use the create_new_Data_Node) and then set reply->data to nullptr,
    // there are mem leaks and even libnetconf2 complains loudly.
    return libyang::create_new_Data_Node(reply->data)->dup_withsiblings(1);
}

std::string Session::getSchema(const char* identifier, const char* version)
{
    NETCONF_CLIENT_TRACE_FUNC;
    auto rpc = impl::guarded(nc_rpc_getschema(identifier, version, nullptr, NC_PARAMTYPE_CONST));
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
    return "";
}

std::shared_ptr<libyang::Data_Node> Session::getConfig(const NC_DATASTORE datastore, const std::string& filter)
{
    NETCONF_CLIENT_TRACE_FUNC;
    auto rpc = impl::guarded(nc_rpc_getconfig(datastore, filter.c_str(), NC_WD_ALL, NC_PARAMTYPE_CONST));
    if (!rpc) {
        throw std::runtime_error("Cannot create get-config RPC");
    }
    auto reply = impl::do_rpc_data(this, std::move(rpc));
    // TODO: can we do without copying?
    // If we just default-construct a new node (or use the create_new_Data_Node) and then set reply->data to nullptr,
    // there are mem leaks and even libnetconf2 complains loudly.
    auto dataNode = libyang::create_new_Data_Node(reply->data);
    if (!dataNode)
        return nullptr;
    else
        return dataNode->dup_withsiblings(1);
}

void Session::editConfig(const NC_DATASTORE datastore,
                         const NC_RPC_EDIT_DFLTOP defaultOperation,
                         const NC_RPC_EDIT_TESTOPT testOption,
                         const NC_RPC_EDIT_ERROPT errorOption,
                         const std::string& data)
{
    NETCONF_CLIENT_TRACE_FUNC;
    auto rpc = impl::guarded(nc_rpc_edit(datastore, defaultOperation, testOption, errorOption, data.c_str(), NC_PARAMTYPE_CONST));
    if (!rpc) {
        throw std::runtime_error("Cannot create edit-config RPC");
    }
    impl::do_rpc_ok(this, std::move(rpc));
}

void Session::copyConfigFromString(const NC_DATASTORE target, const std::string& data)
{
    NETCONF_CLIENT_TRACE_FUNC;
    auto rpc = impl::guarded(nc_rpc_copy(target, nullptr, target /* yeah, cannot be 0... */, data.c_str(), NC_WD_UNKNOWN, NC_PARAMTYPE_CONST));
    if (!rpc) {
        throw std::runtime_error("Cannot create copy-config RPC");
    }
    impl::do_rpc_ok(this, std::move(rpc));
}

void Session::commit()
{
    NETCONF_CLIENT_TRACE_FUNC;
    auto rpc = impl::guarded(nc_rpc_commit(1, /* "Optional confirm timeout" how do you optional an uint32_t? */ 0, NULL, NULL, NC_PARAMTYPE_CONST));
    if (!rpc) {
        throw std::runtime_error("Cannot create commit RPC");
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
