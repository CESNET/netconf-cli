/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/

#include <sysrepo-cpp/Session.h>
#include "sysrepo_subscription.hpp"

class MyCallback : public Callback {
public:
    MyCallback(const std::string& moduleName)
        : m_moduleName(moduleName) {}

    int module_change(S_Session sess, const char *module_name, sr_notif_event_t event, void *private_ctx)
    {

    }

    private:
    std::string m_moduleName;
};

SysrepoSubscription::SysrepoSubscription()
    : m_connection(new Connection("test"))
{
    m_session = std::make_shared<Session>(m_connection);
    m_subscription = std::make_shared<Subscribe>(m_session);
    const char* modName = "test_module";
    m_callback = std::make_shared<MyCallback>(modName);

    m_subscription->module_change_subscribe(modName, m_callback);

}
