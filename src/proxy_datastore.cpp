/*
 * Copyright (C) 2020 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
*/
#include "proxy_datastore.hpp"

ProxyDatastore::ProxyDatastore(DatastoreAccess& datastore)
    : m_datastore(datastore)
{
}

ProxyDatastore::~ProxyDatastore() = default;

DatastoreAccess::Tree ProxyDatastore::getItems(const std::string& path) const
{
    return m_datastore.getItems(path);
}

void ProxyDatastore::setLeaf(const std::string& path, leaf_data_ value)
{
    m_datastore.setLeaf(path, value);
}

void ProxyDatastore::createItem(const std::string& path)
{
    m_datastore.createItem(path);
}

void ProxyDatastore::deleteItem(const std::string& path)
{
    m_datastore.deleteItem(path);
}

void ProxyDatastore::moveItem(const std::string& source, std::variant<yang::move::Absolute, yang::move::Relative> move)
{
    m_datastore.moveItem(source, move);
}

void ProxyDatastore::commitChanges()
{
    m_datastore.commitChanges();
}

void ProxyDatastore::discardChanges()
{
    m_datastore.discardChanges();
}

DatastoreAccess::Tree ProxyDatastore::executeRpc([[maybe_unused]] const std::string& path, [[maybe_unused]] const Tree& input)
{
    throw std::logic_error("ProxyDatastore::executeRpc - this method shouldn't be called.");
}

void ProxyDatastore::copyConfig(const Datastore source, const Datastore destination)
{
    m_datastore.copyConfig(source, destination);
}

std::string ProxyDatastore::dump(const DataFormat format) const
{
    return m_datastore.dump(format);
}

std::shared_ptr<Schema> ProxyDatastore::schema()
{
    return m_datastore.schema();
}

std::vector<ListInstance> ProxyDatastore::listInstances([[maybe_unused]] const std::string& path)
{
    throw std::logic_error("ProxyDatastore::listInstances - this method shouldn't be called.");
}
