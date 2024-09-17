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
        const std::string& module_name,
        const std::optional<std::string>& /* sub_xpath */,
        sysrepo::Event event,
        uint32_t /* request_id */)
    {
        using namespace std::string_literals;
        if (event == sysrepo::Event::Change) {
            return sysrepo::ErrorCode::Ok;
        }

        for (const auto& it : sess.getChanges("/"s + module_name + ":*//.")) {
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
            std::optional<std::string> previousList;

            if (it.previousList) {
                previousList = std::string{*it.previousList};
            }

            m_recorder->write(it.operation, std::string{xpath}, oldValue, newValue, previousList);
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
    : m_subscription([&moduleName, &rec, ds] { // This is an immediately invoked lambda.
        return sysrepo::Connection{}.sessionStart(ds).onModuleChange(moduleName,
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
            [[maybe_unused]] const std::string& moduleName,
            const std::optional<std::string>& subXPath,
            [[maybe_unused]] const std::optional<std::string>& requestXPath,
            [[maybe_unused]] uint32_t requestId,
            std::optional<libyang::DataNode>& output)
    {
        auto data = m_dataSupplier.get_data(*subXPath);
        for (const auto& [p, v] : data) {
            if (!output) {
                output = session.getContext().newPath(p, v.type() == typeid(empty_) ? std::nullopt : std::optional<std::string>(leafDataToString(v)));
            } else {
                output->newPath(p, v.type() == typeid(empty_) ? std::nullopt : std::optional<std::string>(leafDataToString(v)));
            }
        }
        return sysrepo::ErrorCode::Ok;
    }

private:
    const DataSupplier& m_dataSupplier;
};

OperationalDataSubscription::OperationalDataSubscription(const std::string& moduleName, const std::string& path, const DataSupplier& dataSupplier)
    : m_subscription(sysrepo::Connection{}.sessionStart().onOperGet(moduleName, OperationalDataCallback{dataSupplier}, path))
{
}
