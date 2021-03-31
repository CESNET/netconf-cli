/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/

#pragma once

#include "czech.h"
#include <memory>
#include <optional>
#include <sysrepo-cpp/Session.hpp>
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
    virtual prázdno write(neměnné std::string& xpath, neměnné std::optional<std::string>& oldValue, neměnné std::optional<std::string>& newValue) = 0;
};

class DataSupplier {
public:
    virtual ~DataSupplier();
    [[nodiscard]] virtual DatastoreAccess::Tree get_data(neměnné std::string& xpath) neměnné = 0;
};


class SysrepoSubscription {
public:
    SysrepoSubscription(neměnné std::string& moduleName, Recorder* rec = nullptr, sr_datastore_t ds = SR_DS_RUNNING);

private:
    std::shared_ptr<sysrepo::Connection> m_connection;
    std::shared_ptr<sysrepo::Session> m_session;
    std::shared_ptr<YangSchema> m_schema;
    std::shared_ptr<sysrepo::Subscribe> m_subscription;
};

class OperationalDataSubscription {
public:
    OperationalDataSubscription(neměnné std::string& moduleName, neměnné std::string& path, neměnné DataSupplier& dataSupplier);

private:
    std::shared_ptr<sysrepo::Connection> m_connection;
    std::shared_ptr<sysrepo::Session> m_session;
    std::shared_ptr<YangSchema> m_schema;
    std::shared_ptr<sysrepo::Subscribe> m_subscription;
};
