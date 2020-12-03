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

class Schema;
class YangSchema;

using LogCb = std::function<void(NC_VERB_LEVEL, const char*)>;

class NetconfAccess : public DatastoreAccess {
public:
    NetconfAccess(const std::string& hostname, uint16_t port, const std::string& user, const std::string& pubKey, const std::string& privKey);
    NetconfAccess(const std::string& socketPath);
    NetconfAccess(const int source, const int sink);
    NetconfAccess(std::unique_ptr<libnetconf::client::Session>&& session);
    ~NetconfAccess() override;
    [[nodiscard]] Tree getItems(const std::string& path) const override;

    static void setNcLogLevel(NC_VERB_LEVEL level);
    static void setNcLogCallback(const LogCb& callback);
    void setLeaf(const std::string& path, leaf_data_ value) override;
    void createItem(const std::string& path) override;
    void deleteItem(const std::string& path) override;
    void moveItem(const std::string& path, std::variant<yang::move::Absolute, yang::move::Relative> move) override;
    void commitChanges() override;
    void discardChanges() override;
    Tree execute(const std::string& path, const Tree& input) override;
    void copyConfig(const Datastore source, const Datastore destination) override;

    void setDatastore(const Datastore datastore);

    std::shared_ptr<Schema> schema() override;

    [[nodiscard]] std::string dump(const DataFormat format) const override;

private:
    std::vector<ListInstance> listInstances(const std::string& path) override;

    void doEditFromDataNode(std::shared_ptr<libyang::Data_Node> dataNode);

    std::unique_ptr<libnetconf::client::Session> m_session;
    std::shared_ptr<YangSchema> m_schema;
};
