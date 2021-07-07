#pragma once

#include <functional>
#include <libnetconf2/log.h>
#include <libnetconf2/messages_client.h>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

struct nc_session;

namespace libyang {
class Context;
class DataNode;
}

namespace libnetconf {
enum class NmdaDatastore {
    Startup,
    Running,
    Candidate,
    Operational
};
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
    static std::unique_ptr<Session> connectPubkey(const std::string& host, const uint16_t port, const std::string& user, const std::string& pubPath, const std::string& privPath);
    static std::unique_ptr<Session> connectKbdInteractive(const std::string& host, const uint16_t port, const std::string& user, const KbdInteractiveCb& callback);
    static std::unique_ptr<Session> connectSocket(const std::string& path);
    static std::unique_ptr<Session> connectFd(const int source, const int sink);
    [[nodiscard]] std::vector<std::string_view> capabilities() const;
    std::optional<libyang::DataNode> get(const std::optional<std::string>& filter = std::nullopt);
    std::optional<libyang::DataNode> getData(const NmdaDatastore datastore, const std::optional<std::string>& filter = std::nullopt);
    void editConfig(const NC_DATASTORE datastore,
                    const NC_RPC_EDIT_DFLTOP defaultOperation,
                    const NC_RPC_EDIT_TESTOPT testOption,
                    const NC_RPC_EDIT_ERROPT errorOption,
                    const std::string& data);
    void editData(const NmdaDatastore datastore, const std::string& data);
    void copyConfigFromString(const NC_DATASTORE target, const std::string& data);
    std::optional<libyang::DataNode> rpc_or_action(const std::string& xmlData);
    void copyConfig(const NC_DATASTORE source, const NC_DATASTORE destination);
    void commit();
    void discard();

    libyang::Context libyangContext();
    struct nc_session* session_internal(); // FIXME: remove me
protected:
    struct nc_session* m_session;
};
}
}
