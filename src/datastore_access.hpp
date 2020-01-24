/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/

#pragma once

#include <map>
#include <optional>
#include <string>
#include "ast_values.hpp"

/*! \class DatastoreAccess
 *     \brief Abstract class for accessing a datastore
 */

struct DatastoreError {
    std::string message;
    std::optional<std::string> xpath;

    DatastoreError(const std::string& message, const std::optional<std::string>& xpath);
};

class DatastoreException : std::exception {
public:
    DatastoreException(const std::vector<DatastoreError>& errors);
    ~DatastoreException() override = default;
    const char* what() const noexcept override;

private:
    std::string m_what;
};

class Schema;

class DatastoreAccess {
public:
    using Tree = std::map<std::string, leaf_data_>;
    virtual ~DatastoreAccess() = 0;
    virtual Tree getItems(const std::string& path) = 0;
    virtual void setLeaf(const std::string& path, leaf_data_ value) = 0;
    virtual void createPresenceContainer(const std::string& path) = 0;
    virtual void deletePresenceContainer(const std::string& path) = 0;
    virtual void createListInstance(const std::string& path) = 0;
    virtual void deleteListInstance(const std::string& path) = 0;
    virtual Tree executeRpc(const std::string& path, const Tree& input) = 0;

    virtual std::shared_ptr<Schema> schema() = 0;

    virtual void commitChanges() = 0;
    virtual void discardChanges() = 0;
};
