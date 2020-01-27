/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/

#pragma once

#include <string>
#include "datastore_access.hpp"

/*! \class DatastoreAccess
 *     \brief Abstract class for accessing a datastore
 */

namespace sysrepo {
class Connection;
class Session;
class Yang_Schema;
}

class Schema;
class YangSchema;

class SysrepoAccess : public DatastoreAccess {
public:
    ~SysrepoAccess() override;
    SysrepoAccess(const std::string& appname);
    Tree getItems(const std::string& path) override;
    void setLeaf(const std::string& path, leaf_data_ value) override;
    void createPresenceContainer(const std::string& path) override;
    void deletePresenceContainer(const std::string& path) override;
    void createListInstance(const std::string& path) override;
    void deleteListInstance(const std::string& path) override;
    Tree executeRpc(const std::string& path, const Tree& input) override;

    std::shared_ptr<Schema> schema() override;

    void commitChanges() override;
    void discardChanges() override;

private:
    [[noreturn]] void reportErrors();

    std::string fetchSchema(const char* module, const char* revision, const char* submodule);
    std::vector<std::shared_ptr<sysrepo::Yang_Schema>> listSchemas();

    std::shared_ptr<sysrepo::Connection> m_connection;
    std::shared_ptr<sysrepo::Session> m_session;
    std::shared_ptr<YangSchema> m_schema;
};
