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

        if (event == SR_EV_APPLY) {
            return SR_ERR_OK;
        }

        while (auto change = sess->get_change_next(it)) {
            auto xpath = (change->new_val() ? change->new_val() : change->old_val())->xpath();

            auto oldValue = change->old_val() ? std::optional{change->old_val()->val_to_string()} : std::nullopt;
            auto newValue = change->new_val() ? std::optional{change->new_val()->val_to_string()} : std::nullopt;
            m_recorder->write(xpath, oldValue, newValue);
        }

        return SR_ERR_OK;
    }

private:
    std::string m_moduleName;
    Recorder* m_recorder;
};

Recorder::~Recorder() = default;

DataSupplier::~DataSupplier() = default;

SysrepoSubscription::SysrepoSubscription(const std::string& moduleName, Recorder* rec)
    : m_connection(new sysrepo::Connection("netconf-cli-test-subscription"))
{
    m_session = std::make_shared<sysrepo::Session>(m_connection);
    m_subscription = std::make_shared<sysrepo::Subscribe>(m_session);
    if (rec) {
        m_callback = std::make_shared<MyCallback>(moduleName, rec);
    } else {
        m_callback = std::make_shared<sysrepo::Callback>();
    }

    m_subscription->module_change_subscribe(moduleName.c_str(), m_callback);
}

struct leafDataToSysrepoVal {
    leafDataToSysrepoVal (sysrepo::S_Val v, const std::string& xpath)
        : v(v)
        , xpath(xpath)
    {
    }

    void operator()(const binary_& what)
    {
        v->set(xpath.c_str(), what.m_value.c_str(), SR_BINARY_T);
    }

    void operator()(const enum_& what)
    {
        v->set(xpath.c_str(), what.m_value.c_str(), SR_ENUM_T);
    }

    void operator()(const identityRef_& what)
    {
        v->set(xpath.c_str(), (what.m_prefix->m_name + what.m_value).c_str(), SR_IDENTITYREF_T);
    }

    void operator()(const empty_)
    {
        v->set(xpath.c_str(), nullptr, SR_LEAF_EMPTY_T);
    }

    void operator()(const std::string& what)
    {
        v->set(xpath.c_str(), what.c_str());
    }

    template <typename Type>
    void operator()(const Type what)
    {
        v->set(xpath.c_str(), what);
    }

    void operator()([[maybe_unused]] const special_ what)
    {
        throw std::logic_error("Attempted to create a SR val from a special_ value");
    }

    ::sysrepo::S_Val v;
    std::string xpath;
};

class OperationalDataCallback : public sysrepo::Callback {
public:
    OperationalDataCallback(const DataSupplier& dataSupplier)
        : m_dataSupplier(dataSupplier)
    {
    }
    int dp_get_items(const char *xpath, sysrepo::S_Vals_Holder vals, [[maybe_unused]] uint64_t request_id, [[maybe_unused]] const char *original_xpath, [[maybe_unused]] void *private_ctx) override
    {
        auto data = m_dataSupplier.get_data(xpath);
        auto out = vals->allocate(data.size());
        size_t i = 0;
        for (auto it = data.cbegin(); it != data.cend(); ++it, ++i) {
            std::string valuePath = it->first;
            boost::apply_visitor(leafDataToSysrepoVal(out->val(i), valuePath), it->second);
        }
        return SR_ERR_OK;
    }
private:
    const DataSupplier& m_dataSupplier;
};

OperationalDataSubscription::OperationalDataSubscription(const std::string& moduleName, const DataSupplier& dataSupplier)
    : m_connection(new sysrepo::Connection("netconf-cli-test-subscription"))
    , m_session(std::make_shared<sysrepo::Session>(m_connection))
    , m_subscription(std::make_shared<sysrepo::Subscribe>(m_session))
    , m_callback(std::make_shared<OperationalDataCallback>(dataSupplier))
{
    m_subscription->dp_get_items_subscribe(moduleName.c_str(), m_callback);
}
