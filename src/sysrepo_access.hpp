/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/

#pragma once

#include <string>
#include <sysrepo-cpp/Connection.hpp>
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
    SysrepoAccess();
    [[nodiscard]] Tree getItems(const std::string& path) const override;
    void setLeaf(const std::string& path, leaf_data_ value) override;
    void createItem(const std::string& path) override;
    void deleteItem(const std::string& path) override;
    void moveItem(const std::string& source, std::variant<yang::move::Absolute, yang::move::Relative> move) override;
    Tree execute(const std::string& path, const Tree& input) override;

    std::shared_ptr<Schema> schema() override;

    ChangeTree pendingChanges() const override;
    void commitChanges() override;
    void discardChanges() override;
    void copyConfig(const Datastore source, const Datastore destination) override;

    [[nodiscard]] std::string dump(const DataFormat format) const override;

private:
    std::vector<ListInstance> listInstances(const std::string& path) override;
    [[noreturn]] void reportErrors() const;

    sysrepo::Connection m_connection;
    sysrepo::Session m_session;
    std::shared_ptr<YangSchema> m_schema;
};
