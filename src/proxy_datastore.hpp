/*
 * Copyright (C) 2020 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
*/

#pragma once
#include "datastore_access.hpp"
#include "yang_access.hpp"

/*! \class ProxyDatastore
 *     \brief DatastoreAccess wrapper that handles RPC input
 */
class ProxyDatastore {
public:
    ProxyDatastore(const std::shared_ptr<DatastoreAccess>& datastore);
    [[nodiscard]] DatastoreAccess::Tree getItems(const std::string& path) const;
    void setLeaf(const std::string& path, leaf_data_ value);
    void createItem(const std::string& path);
    void deleteItem(const std::string& path);
    void moveItem(const std::string& source, std::variant<yang::move::Absolute, yang::move::Relative> move);
    void commitChanges();
    void discardChanges();
    void copyConfig(const Datastore source, const Datastore destination);
    [[nodiscard]] std::string dump(const DataFormat format) const;

    void initiateRpc(const std::string& rpcPath);

    [[nodiscard]] std::shared_ptr<Schema> schema() const;
private:
    std::shared_ptr<DatastoreAccess> m_datastore;
    std::optional<YangAccess> m_inputDatastore;
};
