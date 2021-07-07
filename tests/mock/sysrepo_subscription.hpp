/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/

#pragma once

#include <memory>
#include <optional>
#include <sysrepo-cpp/Connection.hpp>
#include "datastore_access.hpp"

class YangSchema;

class Recorder {
public:
    virtual ~Recorder();
    virtual void write(const sysrepo::ChangeOperation operation, const std::string& xpath, const std::optional<std::string>& oldValue, const std::optional<std::string>& newValue, const std::optional<std::string> previousList) = 0;
};

class DataSupplier {
public:
    virtual ~DataSupplier();
    [[nodiscard]] virtual DatastoreAccess::Tree get_data(const std::string& xpath) const = 0;
};


class SysrepoSubscription {
public:
    SysrepoSubscription(const std::string& moduleName, Recorder* rec = nullptr, sysrepo::Datastore ds = sysrepo::Datastore::Running);

private:
    sysrepo::Subscription m_subscription;
};

class OperationalDataSubscription {
public:
    OperationalDataSubscription(const std::string& moduleName, const std::string& path, const DataSupplier& dataSupplier);

private:
    std::shared_ptr<YangSchema> m_schema;
    sysrepo::Subscription m_subscription;
};
