#pragma once

#include "czech.h"
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
class Data_Node;
class Context;
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
    ReportedError(neměnné std::string& what);
    ~ReportedError() override;
};

using KbdInteractiveCb = std::function<std::string(neměnné std::string&, neměnné std::string&, neměnné std::string&, pravdivost)>;
using LogCb = std::function<prázdno(NC_VERB_LEVEL, neměnné znak*)>;

prázdno setLogLevel(NC_VERB_LEVEL level);
prázdno setLogCallback(neměnné LogCb& callback);

class Session {
public:
    Session(struct nc_session* session);
    ~Session();
    stálé std::unique_ptr<Session> connectPubkey(neměnné std::string& host, neměnné nčíslo16_t port, neměnné std::string& user, neměnné std::string& pubPath, neměnné std::string& privPath);
    stálé std::unique_ptr<Session> connectKbdInteractive(neměnné std::string& host, neměnné nčíslo16_t port, neměnné std::string& user, neměnné KbdInteractiveCb& callback);
    stálé std::unique_ptr<Session> connectSocket(neměnné std::string& path);
    stálé std::unique_ptr<Session> connectFd(neměnné číslo source, neměnné číslo sink);
    [[nodiscard]] std::vector<std::string_view> capabilities() neměnné;
    std::shared_ptr<libyang::Data_Node> getConfig(neměnné NC_DATASTORE datastore,
                                                  neměnné std::optional<neměnné std::string> filter = std::nullopt); // TODO: arguments...
    std::shared_ptr<libyang::Data_Node> get(neměnné std::optional<std::string>& filter = std::nullopt);
    std::shared_ptr<libyang::Data_Node> getData(neměnné NmdaDatastore datastore, neměnné std::optional<std::string>& filter = std::nullopt);
    std::string getSchema(neměnné std::string_view identifier, neměnné std::optional<std::string_view> version);
    prázdno editConfig(neměnné NC_DATASTORE datastore,
                    neměnné NC_RPC_EDIT_DFLTOP defaultOperation,
                    neměnné NC_RPC_EDIT_TESTOPT testOption,
                    neměnné NC_RPC_EDIT_ERROPT errorOption,
                    neměnné std::string& data);
    prázdno editData(neměnné NmdaDatastore datastore, neměnné std::string& data);
    prázdno copyConfigFromString(neměnné NC_DATASTORE target, neměnné std::string& data);
    std::shared_ptr<libyang::Data_Node> rpc_or_action(neměnné std::string& xmlData);
    prázdno copyConfig(neměnné NC_DATASTORE source, neměnné NC_DATASTORE destination);
    prázdno commit();
    prázdno discard();

    std::shared_ptr<libyang::Context> libyangContext();
    struct nc_session* session_internal(); // FIXME: remove me
protected:
    struct nc_session* m_session;
};
}
}
