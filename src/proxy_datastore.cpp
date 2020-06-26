/*
 * Copyright (C) 2020 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
*/
#include <boost/algorithm/string/predicate.hpp>
#include "proxy_datastore.hpp"
#include "yang_schema.hpp"

ProxyDatastore::ProxyDatastore(const std::shared_ptr<DatastoreAccess>& datastore, std::function<std::shared_ptr<DatastoreAccess>(const std::shared_ptr<DatastoreAccess>&)> getTemporaryDatastore)
    : internal_no_touchy_touchy_datastore(datastore)
    , m_getTemporaryDatastore(getTemporaryDatastore)
{
}

DatastoreAccess::Tree ProxyDatastore::getItems(const std::string& path) const
{
    return datastore(path)->getItems(path);
}

void ProxyDatastore::setLeaf(const std::string& path, leaf_data_ value)
{
    datastore(path)->setLeaf(path, value);
}

void ProxyDatastore::createItem(const std::string& path)
{
    datastore(path)->createItem(path);
}

void ProxyDatastore::deleteItem(const std::string& path)
{
    datastore(path)->deleteItem(path);
}

void ProxyDatastore::moveItem(const std::string& source, std::variant<yang::move::Absolute, yang::move::Relative> move)
{
    datastore(source)->moveItem(source, move);
}

void ProxyDatastore::commitChanges()
{
    internal_no_touchy_touchy_datastore->commitChanges();
    if (internal_no_touchy_touchy_input_datastore) {
        internal_no_touchy_touchy_input_datastore->commitChanges();
    }
}

void ProxyDatastore::discardChanges()
{
    datastore("")->discardChanges();
}

void ProxyDatastore::copyConfig(const Datastore source, const Datastore destination)
{
    datastore("")->copyConfig(source, destination);
}

std::string ProxyDatastore::dump(const DataFormat format) const
{
    return datastore("")->dump(format);
}

void ProxyDatastore::initiateRpc(const std::string& rpcPath)
{
    internal_no_touchy_touchy_input_datastore = m_getTemporaryDatastore(internal_no_touchy_touchy_datastore);
    m_rpcPath = rpcPath;
    internal_no_touchy_touchy_input_datastore->createItem(rpcPath);
}

DatastoreAccess::Tree ProxyDatastore::executeRpc()
{
    if (!internal_no_touchy_touchy_input_datastore) {
        throw std::runtime_error("No rpc input in progress?");
    }
    auto inputData = internal_no_touchy_touchy_input_datastore->getItems("/");
    internal_no_touchy_touchy_input_datastore = nullptr;
    return internal_no_touchy_touchy_datastore->executeRpc(m_rpcPath, inputData);
}

std::shared_ptr<Schema> ProxyDatastore::schema() const
{
    return datastore("")->schema();
}

std::shared_ptr<DatastoreAccess> ProxyDatastore::datastore(const std::string& path) const
{
    if (boost::starts_with(path, m_rpcPath)) {
        return internal_no_touchy_touchy_input_datastore;
    } else {
        return internal_no_touchy_touchy_datastore;
    }
}
