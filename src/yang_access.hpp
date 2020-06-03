/*
 * Copyright (C) 2020 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
*/

#pragma once

#include "datastore_access.hpp"

/*! \class YangAccess
 *     \brief Implementation of DatastoreAccess with a local libyang data node instance
 */

class YangSchema;

namespace libyang {
    class Data_Node;
}

class YangAccess : public DatastoreAccess {
public:
    YangAccess();
    ~YangAccess() override;
    Tree getItems(const std::string& path) override;
    void setLeaf(const std::string& path, leaf_data_ value) override;
    void createPresenceContainer(const std::string& path) override;
    void deletePresenceContainer(const std::string& path) override;
    void createListInstance(const std::string& path) override;
    void deleteListInstance(const std::string& path) override;
    void createLeafListInstance(const std::string& path) override;
    void deleteLeafListInstance(const std::string& path) override;
    void commitChanges() override;
    void discardChanges() override;
    Tree executeRpc(const std::string& path, const Tree& input) override;
    void copyConfig(const Datastore source, const Datastore destination) override;

    std::shared_ptr<Schema> schema() override;

private:
    std::vector<ListInstance> listInstances(const std::string& path) override;

    void mergeToStaging(const std::shared_ptr<libyang::Data_Node>& node);

    std::shared_ptr<YangSchema> m_schema;
    std::shared_ptr<libyang::Data_Node> m_datastore;
    std::shared_ptr<libyang::Data_Node> m_staging;
};
