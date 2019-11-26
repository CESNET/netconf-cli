/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/

#include <libyang/Tree_Data.hpp>
#include <libyang/Tree_Schema.hpp>
#include <sysrepo-cpp/Session.hpp>
#include "libyang_utils.hpp"
#include "sysrepo_access.hpp"
#include "yang_schema.hpp"

leaf_data_ leafValueFromVal(const sysrepo::S_Val& value)
{
    using namespace std::string_literals;
    switch (value->type()) {
    case SR_INT8_T:
        return value->data()->get_int8();
    case SR_UINT8_T:
        return value->data()->get_uint8();
    case SR_INT16_T:
        return value->data()->get_int16();
    case SR_UINT16_T:
        return value->data()->get_uint16();
    case SR_INT32_T:
        return value->data()->get_int32();
    case SR_UINT32_T:
        return value->data()->get_uint32();
    case SR_INT64_T:
        return value->data()->get_int64();
    case SR_UINT64_T:
        return value->data()->get_uint64();
    case SR_BOOL_T:
        return value->data()->get_bool();
    case SR_STRING_T:
        return std::string(value->data()->get_string());
    case SR_ENUM_T:
        return enum_{std::string(value->data()->get_enum())};
    case SR_DECIMAL64_T:
        return value->data()->get_decimal64();
    case SR_CONTAINER_T:
        return special_{SpecialValue::Container};
    case SR_CONTAINER_PRESENCE_T:
        return special_{SpecialValue::PresenceContainer};
    case SR_LIST_T:
        return special_{SpecialValue::List};
    default: // TODO: implement all types
        return value->val_to_string();
    }
}

struct valFromValue : boost::static_visitor<sysrepo::S_Val> {
    sysrepo::S_Val operator()(const enum_& value) const
    {
        return std::make_shared<sysrepo::Val>(value.m_value.c_str(), SR_ENUM_T);
    }

    sysrepo::S_Val operator()(const binary_& value) const
    {
        return std::make_shared<sysrepo::Val>(value.m_value.c_str(), SR_BINARY_T);
    }

    sysrepo::S_Val operator()(const identityRef_& value) const
    {
        auto res = value.m_prefix.value().m_name + ":" + value.m_value;
        return std::make_shared<sysrepo::Val>(res.c_str(), SR_IDENTITYREF_T);
    }

    sysrepo::S_Val operator()(const special_& value) const
    {
        throw std::runtime_error("Tried constructing S_Val from a " + specialValueToString(value));
    }

    sysrepo::S_Val operator()(const std::string& value) const
    {
        return std::make_shared<sysrepo::Val>(value.c_str());
    }

    template <typename T>
    sysrepo::S_Val operator()(const T& value) const
    {
        return std::make_shared<sysrepo::Val>(value);
    }
};

SysrepoAccess::~SysrepoAccess() = default;

SysrepoAccess::SysrepoAccess(const std::string& appname)
    : m_connection(new sysrepo::Connection(appname.c_str()))
    , m_schema(new YangSchema())
{
    try {
        m_session = std::make_shared<sysrepo::Session>(m_connection);
    } catch (sysrepo::sysrepo_exception& ex) {
        reportErrors();
    }

    // If fetching a submodule, sysrepo::Session::get_schema will determine the revision from the main module.
    // That's why submoduleRevision is ignored.
    m_schema->registerModuleCallback([this](const char* moduleName, const char* revision, const char* submodule, [[maybe_unused]] const char* submoduleRevision) {
        return fetchSchema(moduleName, revision, submodule);
    });

    for (const auto& it : listSchemas()) {
        if (it->implemented()) {
            m_schema->loadModule(it->module_name());
            for (unsigned int i = 0; i < it->enabled_feature_cnt(); i++) {
                m_schema->enableFeature(it->module_name(), it->enabled_features(i));
            }
        }
    }
}

std::map<std::string, leaf_data_> SysrepoAccess::getItems(const std::string& path)
{
    using namespace std::string_literals;
    std::map<std::string, leaf_data_> res;

    auto fillMap = [&res](auto items) {
        if (!items)
            return;
        for (unsigned int i = 0; i < items->val_cnt(); i++) {
            res.emplace(items->val(i)->xpath(), leafValueFromVal(items->val(i)));
        }
    };

    try {
        if (path == "/") {
            // Sysrepo doesn't have a root node ("/"), so we take all top-level nodes from all schemas
            auto schemas = m_session->list_schemas();
            for (unsigned int i = 0; i < schemas->schema_cnt(); i++) {
                fillMap(m_session->get_items(("/"s + schemas->schema(i)->module_name() + ":*//.").c_str()));
            }
        } else {
            fillMap(m_session->get_items((path + "//.").c_str()));
        }
    } catch (sysrepo::sysrepo_exception& ex) {
        reportErrors();
    }
    return res;
}

void SysrepoAccess::setLeaf(const std::string& path, leaf_data_ value)
{
    try {
        m_session->set_item(path.c_str(), boost::apply_visitor(valFromValue(), value));
    } catch (sysrepo::sysrepo_exception& ex) {
        reportErrors();
    }
}

void SysrepoAccess::createPresenceContainer(const std::string& path)
{
    try {
        m_session->set_item(path.c_str());
    } catch (sysrepo::sysrepo_exception& ex) {
        reportErrors();
    }
}

