/*
 * Copyright (C) 2020 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
*/
#include "proxy_datastore.hpp"
#include "yang_schema.hpp"

ProxyDatastore::ProxyDatastore(const std::shared_ptr<DatastoreAccess>& datastore)
    : m_datastore(datastore)
{
}

DatastoreAccess::Tree ProxyDatastore::getItems(const std::string& path) const
{
    return m_datastore->getItems(path);
}

void ProxyDatastore::setLeaf(const std::string& path, leaf_data_ value)
{
    m_datastore->setLeaf(path, value);
}

void ProxyDatastore::createItem(const std::string& path)
{
    m_datastore->createItem(path);
}

void ProxyDatastore::deleteItem(const std::string& path)
{
    m_datastore->deleteItem(path);
}

void ProxyDatastore::moveItem(const std::string& source, std::variant<yang::move::Absolute, yang::move::Relative> move)
{
    m_datastore->moveItem(source, move);
}

void ProxyDatastore::commitChanges()
{
    m_datastore->commitChanges();
}

void ProxyDatastore::discardChanges()
{
    m_datastore->discardChanges();
}

void ProxyDatastore::copyConfig(const Datastore source, const Datastore destination)
{
    m_datastore->copyConfig(source, destination);
}

std::string ProxyDatastore::dump(const DataFormat format) const
{
    return m_datastore->dump(format);
}

void ProxyDatastore::initiateRpc(const std::string&)
{
    auto schema = std::make_shared<YangSchema>();
    schema->registerModuleCallback([this] (const char* mod_name, const char* mod_revision, const char* submod_name, const char* submod_revision) {
        return m_datastore.schema()->yangSource(mod_name, mod_revision, submod_name, submod_revision);
    });
    for (auto module : m_datastore.schema()->allModules()) {
        schema->loadModule(module);
    }
    m_inputDatastore.emplace(schema);
}

std::shared_ptr<Schema> ProxyDatastore::schema() const
{
    return m_datastore->schema();
}
