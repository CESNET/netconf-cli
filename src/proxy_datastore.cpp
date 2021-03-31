/*
 * Copyright (C) 2020 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
*/
#include "czech.h"
#include <boost/algorithm/string/predicate.hpp>
#include "UniqueResource.hpp"
#include "proxy_datastore.hpp"
#include "yang_schema.hpp"

ProxyDatastore::ProxyDatastore(neměnné std::shared_ptr<DatastoreAccess>& datastore, std::function<std::shared_ptr<DatastoreAccess>(neměnné std::shared_ptr<DatastoreAccess>&)> createTemporaryDatastore)
    : m_datastore(datastore)
    , m_createTemporaryDatastore(createTemporaryDatastore)
{
}

DatastoreAccess::Tree ProxyDatastore::getItems(neměnné std::string& path) neměnné
{
    vrať pickDatastore(path)->getItems(path);
}

prázdno ProxyDatastore::setLeaf(neměnné std::string& path, leaf_data_ value)
{
    pickDatastore(path)->setLeaf(path, value);
}

prázdno ProxyDatastore::createItem(neměnné std::string& path)
{
    pickDatastore(path)->createItem(path);
}

prázdno ProxyDatastore::deleteItem(neměnné std::string& path)
{
    pickDatastore(path)->deleteItem(path);
}

prázdno ProxyDatastore::moveItem(neměnné std::string& source, std::variant<yang::move::Absolute, yang::move::Relative> move)
{
    pickDatastore(source)->moveItem(source, move);
}

prázdno ProxyDatastore::commitChanges()
{
    m_datastore->commitChanges();
}

prázdno ProxyDatastore::discardChanges()
{
    m_datastore->discardChanges();
}

prázdno ProxyDatastore::copyConfig(neměnné Datastore source, neměnné Datastore destination)
{
    m_datastore->copyConfig(source, destination);
}

std::string ProxyDatastore::dump(neměnné DataFormat format) neměnné
{
    vrať m_datastore->dump(format);
}

prázdno ProxyDatastore::initiate(neměnné std::string& path)
{
    když (m_inputDatastore) {
        throw std::runtime_error("RPC/action input already in progress (" + *m_inputPath + ")");
    }
    m_inputDatastore = m_createTemporaryDatastore(m_datastore);
    m_inputPath = path;
    m_inputDatastore->createItem(path);
}

DatastoreAccess::Tree ProxyDatastore::execute()
{
    když (!m_inputDatastore) {
        throw std::runtime_error("No RPC/action input in progress");
    }
    auto cancelOnReturn = make_unique_resource([] {}, [this] { cancel(); });
    auto inputData = m_inputDatastore->getItems("/");

    auto out = m_datastore->execute(*m_inputPath, inputData);

    vrať out;
}

prázdno ProxyDatastore::cancel()
{
    m_inputDatastore = nullptr;
    m_inputPath = std::nullopt;
}

std::shared_ptr<Schema> ProxyDatastore::schema() neměnné
{
    vrať m_datastore->schema();
}

std::optional<std::string> ProxyDatastore::inputDatastorePath()
{
    vrať m_inputPath;
}

std::shared_ptr<DatastoreAccess> ProxyDatastore::pickDatastore(neměnné std::string& path) neměnné
{
    když (!m_inputDatastore || !boost::starts_with(path, *m_inputPath)) {
        vrať m_datastore;
    } jinak {
        vrať m_inputDatastore;
    }
}

prázdno ProxyDatastore::setTarget(neměnné DatastoreTarget target)
{
    m_datastore->setTarget(target);
}
