/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/

#pragma once

#include <memory>

class Callback;
class Connection;
class Session;
class Subscribe;
class YangSchema;

class Recorder {
public:
    virtual ~Recorder();
    virtual void write(const std::string& xpath, const std::string& oldValue, const std::string& newValue) = 0;
};

class SysrepoSubscription {
public:
    SysrepoSubscription(Recorder* rec);

private:
    std::shared_ptr<Connection> m_connection;
    std::shared_ptr<Session> m_session;
    std::shared_ptr<YangSchema> m_schema;
    std::shared_ptr<Callback> m_callback;
    std::shared_ptr<Subscribe> m_subscription;

    Recorder* m_rec;
};
