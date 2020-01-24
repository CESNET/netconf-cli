/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/

#include <sysrepo-cpp/Session.hpp>
#include "sysrepo_subscription.hpp"


class MyCallback : public sysrepo::Callback {
public:
    MyCallback(const std::string& moduleName, Recorder* rec)
        : m_moduleName(moduleName)
        , m_recorder(rec)
    {
    }

    int module_change(sysrepo::S_Session sess, const char* module_name, sr_notif_event_t event, void*) override
    {
        using namespace std::string_literals;
        auto xpath = "/"s + module_name + ":*";
        auto it = sess->get_changes_iter(xpath.c_str());

        if (event == SR_EV_APPLY)
            return SR_ERR_OK;

        while (auto change = sess->get_change_next(it)) {
            auto xpath = change->new_val() ? change->new_val()->xpath() :
                         change->old_val() ? change->old_val()->xpath() :
                         "<no-xpath>";
            auto oldValue = change->old_val() ? std::optional<std::string>{change->old_val()->val_to_string()} : std::nullopt;
            auto newValue = change->new_val() ? std::optional<std::string>{change->new_val()->val_to_string()} : std::nullopt;
            m_recorder->write(xpath, oldValue, newValue);
        }

        return SR_ERR_OK;
    }

private:
    std::string m_moduleName;
    Recorder* m_recorder;
};

Recorder::~Recorder() = default;

SysrepoSubscription::SysrepoSubscription(Recorder* rec)
    : m_connection(new sysrepo::Connection("netconf-cli-test-subscription"))
{
    m_session = std::make_shared<sysrepo::Session>(m_connection);
    m_subscription = std::make_shared<sysrepo::Subscribe>(m_session);
    const char* modName = "example-schema";
    m_callback = std::make_shared<MyCallback>(modName, rec);

    m_subscription->module_change_subscribe(modName, m_callback);
}
