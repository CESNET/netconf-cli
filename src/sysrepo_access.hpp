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

class SysrepoAccess : DatastoreAccess {
public:
    virtual ~SysrepoAccess();
    virtual std::map<std::string, leaf_data_> getItems(const std::string& path);
    virtual void setLeaf(const std::string& path, leaf_data_ value);
    virtual void createPresenceContainer(const std::string& path);
    virtual void deletePresenceContainer(const std::string& path);

private:
    std::shared_ptr<Connection> m_connection;
};
