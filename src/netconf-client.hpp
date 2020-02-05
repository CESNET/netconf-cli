#pragma once

#include <functional>
#include <libnetconf2/log.h>
#include <libnetconf2/messages_client.h>
#include <libssh/libssh.h>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

struct nc_session;

namespace libyang {
class Data_Node;
class Context;
}

namespace ssh {
class Session;
}

namespace libnetconf {
namespace client {

class ReportedError : public std::runtime_error {
public:
    ReportedError(const std::string& what);
    ~ReportedError() override;
};

using KbdInteractiveCb = std::function<std::string(const std::string&, const std::string&, const std::string&, bool)>;
using LogCb = std::function<void(NC_VERB_LEVEL, const char*)>;

void setLogLevel(NC_VERB_LEVEL level);
void setLogCallback(const LogCb& callback);

class Session {
public:
    Session(struct nc_session* session);
    ~Session();
    static std::unique_ptr<Session> fromSshSession(ssh_session_struct* sshSession);
    static std::unique_ptr<Session> connectPubkey(const std::string& host, const uint16_t port, const std::string& user, const std::string& pubPath, const std::string& privPath);
    static std::unique_ptr<Session> connectKbdInteractive(const std::string& host, const uint16_t port, const std::string& user, const KbdInteractiveCb& callback);
    static std::unique_ptr<Session> connectSocket(const std::string& path);
    static std::unique_ptr<Session> connectFd(const int inFd, const int outFd);
    [[nodiscard]] std::vector<std::string_view> capabilities() const;
    std::shared_ptr<libyang::Data_Node> getConfig(const NC_DATASTORE datastore,
                                                  const std::optional<const std::string> filter = std::nullopt); // TODO: arguments...
    std::shared_ptr<libyang::Data_Node> get(const std::optional<std::string>& filter = std::nullopt);
    std::string getSchema(const std::string_view identifier, const std::optional<std::string_view> version);
    void editConfig(const NC_DATASTORE datastore,
                    const NC_RPC_EDIT_DFLTOP defaultOperation,
                    const NC_RPC_EDIT_TESTOPT testOption,
                    const NC_RPC_EDIT_ERROPT errorOption,
                    const std::string& data);
    void copyConfigFromString(const NC_DATASTORE target, const std::string& data);
    std::shared_ptr<libyang::Data_Node> rpc_or_action(const std::string& xmlData);
    void copyConfig(const NC_DATASTORE source, const NC_DATASTORE destination);
    void commit();
    void discard();

    std::shared_ptr<libyang::Context> libyangContext();
    struct nc_session* session_internal(); // FIXME: remove me
protected:
    struct nc_session* m_session;
};
}
}
