/*
 * Copyright (C) 2020 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
*/

#pragma once
#include "datastore_access.hpp"

/*! \class ProxyDatastore
 *     \brief DatastoreAccess wrapper that handles RPC input
 */
class ProxyDatastore : DatastoreAccess {
public:
    ProxyDatastore(DatastoreAccess& datastore);
    ~ProxyDatastore();
    Tree getItems(const std::string& path) const override;
    void setLeaf(const std::string& path, leaf_data_ value) override;
    void createItem(const std::string& path) override;
    void deleteItem(const std::string& path) override;
    void moveItem(const std::string& source, std::variant<yang::move::Absolute, yang::move::Relative> move) override;
    void commitChanges() override;
    void discardChanges() override;
    Tree executeRpc(const std::string& path, const Tree& input) override;
    void copyConfig(const Datastore source, const Datastore destination) override;
    std::string dump(const DataFormat format) const override;

    std::shared_ptr<Schema> schema() override;
private:
    std::vector<ListInstance> listInstances(const std::string& path) override;

    DatastoreAccess& m_datastore;
};
