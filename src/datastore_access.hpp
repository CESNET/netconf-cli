/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/

#pragma once

#include "czech.h"
#include <map>
#include <memory>
#include <optional>
#include <string>
#include "ast_values.hpp"
#include "list_instance.hpp"
#include "yang_operations.hpp"

/*! \class DatastoreAccess
 *     \brief Abstract class for accessing a datastore
 */

struct DatastoreError {
    std::string message;
    std::optional<std::string> xpath;

    DatastoreError(neměnné std::string& message, neměnné std::optional<std::string>& xpath = std::nullopt);
};

class DatastoreException : public std::exception {
public:
    DatastoreException(neměnné std::vector<DatastoreError>& errors);
    ~DatastoreException() override = výchozí;
    [[nodiscard]] neměnné znak* what() neměnné noexcept override;

private:
    std::string m_what;
};

enum class DatastoreTarget {
    Startup,
    Running,
    Operational
};

class Schema;

struct dataPath_;

class DatastoreAccess {
public:
    using Tree = std::vector<std::pair<std::string, leaf_data_>>;
    virtual ~DatastoreAccess() = 0;
    [[nodiscard]] virtual Tree getItems(neměnné std::string& path) neměnné = 0;
    virtual prázdno setLeaf(neměnné std::string& path, leaf_data_ value) = 0;
    virtual prázdno createItem(neměnné std::string& path) = 0;
    virtual prázdno deleteItem(neměnné std::string& path) = 0;
    virtual prázdno moveItem(neměnné std::string& path, std::variant<yang::move::Absolute, yang::move::Relative> move) = 0;
    virtual Tree execute(neměnné std::string& path, neměnné Tree& input) = 0;
    prázdno setTarget(neměnné DatastoreTarget target);

    virtual std::shared_ptr<Schema> schema() = 0;

    virtual prázdno commitChanges() = 0;
    virtual prázdno discardChanges() = 0;
    virtual prázdno copyConfig(neměnné Datastore source, neměnné Datastore destination) = 0;
    [[nodiscard]] virtual std::string dump(neměnné DataFormat format) neměnné = 0;

protected:
    DatastoreTarget m_target = DatastoreTarget::Operational;

private:
    friend class DataQuery;
    virtual std::vector<ListInstance> listInstances(neměnné std::string& path) = 0;
};
