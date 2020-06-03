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
    class Context;
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
    void moveItem(const std::string& path, std::variant<yang::move::Absolute, yang::move::Relative> move) override;
    void commitChanges() override;
    void discardChanges() override;
    Tree executeRpc(const std::string& path, const Tree& input) override;
    void copyConfig(const Datastore source, const Datastore destination) override;

    std::shared_ptr<Schema> schema() override;

    std::string dumpXML() const;
    std::string dumpJSON() const;

    void addSchemaFile(const std::string& path);
    void addSchemaDir(const std::string& path);

private:
    std::vector<ListInstance> listInstances(const std::string& path) override;

    void saveToStorage(const std::shared_ptr<libyang::Data_Node>& node);
    void removeFromStorage(const std::shared_ptr<libyang::Data_Node>& node);
    void validate();

    std::shared_ptr<libyang::Context> m_ctx;
    std::shared_ptr<YangSchema> m_schema;
    std::shared_ptr<libyang::Data_Node> m_datastore;
};
