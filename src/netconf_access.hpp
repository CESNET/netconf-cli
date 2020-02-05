/*
 * Copyright (C) 2019 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
*/

#pragma once

#include <libnetconf2/log.h>
#include <string>
#include "datastore_access.hpp"

/*! \class NetconfAccess
 *     \brief Implementation of DatastoreAccess for accessing a NETCONF server
 */

namespace libnetconf {
namespace client {
class Session;
}
}

namespace libyang {
class Data_Node;
}

struct ssh_session_struct;

class Schema;
class YangSchema;

using LogCb = std::function<void(NC_VERB_LEVEL, const char*)>;

class NetconfAccess : public DatastoreAccess {
public:
    NetconfAccess(const std::string& hostname, uint16_t port, const std::string& user, const std::string& pubKey, const std::string& privKey);
    NetconfAccess(const std::string& socketPath);
    NetconfAccess(std::unique_ptr<libnetconf::client::Session>&& session);
    NetconfAccess(ssh_session_struct* session);
    ~NetconfAccess() override;

    static void setNcLogLevel(NC_VERB_LEVEL level);
    static void setNcLogCallback(const LogCb& callback);
    Tree getItems(const std::string& path) override;
    void setLeaf(const std::string& path, leaf_data_ value) override;
    void createPresenceContainer(const std::string& path) override;
    void deletePresenceContainer(const std::string& path) override;
    void createListInstance(const std::string& path) override;
    void deleteListInstance(const std::string& path) override;
    void commitChanges() override;
    void discardChanges() override;
    Tree executeRpc(const std::string& path, const Tree& input) override;

    std::shared_ptr<Schema> schema() override;

private:
    std::vector<std::map<std::string, leaf_data_>> listInstances(const std::string& path) override;

    std::string fetchSchema(const std::string_view module, const
            std::optional<std::string_view> revision, const
            std::optional<std::string_view> submodule, const
            std::optional<std::string_view> submoduleRevision);
    std::vector<std::string> listImplementedSchemas();
    void datastoreInit();
    void doEditFromDataNode(std::shared_ptr<libyang::Data_Node> dataNode);

    std::unique_ptr<libnetconf::client::Session> m_session;
    std::shared_ptr<YangSchema> m_schema;
};
