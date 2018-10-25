/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/

#pragma once

#include <string>
#include "ast_values.hpp"

/*! \class DatastoreAccess
 *     \brief Abstract class for accessing a datastore
 */


class DatastoreAccess {
public:
    virtual ~DatastoreAccess() = 0;
    virtual std::map<std::string, leaf_data_> getItems(const std::string& path) = 0;
    virtual void setLeaf(const std::string& path, leaf_data_ value) = 0;
    virtual void createPresenceContainer(const std::string& path) = 0;
    virtual void deletePresenceContainer(const std::string& path) = 0;

    virtual void commitChanges() = 0;
    virtual void discardChanges() = 0;
};