void SysrepoAccess::deletePresenceContainer(const std::string& path)
{
    try {
        m_session->delete_item(path.c_str());
    } catch (sysrepo::sysrepo_exception& ex) {
        reportErrors();
    }
}

void SysrepoAccess::createListInstance(const std::string& path)
{
    try {
        m_session->set_item(path.c_str());
    } catch (sysrepo::sysrepo_exception& ex) {
        reportErrors();
    }
}

void SysrepoAccess::deleteListInstance(const std::string& path)
{
    try {
        m_session->delete_item(path.c_str());
    } catch (sysrepo::sysrepo_exception& ex) {
        reportErrors();
    }
}

void SysrepoAccess::commitChanges()
{
    try {
        m_session->commit();
    } catch (sysrepo::sysrepo_exception& ex) {
        reportErrors();
    }
}

void SysrepoAccess::discardChanges()
{
    try {
        m_session->discard_changes();
    } catch (sysrepo::sysrepo_exception& ex) {
        reportErrors();
    }
}

std::string SysrepoAccess::fetchSchema(const char* module, const char* revision, const char* submodule)
{
    std::string schema;
    try {
        schema = m_session->get_schema(module, revision, submodule, SR_SCHEMA_YANG);
    } catch (sysrepo::sysrepo_exception& ex) {
        reportErrors();
    }

    if (schema.empty())
        throw std::runtime_error(std::string("Module ") + module + " not available");

    return schema;
}

std::vector<std::shared_ptr<sysrepo::Yang_Schema>> SysrepoAccess::listSchemas()
{
    std::vector<sysrepo::S_Yang_Schema> res;
    std::shared_ptr<sysrepo::Yang_Schemas> schemas;
    try {
        schemas = m_session->list_schemas();
    } catch (sysrepo::sysrepo_exception& ex) {
        reportErrors();
    }
    for (unsigned int i = 0; i < schemas->schema_cnt(); i++) {
        auto schema = schemas->schema(i);
        res.push_back(schema);
    }
    return res;
}

std::shared_ptr<Schema> SysrepoAccess::schema()
{
    return m_schema;
}

[[noreturn]] void SysrepoAccess::reportErrors()
{
    // I only use get_last_errors to get error info, since the error code from
    // sysrepo_exception doesn't really give any meaningful information. For
    // example an "invalid argument" error could mean a node isn't enabled, or
    // it could mean something totally different and there is no documentation
    // for that, so it's better to just use the message sysrepo gives me.
    auto srErrors = m_session->get_last_errors();
    std::vector<DatastoreError> res;

    for (size_t i = 0; i < srErrors->error_cnt(); i++) {
        auto error = srErrors->error(i);
        res.emplace_back(error->message(), error->xpath() ? std::optional<std::string>{error->xpath()} : std::nullopt);
    }

    throw DatastoreException(res);
}

std::vector<ListInstance> SysrepoAccess::listInstances(const std::string& path)
{
    std::vector<ListInstance> res;
    auto lists = getItems(path);

    decltype(lists) instances;
    auto wantedTree = *(m_schema->dataNodeFromPath(path)->find_path(path.c_str())->data().begin());
    std::copy_if(lists.begin(), lists.end(), std::inserter(instances, instances.end()), [this, pathToCheck=wantedTree->schema()->path()](const auto& item) {
        // This filters out non-instances.
        if (item.second.type() != typeid(special_) || boost::get<special_>(item.second).m_value != SpecialValue::List) {
            return false;
        }

        // Now, getItems is recursive: it gives everything including nested lists. So I try create a tree from the instance...
        auto instanceTree = *(m_schema->dataNodeFromPath(item.first)->find_path(item.first.c_str())->data().begin());
        // And then check if its schema path matches the list we actually want. This filters out lists which are not the ones I requested.
        return instanceTree->schema()->path() == pathToCheck;
    });

    // If there are no instances, then just return
    if (instances.empty()) {
        return res;
    }

    // I need to find out which keys does the list have. To do that, I create a
    // tree from the first instance. This is gives me some top level node,
    // which will be our list in case out list is a top-level node. In case it
    // isn't, we have call find_path on the top level node. After that, I just
    // retrieve the keys.
    auto topLevelTree = m_schema->dataNodeFromPath(instances.begin()->first);
    auto list = *(topLevelTree->find_path(path.c_str())->data().begin());
    auto keys = libyang::Schema_Node_List{list->schema()}.keys();

    // Creating a full tree at the same time from the values sysrepo gives me
    // would be a pain (and after sysrepo switches to libyang meaningless), so
    // I just use this algorithm to create data nodes one by one and get the
    // key values from them.
    for (const auto& instance : instances) {
        auto wantedList = *(m_schema->dataNodeFromPath(instance.first)->find_path(path.c_str())->data().begin());
        ListInstance instanceRes;
        for (const auto& key : keys) {
            auto vec = wantedList->find_path(key->name())->data();
            auto leaf = libyang::Data_Node_Leaf_List{*(vec.begin())};
            instanceRes.emplace(key->name(), leafValueFromValue(leaf.value(), leaf.leaf_type()->base()));
        }
        res.push_back(instanceRes);
    }

    return res;
}
