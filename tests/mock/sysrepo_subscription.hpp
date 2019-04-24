/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/

#pragma once

#include <memory>

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
    virtual void write(const std::string& xpath, const std::string& oldValue, const std::string& newValue) = 0;
};

class SysrepoSubscription {
public:
    SysrepoSubscription(Recorder* rec);
    ~SysrepoSubscription();

private:
    std::shared_ptr<sysrepo::Connection> m_connection;
    std::shared_ptr<sysrepo::Session> m_session;
    std::shared_ptr<YangSchema> m_schema;
    std::shared_ptr<sysrepo::Callback> m_callback;
    std::shared_ptr<sysrepo::Subscribe> m_subscription;
};
