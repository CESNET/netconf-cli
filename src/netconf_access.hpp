/*
 * Copyright (C) 2019 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
*/

#pragma once

#include <libyang-cpp/Context.hpp>
#include <libnetconf2-cpp/Enum.hpp>
#include <string>
#include "datastore_access.hpp"

/*! \class NetconfAccess
 *     \brief Implementation of DatastoreAccess for accessing a NETCONF server
 */

namespace libnetconf::client {
class Session;
}

class Schema;
class YangSchema;

using LogCb = std::function<void(libnetconf::LogLevel, const char*)>;

class NetconfAccess : public DatastoreAccess {
public:
    NetconfAccess(const std::string& socketPath);
    NetconfAccess(const int source, const int sink);
    NetconfAccess(std::unique_ptr<libnetconf::client::Session>&& session);
    ~NetconfAccess() override;
    [[nodiscard]] Tree getItems(const std::string& path) const override;

    static void setNcLogLevel(libnetconf::LogLevel level);
    static void setNcLogCallback(const LogCb& callback);
    void setLeaf(const std::string& path, leaf_data_ value) override;
    void createItem(const std::string& path) override;
    void deleteItem(const std::string& path) override;
    void moveItem(const std::string& path, std::variant<yang::move::Absolute, yang::move::Relative> move) override;
    void commitChanges() override;
    void discardChanges() override;
    Tree execute(const std::string& path, const Tree& input) override;
    void copyConfig(const Datastore source, const Datastore destination) override;

    std::shared_ptr<Schema> schema() override;

    [[nodiscard]] std::string dump(const DataFormat format) const override;

private:
    std::vector<ListInstance> listInstances(const std::string& path) override;

    void doEditFromDataNode(libyang::DataNode dataNode);

    void checkNMDA();

    bool m_serverHasNMDA;

    libyang::Context m_context;
    std::unique_ptr<libnetconf::client::Session> m_session;
    std::shared_ptr<YangSchema> m_schema;
};
