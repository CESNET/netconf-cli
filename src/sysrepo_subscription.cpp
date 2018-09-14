/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/

#include <sysrepo-cpp/Session.h>
#include "sysrepo_subscription.hpp"

leaf_data_ leafValueFromVal(const S_Val& value)
{
    using namespace std::string_literals;
    switch (value->type()) {
    case SR_INT32_T:
        return value->data()->get_int32();
    case SR_UINT32_T:
        return value->data()->get_uint32();
    case SR_BOOL_T:
        return value->data()->get_bool();
    case SR_STRING_T:
        return std::string(value->data()->get_string());
    case SR_ENUM_T:
        return std::string(value->data()->get_enum());
    case SR_DECIMAL64_T:
        return value->data()->get_decimal64();
    case SR_CONTAINER_T:
        return "(container)"s;
    case SR_CONTAINER_PRESENCE_T:
        return "(presence container)"s;
    case SR_LIST_T:
        return "(list)"s;
    default: // TODO: implement all types
        return value->val_to_string();
    }
}

class MyCallback : public Callback {
public:
    MyCallback(const std::string& moduleName, const std::shared_ptr<Recorder> rec)
        : m_moduleName(moduleName)
        , m_recorder(rec)
    {}

    int module_change(S_Session sess, const char* module_name, sr_notif_event_t event, void*) override
    {
        using namespace std::string_literals;
        auto xpath = "/"s + module_name + ":*";
        auto it = sess->get_changes_iter(xpath.c_str());

        while (auto change = sess->get_change_next(it)) {
            m_recorder->write(change->old_val()->xpath(),
                              leafValueFromVal(change->old_val()),
                              leafValueFromVal(change->new_val()));
        }

        return SR_ERR_OK;
    }

private:
    std::string m_moduleName;
    std::shared_ptr<Recorder> m_recorder;
};

Recorder::~Recorder() = default;

SysrepoSubscription::SysrepoSubscription(const std::shared_ptr<Recorder>& rec)
    : m_connection(new Connection("test"))
{
    m_session = std::make_shared<Session>(m_connection);
    m_subscription = std::make_shared<Subscribe>(m_session);
    const char* modName = "test_module";
    m_callback = std::make_shared<MyCallback>(modName, rec);

    m_subscription->module_change_subscribe(modName, m_callback);

}
