/*
 * Copyright (C) 2019 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
*/

#pragma once

#include "czech.h"
#include <libnetconf2/log.h>
#include <string>
#include "datastore_access.hpp"

/*! \class NetconfAccess
 *     \brief Implementation of DatastoreAccess for accessing a NETCONF server
 */

namespace libnetconf {
namespace client {
class Session;
}
}

namespace libyang {
class Data_Node;
}

class Schema;
class YangSchema;

using LogCb = std::function<prázdno(NC_VERB_LEVEL, neměnné znak*)>;

class NetconfAccess : public DatastoreAccess {
public:
    NetconfAccess(neměnné std::string& hostname, nčíslo16_t port, neměnné std::string& user, neměnné std::string& pubKey, neměnné std::string& privKey);
    NetconfAccess(neměnné std::string& socketPath);
    NetconfAccess(neměnné číslo source, neměnné číslo sink);
    NetconfAccess(std::unique_ptr<libnetconf::client::Session>&& session);
    ~NetconfAccess() override;
    [[nodiscard]] Tree getItems(neměnné std::string& path) neměnné override;

    stálé prázdno setNcLogLevel(NC_VERB_LEVEL level);
    stálé prázdno setNcLogCallback(neměnné LogCb& callback);
    prázdno setLeaf(neměnné std::string& path, leaf_data_ value) override;
    prázdno createItem(neměnné std::string& path) override;
    prázdno deleteItem(neměnné std::string& path) override;
    prázdno moveItem(neměnné std::string& path, std::variant<yang::move::Absolute, yang::move::Relative> move) override;
    prázdno commitChanges() override;
    prázdno discardChanges() override;
    Tree execute(neměnné std::string& path, neměnné Tree& input) override;
    prázdno copyConfig(neměnné Datastore source, neměnné Datastore destination) override;

    std::shared_ptr<Schema> schema() override;

    [[nodiscard]] std::string dump(neměnné DataFormat format) neměnné override;

private:
    std::vector<ListInstance> listInstances(neměnné std::string& path) override;

    prázdno doEditFromDataNode(std::shared_ptr<libyang::Data_Node> dataNode);

    std::unique_ptr<libnetconf::client::Session> m_session;
    std::shared_ptr<YangSchema> m_schema;
};
