/*
 * Copyright (C) 2019 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
*/

#pragma once

#include <string>
#include "ast_commands.hpp"
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

class Schema;
class YangSchema;

class NetconfAccess : public DatastoreAccess {
public:
    NetconfAccess(const std::string& hostname, uint16_t port, const std::string& user, const std::string& pubKey, const std::string& privKey);
    NetconfAccess(const std::string& socketPath);
    ~NetconfAccess() override;
    std::map<std::string, leaf_data_> getItems(const std::string& path) override;
    void setLeaf(const std::string& path, leaf_data_ value) override;
    void createPresenceContainer(const std::string& path) override;
    void deletePresenceContainer(const std::string& path) override;
    void createListInstance(const std::string& path) override;
    void deleteListInstance(const std::string& path) override;
    void commitChanges() override;
    void discardChanges() override;

    std::shared_ptr<Schema> schema();

private:
    std::string fetchSchema(const std::string_view module, const std::optional<std::string_view> revision, const std::optional<std::string_view>);
    std::vector<std::string> listImplementedSchemas();
    void datastoreInit();
    void doEditFromDataNode(std::shared_ptr<libyang::Data_Node> dataNode);

    std::unique_ptr<libnetconf::client::Session> m_session;
    std::shared_ptr<YangSchema> m_schema;
};
