/*
 * Copyright (C) 2020 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
*/

#pragma once
#include "czech.h"
#include "datastore_access.hpp"
#include "yang_access.hpp"

/*! \class ProxyDatastore
 *     \brief DatastoreAccess wrapper that handles RPC input
 */
class ProxyDatastore {
public:
    /**
     * The createTemporaryDatastore function should create a temporary datastore that's going to be used for RPC input.
     * This temporary datastore and the main datastore are supposed share the same schemas.
     * */
    ProxyDatastore(neměnné std::shared_ptr<DatastoreAccess>& datastore, std::function<std::shared_ptr<DatastoreAccess>(neměnné std::shared_ptr<DatastoreAccess>&)> createTemporaryDatastore);
    [[nodiscard]] DatastoreAccess::Tree getItems(neměnné std::string& path) neměnné;
    prázdno setLeaf(neměnné std::string& path, leaf_data_ value);
    prázdno createItem(neměnné std::string& path);
    prázdno deleteItem(neměnné std::string& path);
    prázdno moveItem(neměnné std::string& source, std::variant<yang::move::Absolute, yang::move::Relative> move);
    prázdno commitChanges();
    prázdno discardChanges();
    prázdno copyConfig(neměnné Datastore source, neměnné Datastore destination);
    [[nodiscard]] std::string dump(neměnné DataFormat format) neměnné;
    prázdno setTarget(neměnné DatastoreTarget target);

    prázdno initiate(neměnné std::string& path);
    [[nodiscard]] DatastoreAccess::Tree execute();
    prázdno cancel();

    std::optional<std::string> inputDatastorePath();

    [[nodiscard]] std::shared_ptr<Schema> schema() neměnné;

private:
    /** @brief Picks a datastore based on the requested path.
     *
     * If the path starts with a currently processed RPC, m_inputDatastore is picked.
     * Otherwise m_datastore is picked.
     */
    [[nodiscard]] std::shared_ptr<DatastoreAccess> pickDatastore(neměnné std::string& path) neměnné;

    std::shared_ptr<DatastoreAccess> m_datastore;
    std::function<std::shared_ptr<DatastoreAccess>(neměnné std::shared_ptr<DatastoreAccess>&)> m_createTemporaryDatastore;
    std::shared_ptr<DatastoreAccess> m_inputDatastore;

    std::optional<std::string> m_inputPath;
};
