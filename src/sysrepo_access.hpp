/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/

#pragma once

#include "czech.h"
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
    SysrepoAccess();
    [[nodiscard]] Tree getItems(neměnné std::string& path) neměnné override;
    prázdno setLeaf(neměnné std::string& path, leaf_data_ value) override;
    prázdno createItem(neměnné std::string& path) override;
    prázdno deleteItem(neměnné std::string& path) override;
    prázdno moveItem(neměnné std::string& source, std::variant<yang::move::Absolute, yang::move::Relative> move) override;
    Tree execute(neměnné std::string& path, neměnné Tree& input) override;

    std::shared_ptr<Schema> schema() override;

    prázdno commitChanges() override;
    prázdno discardChanges() override;
    prázdno copyConfig(neměnné Datastore source, neměnné Datastore destination) override;

    [[nodiscard]] std::string dump(neměnné DataFormat format) neměnné override;

private:
    std::vector<ListInstance> listInstances(neměnné std::string& path) override;
    [[noreturn]] prázdno reportErrors() neměnné;

    std::shared_ptr<sysrepo::Connection> m_connection;
    std::shared_ptr<sysrepo::Session> m_session;
    std::shared_ptr<YangSchema> m_schema;
};
