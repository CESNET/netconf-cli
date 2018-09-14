/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/

#pragma once

#include <memory>
#include "ast_values.hpp"

class Callback;
class Connection;
class Session;
class Subscribe;
class YangSchema;

class Recorder {
public:
    virtual ~Recorder();
    virtual void write(const std::string& xpath, leaf_data_ oldValue, leaf_data_ newValue) = 0;
};

class SysrepoSubscription {
public:
    SysrepoSubscription(const std::shared_ptr<Recorder>& rec);

private:
    std::shared_ptr<Connection> m_connection;
    std::shared_ptr<Session> m_session;
    std::shared_ptr<YangSchema> m_schema;
    std::shared_ptr<Callback> m_callback;
    std::shared_ptr<Subscribe> m_subscription;

    std::shared_ptr<Recorder> m_rec;
};
