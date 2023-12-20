/*
 * Copyright (C) 2020 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
*/

#pragma once

#include <libyang-cpp/Context.hpp>
#include "datastore_access.hpp"

/*! \class YangAccess
 *     \brief Implementation of DatastoreAccess with a local libyang data node instance
 */

enum class StrictDataParsing {
    Yes,
    No
};

class YangSchema;

struct ly_ctx;
struct lyd_node;

class YangAccess : public DatastoreAccess {
public:
    YangAccess();
    YangAccess(std::shared_ptr<YangSchema> schema);
    ~YangAccess() override;
    [[nodiscard]] Tree getItems(const std::string& path) const override;
    void setLeaf(const std::string& path, leaf_data_ value) override;
    void createItem(const std::string& path) override;
    void deleteItem(const std::string& path) override;
    void moveItem(const std::string& source, std::variant<yang::move::Absolute, yang::move::Relative> move) override;
    void commitChanges() override;
    void discardChanges() override;
    [[noreturn]] Tree execute(const std::string& path, const Tree& input) override;
    void copyConfig(const Datastore source, const Datastore destination) override;

    std::shared_ptr<Schema> schema() override;

    void setEnabledFeatures(const std::string& module, const std::vector<std::string>& features);
    [[nodiscard]] std::string dump(const DataFormat format) const override;

    void loadModule(const std::string& name);
    void addSchemaFile(const std::filesystem::path& path);
    void addSchemaDir(const std::filesystem::path& path);
    void addDataFile(const std::string& path, const StrictDataParsing strict);

private:
    std::vector<ListInstance> listInstances(const std::string& path) override;
    [[noreturn]] void impl_execute(const std::string& type, const std::string& path, const Tree& input);

    [[noreturn]] void getErrorsAndThrow() const;
    void impl_newPath(const std::string& path, const std::optional<std::string>& value = std::nullopt);
    void impl_removeNode(const std::string& path);
    void validate();

    libyang::Context m_ctx;
    std::optional<libyang::DataNode> m_datastore;
    std::shared_ptr<YangSchema> m_schema;
};
