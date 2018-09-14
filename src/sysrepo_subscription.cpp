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
    MyCallback(const std::string& moduleName, Recorder* rec)
        : m_moduleName(moduleName)
        , m_recorder(rec)
    {}

    int module_change(S_Session sess, const char* module_name, sr_notif_event_t event, void*) override
    {
        using namespace std::string_literals;
        auto xpath = "/"s + module_name + ":*";
        auto it = sess->get_changes_iter(xpath.c_str());

        if (event == SR_EV_APPLY)
            return SR_ERR_OK;

        while (auto change = sess->get_change_next(it)) {
            m_recorder->write(change->new_val()->xpath(),
                              change->old_val() ? change->old_val()->val_to_string() : "",
                              change->new_val()->val_to_string());
        }

        return SR_ERR_OK;
    }

private:
    std::string m_moduleName;
    Recorder* m_recorder;
};

Recorder::~Recorder() = default;

SysrepoSubscription::SysrepoSubscription(Recorder* rec)
    : m_connection(new Connection("netconf-cli-test-subscription"))
{
    m_session = std::make_shared<Session>(m_connection);
    m_subscription = std::make_shared<Subscribe>(m_session);
    const char* modName = "example-schema";
    m_callback = std::make_shared<MyCallback>(modName, rec);

    m_subscription->module_change_subscribe(modName, m_callback);

}
