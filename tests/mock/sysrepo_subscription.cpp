/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/

#include <experimental/iterator>
#include <sstream>
#include <sysrepo-cpp/Session.hpp>
#include "sysrepo_subscription.hpp"
#include "utils.hpp"


class MyCallback {
public:
    MyCallback(const std::string& moduleName, Recorder* rec)
        : m_moduleName(moduleName)
        , m_recorder(rec)
    {
    }

    sysrepo::ErrorCode operator()(
        sysrepo::Session sess,
        uint32_t /* sub_id */,
        std::string_view module_name,
        std::optional<std::string_view> /* sub_xpath */,
        sysrepo::Event event,
        uint32_t /* request_id */)
    {
        using namespace std::string_literals;
        if (event == sysrepo::Event::Change) {
            return sysrepo::ErrorCode::Ok;
        }

        for (const auto& it : sess.getChanges(("/"s + module_name.data() + ":*//.").c_str())) {
            auto xpath = it.node.path();
            std::optional<std::string> oldValue;
            std::optional<std::string> newValue;
            if (it.operation == sysrepo::ChangeOperation::Deleted) {
                oldValue = it.node.schema().nodeType() == libyang::NodeType::Leaf || it.node.schema().nodeType() == libyang::NodeType::Leaflist ?
                    std::optional<std::string>{it.node.asTerm().valueStr()} :
                    std::nullopt;
            } else {
                oldValue = std::optional<std::string>{it.previousValue};
                newValue = it.node.schema().nodeType() == libyang::NodeType::Leaf || it.node.schema().nodeType() == libyang::NodeType::Leaflist ?
                    std::optional<std::string>{it.node.asTerm().valueStr()} :
                    std::nullopt;

            }


            m_recorder->write(it.operation, std::string{xpath}, oldValue, newValue);
        }

        return sysrepo::ErrorCode::Ok;
    }

private:
    std::string m_moduleName;
    Recorder* m_recorder;
};

Recorder::~Recorder() = default;

DataSupplier::~DataSupplier() = default;

SysrepoSubscription::SysrepoSubscription(const std::string& moduleName, Recorder* rec, sysrepo::Datastore ds)
    : m_subscription([&moduleName, &rec, ds] { // Well.... :D
        return sysrepo::Connection{}.sessionStart(ds).onModuleChange(moduleName.c_str(),
                rec ? sysrepo::ModuleChangeCb{MyCallback{moduleName, rec}}
                : sysrepo::ModuleChangeCb{[](auto, auto, auto, auto, auto, auto) { return sysrepo::ErrorCode::Ok; }});
    }())
{
}

// class OperationalDataCallback {
// public:
//     OperationalDataCallback(const DataSupplier& dataSupplier)
//         : m_dataSupplier(dataSupplier)
//     {
//     }
//     int operator()(
//         [[maybe_unused]] sysrepo::S_Session sess,
//         [[maybe_unused]] const char* module_name,
//         const char* path,
//         [[maybe_unused]] const char* request_xpath,
//         [[maybe_unused]] uint32_t request_id,
//         libyang::S_Data_Node& parent)
//     {
//         auto data = m_dataSupplier.get_data(path);
//         libyang::S_Data_Node res;
//         for (const auto& [p, v] : data) {
//             if (!res) {
//                 res = std::make_shared<libyang::Data_Node>(
//                     sess->get_context(),
//                     p.c_str(),
//                     v.type() == typeid(empty_) ? nullptr : leafDataToString(v).c_str(),
//                     LYD_ANYDATA_CONSTSTRING,
//                     0);
//             } else {
//                 res->new_path(
//                     sess->get_context(),
//                     p.c_str(),
//                     v.type() == typeid(empty_) ? nullptr : leafDataToString(v).c_str(),
//                     LYD_ANYDATA_CONSTSTRING,
//                     0);
//             }
//         }
//         parent = res;
//         return SR_ERR_OK;
//     }

// private:
//     const DataSupplier& m_dataSupplier;
// };

// OperationalDataSubscription::OperationalDataSubscription(const std::string& moduleName, const std::string& path, const DataSupplier& dataSupplier)
//     : m_connection(std::make_shared<sysrepo::Connection>())
//     , m_session(std::make_shared<sysrepo::Session>(m_connection))
//     , m_subscription(std::make_shared<sysrepo::Subscribe>(m_session))
// {
//     m_subscription->oper_get_items_subscribe(moduleName.c_str(), OperationalDataCallback{dataSupplier}, path.c_str());
// }
