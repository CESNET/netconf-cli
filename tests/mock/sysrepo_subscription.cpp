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

class OperationalDataCallback {
public:
    OperationalDataCallback(const DataSupplier& dataSupplier)
        : m_dataSupplier(dataSupplier)
    {
    }
    sysrepo::ErrorCode operator()(
            sysrepo::Session session,
            [[maybe_unused]] uint32_t subscriptionId,
            [[maybe_unused]] std::string_view moduleName,
            std::optional<std::string_view> subXPath,
            [[maybe_unused]] std::optional<std::string_view> requestXPath,
            [[maybe_unused]] uint32_t requestId,
            std::optional<libyang::DataNode>& output)
    {
        auto data = m_dataSupplier.get_data(subXPath->data());
        for (const auto& [p, v] : data) {
            if (!output) {
                output = session.getContext().newPath(p.c_str(), v.type() == typeid(empty_) ? nullptr : leafDataToString(v).c_str());
            } else {
                output->newPath(p.c_str(), v.type() == typeid(empty_) ? nullptr : leafDataToString(v).c_str());
            }
        }
        return sysrepo::ErrorCode::Ok;
    }

private:
    const DataSupplier& m_dataSupplier;
};

OperationalDataSubscription::OperationalDataSubscription(const std::string& moduleName, const std::string& path, const DataSupplier& dataSupplier)
    : m_subscription(sysrepo::Connection{}.sessionStart().onOperGetItems(moduleName.c_str(), OperationalDataCallback{dataSupplier}, path.c_str()))
{
}
