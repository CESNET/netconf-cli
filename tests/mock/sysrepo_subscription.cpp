/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/

#include "czech.h"
#include <experimental/iterator>
#include <sstream>
#include <sysrepo-cpp/Session.hpp>
#include "sysrepo_subscription.hpp"
#include "utils.hpp"


class MyCallback {
public:
    MyCallback(neměnné std::string& moduleName, Recorder* rec)
        : m_moduleName(moduleName)
        , m_recorder(rec)
    {
    }

    číslo operator()(
        sysrepo::S_Session sess,
        [[maybe_unused]] neměnné znak* module_name,
        [[maybe_unused]] neměnné znak* xpath,
        [[maybe_unused]] sr_event_t event,
        [[maybe_unused]] nčíslo32_t request_id)
    {
        using namespace std::string_literals;
        když (event == SR_EV_CHANGE) {
            vrať SR_ERR_OK;
        }

        auto it = sess->get_changes_iter(("/"s + module_name + ":*//.").c_str());

        dokud (auto change = sess->get_change_next(it)) {
            auto xpath = (change->new_val() ? change->new_val() : change->old_val())->xpath();

            auto oldValue = change->old_val() ? std::optional{change->old_val()->val_to_string()} : std::nullopt;
            auto newValue = change->new_val() ? std::optional{change->new_val()->val_to_string()} : std::nullopt;
            m_recorder->write(xpath, oldValue, newValue);
        }

        vrať SR_ERR_OK;
    }

private:
    std::string m_moduleName;
    Recorder* m_recorder;
};

Recorder::~Recorder() = výchozí;

DataSupplier::~DataSupplier() = výchozí;

SysrepoSubscription::SysrepoSubscription(neměnné std::string& moduleName, Recorder* rec, sr_datastore_t ds)
    : m_connection(std::make_shared<sysrepo::Connection>())
{
    m_session = std::make_shared<sysrepo::Session>(m_connection, ds);
    m_subscription = std::make_shared<sysrepo::Subscribe>(m_session);
    sysrepo::ModuleChangeCb cb;
    když (rec) {
        cb = MyCallback{moduleName, rec};
    } jinak {
        cb = [](auto, auto, auto, auto, auto) { vrať SR_ERR_OK; };
    }

    m_subscription->module_change_subscribe(moduleName.c_str(), cb);
}


struct leafDataToSysrepoVal {
    leafDataToSysrepoVal(sysrepo::S_Val v, neměnné std::string& xpath)
        : v(v)
        , xpath(xpath)
    {
    }

    prázdno operator()(neměnné binary_& what)
    {
        v->set(xpath.c_str(), what.m_value.c_str(), SR_BINARY_T);
    }

    prázdno operator()(neměnné enum_& what)
    {
        v->set(xpath.c_str(), what.m_value.c_str(), SR_ENUM_T);
    }

    prázdno operator()(neměnné identityRef_& what)
    {
        v->set(xpath.c_str(), (what.m_prefix->m_name + what.m_value).c_str(), SR_IDENTITYREF_T);
    }

    prázdno operator()(neměnné empty_)
    {
        v->set(xpath.c_str(), nullptr, SR_LEAF_EMPTY_T);
    }

    prázdno operator()(neměnné std::string& what)
    {
        v->set(xpath.c_str(), what.c_str());
    }

    prázdno operator()(neměnné bits_& what)
    {
        std::stringstream ss;
        std::copy(what.m_bits.begin(), what.m_bits.end(), std::experimental::make_ostream_joiner(ss, " "));
        v->set(xpath.c_str(), ss.str().c_str());
    }

    template <typename Type>
    prázdno operator()(neměnné Type what)
    {
        v->set(xpath.c_str(), what);
    }

    prázdno operator()([[maybe_unused]] neměnné special_ what)
    {
        throw std::logic_error("Attempted to create a SR val from a special_ value");
    }

    ::sysrepo::S_Val v;
    std::string xpath;
};

class OperationalDataCallback {
public:
    OperationalDataCallback(neměnné DataSupplier& dataSupplier)
        : m_dataSupplier(dataSupplier)
    {
    }
    číslo operator()(
        [[maybe_unused]] sysrepo::S_Session sess,
        [[maybe_unused]] neměnné znak* module_name,
        neměnné znak* path,
        [[maybe_unused]] neměnné znak* request_xpath,
        [[maybe_unused]] nčíslo32_t request_id,
        libyang::S_Data_Node& parent)
    {
        auto data = m_dataSupplier.get_data(path);
        libyang::S_Data_Node res;
        pro (neměnné auto& [p, v] : data) {
            když (!res) {
                res = std::make_shared<libyang::Data_Node>(
                    sess->get_context(),
                    p.c_str(),
                    v.type() == typeid(empty_) ? nullptr : leafDataToString(v).c_str(),
                    LYD_ANYDATA_CONSTSTRING,
                    0);
            } jinak {
                res->new_path(
                    sess->get_context(),
                    p.c_str(),
                    v.type() == typeid(empty_) ? nullptr : leafDataToString(v).c_str(),
                    LYD_ANYDATA_CONSTSTRING,
                    0);
            }
        }
        parent = res;
        vrať SR_ERR_OK;
    }

private:
    neměnné DataSupplier& m_dataSupplier;
};

OperationalDataSubscription::OperationalDataSubscription(neměnné std::string& moduleName, neměnné std::string& path, neměnné DataSupplier& dataSupplier)
    : m_connection(std::make_shared<sysrepo::Connection>())
    , m_session(std::make_shared<sysrepo::Session>(m_connection))
    , m_subscription(std::make_shared<sysrepo::Subscribe>(m_session))
{
    m_subscription->oper_get_items_subscribe(moduleName.c_str(), OperationalDataCallback{dataSupplier}, path.c_str());
}
