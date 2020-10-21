/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/

#pragma once

#include <map>
#include <memory>
#include <optional>
#include <string>
#include "yang_operations.hpp"
#include "ast_values.hpp"
#include "list_instance.hpp"

/*! \class DatastoreAccess
 *     \brief Abstract class for accessing a datastore
 */

struct DatastoreError {
    std::string message;
    std::optional<std::string> xpath;

    DatastoreError(const std::string& message, const std::optional<std::string>& xpath = std::nullopt);
};

class DatastoreException : std::exception {
public:
    DatastoreException(const std::vector<DatastoreError>& errors);
    ~DatastoreException() override = default;
    [[nodiscard]] const char* what() const noexcept override;

private:
    std::string m_what;
};

class Schema;

struct dataPath_;

class DatastoreAccess {
public:
    using Tree = std::vector<std::pair<std::string, leaf_data_>>;
    virtual ~DatastoreAccess() = 0;
    [[nodiscard]] virtual Tree getItems(const std::string& path) const = 0;
    virtual void setLeaf(const std::string& path, leaf_data_ value) = 0;
    virtual void createItem(const std::string& path) = 0;
    virtual void deleteItem(const std::string& path) = 0;
    virtual void moveItem(const std::string& path, std::variant<yang::move::Absolute, yang::move::Relative> move) = 0;
    virtual Tree executeRpc(const std::string& path, const Tree& input) = 0;
    virtual Tree executeAction(const std::string& path, const Tree& input) = 0;

    virtual std::shared_ptr<Schema> schema() = 0;

    virtual void commitChanges() = 0;
    virtual void discardChanges() = 0;
    virtual void copyConfig(const Datastore source, const Datastore destination) = 0;
    [[nodiscard]] virtual std::string dump(const DataFormat format) const = 0;

private:
    friend class DataQuery;
    virtual std::vector<ListInstance> listInstances(const std::string& path) = 0;
};
