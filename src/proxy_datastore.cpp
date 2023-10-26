/*
 * Copyright (C) 2020 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
*/
#include <boost/algorithm/string/predicate.hpp>
#include "UniqueResource.hpp"
#include "proxy_datastore.hpp"
#include "yang_schema.hpp"

ProxyDatastore::ProxyDatastore(const std::shared_ptr<DatastoreAccess>& datastore, std::function<std::shared_ptr<DatastoreAccess>(const std::shared_ptr<DatastoreAccess>&)> createTemporaryDatastore)
    : m_datastore(datastore)
    , m_createTemporaryDatastore(createTemporaryDatastore)
{
}

DatastoreAccess::Tree ProxyDatastore::getItems(const std::string& path) const
{
    return pickDatastore(path)->getItems(path);
}

void ProxyDatastore::setLeaf(const std::string& path, leaf_data_ value)
{
    pickDatastore(path)->setLeaf(path, value);
}

void ProxyDatastore::createItem(const std::string& path)
{
    pickDatastore(path)->createItem(path);
}

void ProxyDatastore::deleteItem(const std::string& path)
{
    pickDatastore(path)->deleteItem(path);
}

void ProxyDatastore::moveItem(const std::string& source, std::variant<yang::move::Absolute, yang::move::Relative> move)
{
    pickDatastore(source)->moveItem(source, move);
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

void ProxyDatastore::initiate(const std::string& path)
{
    if (m_inputDatastore) {
        throw std::runtime_error("RPC/action input already in progress (" + *m_inputPath + ")");
    }
    m_inputDatastore = m_createTemporaryDatastore(m_datastore);
    m_inputPath = path;
    m_inputDatastore->createItem(path);
}

DatastoreAccess::Tree ProxyDatastore::execute()
{
    if (!m_inputDatastore) {
        throw std::runtime_error("No RPC/action input in progress");
    }
    auto cancelOnReturn = make_unique_resource([] {}, [this] { cancel(); });
    auto inputData = m_inputDatastore->getItems("/");

    auto out = m_datastore->execute(*m_inputPath, inputData);

    return out;
}

void ProxyDatastore::cancel()
{
    m_inputDatastore = nullptr;
    m_inputPath = std::nullopt;
}

std::shared_ptr<Schema> ProxyDatastore::schema() const
{
    return m_datastore->schema();
}

std::optional<std::string> ProxyDatastore::inputDatastorePath() const
{
    return m_inputPath;
}

std::shared_ptr<DatastoreAccess> ProxyDatastore::pickDatastore(const std::string& path) const
{
    if (!m_inputDatastore || !boost::starts_with(path, *m_inputPath)) {
        return m_datastore;
    } else {
        return m_inputDatastore;
    }
}

DatastoreTarget ProxyDatastore::target() const
{
    return m_datastore->target();
}

void ProxyDatastore::setTarget(const DatastoreTarget target)
{
    m_datastore->setTarget(target);
}
