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

struct ly_ctx;
struct lyd_node;

class YangAccess : public DatastoreAccess {
public:
    YangAccess();
    ~YangAccess() override;
    Tree getItems(const std::string& path) override;
    void setLeaf(const std::string& path, leaf_data_ value) override;
    void createItem(const std::string& path) override;
    void deleteItem(const std::string& path) override;
    void moveItem(const std::string& source, std::variant<yang::move::Absolute, yang::move::Relative> move) override;
    void commitChanges() override;
    void discardChanges() override;
    Tree executeRpc(const std::string& path, const Tree& input) override;
    void copyConfig(const Datastore source, const Datastore destination) override;

    std::shared_ptr<Schema> schema() override;

    void enableFeature(const std::string& module, const std::string& feature);
    std::string dumpXML() const;
    std::string dumpJSON() const;

    void addSchemaFile(const std::string& path);
    void addSchemaDir(const std::string& path);

private:
    std::vector<ListInstance> listInstances(const std::string& path) override;

    void getErrorsAndThrow() const;
    void impl_newPath(const std::string& path, const std::optional<std::string>& value = std::nullopt);
    void impl_removeNode(const std::string& path);
    void validate();

    std::unique_ptr<ly_ctx, void(*)(ly_ctx*)> m_ctx;
    std::unique_ptr<lyd_node, void(*)(lyd_node*)> m_datastore;
    std::shared_ptr<YangSchema> m_schema;
};
