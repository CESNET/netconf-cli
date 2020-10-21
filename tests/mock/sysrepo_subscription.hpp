/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/

#pragma once

#include <optional>
#include <memory>
#include "datastore_access.hpp"

namespace sysrepo {
class Callback;
class Connection;
class Session;
class Subscribe;
}
class YangSchema;

class Recorder {
public:
    virtual ~Recorder();
    virtual void write(const std::string& xpath, const std::optional<std::string>& oldValue, const std::optional<std::string>& newValue) = 0;
};

class DataSupplier {
public:
    virtual ~DataSupplier();
    [[nodiscard]] virtual DatastoreAccess::Tree get_data(const std::string& xpath) const = 0;
};


class SysrepoSubscription {
public:
    SysrepoSubscription(const std::string& moduleName, Recorder* rec = nullptr);

private:
    std::shared_ptr<sysrepo::Connection> m_connection;
    std::shared_ptr<sysrepo::Session> m_session;
    std::shared_ptr<YangSchema> m_schema;
    std::shared_ptr<sysrepo::Subscribe> m_subscription;
};

class OperationalDataSubscription {
public:
    OperationalDataSubscription(const std::string& moduleName, const std::string& path, const DataSupplier& dataSupplier);
private:
    std::shared_ptr<sysrepo::Connection> m_connection;
    std::shared_ptr<sysrepo::Session> m_session;
    std::shared_ptr<YangSchema> m_schema;
    std::shared_ptr<sysrepo::Subscribe> m_subscription;
};
