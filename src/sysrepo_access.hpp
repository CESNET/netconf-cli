/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/

#pragma once

#include <string>
#include "ast_commands.hpp"
#include "datastore_access.hpp"

/*! \class DatastoreAccess
 *     \brief Abstract class for accessing a datastore
 */

class Connection;
class Session;

class SysrepoAccess : public DatastoreAccess {
public:
    ~SysrepoAccess() override;
    SysrepoAccess(const std::string& appname);
    std::map<std::string, leaf_data_> getItems(const std::string& path) override;
    void setLeaf(const std::string& path, leaf_data_ value) override;
    leaf_data_ getLeaf(const std::string& path) override;
    void createPresenceContainer(const std::string& path) override;
    void deletePresenceContainer(const std::string& path) override;

private:
    std::shared_ptr<Connection> m_connection;
    std::shared_ptr<Session> m_session;
};
