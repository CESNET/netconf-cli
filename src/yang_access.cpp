#include <libyang/Tree_Data.hpp>
#include "libyang_utils.hpp"
#include "utils.hpp"
#include "yang_access.hpp"
#include "yang_schema.hpp"

YangAccess::YangAccess()
{
}

YangAccess::~YangAccess() = default;

void YangAccess::mergeToStaging(const libyang::S_Data_Node& node)
{
    if (!m_staging) {
        m_staging = node;
        return;
    }

    m_staging->merge(node, LYD_OPT_DESTRUCT);
}

DatastoreAccess::Tree YangAccess::getItems(const std::string& path)
{
    DatastoreAccess::Tree res;
    auto toPrint = m_staging->find_path(path.c_str());
    if (toPrint->number() == 0) {
        return res;
    }

    lyNodesToTree(res, toPrint->data());

    return res;
}

void YangAccess::setLeaf(const std::string& path, leaf_data_ value)
{
    auto lyValue = value.type() == typeid(empty_) ? std::nullopt : std::optional(leafDataToString(value));
    auto node = m_schema->dataNodeFromPath(path, lyValue);
    mergeToStaging(node);
}

void YangAccess::createPresenceContainer(const std::string& path)
{
    auto node = m_schema->dataNodeFromPath(path);
    mergeToStaging(node);
}

void YangAccess::deletePresenceContainer(const std::string&)
{
    // TODO: implement this
}

void YangAccess::createListInstance(const std::string& path)
{
    auto node = m_schema->dataNodeFromPath(path);
    mergeToStaging(node);
}

void YangAccess::deleteListInstance(const std::string&)
{
    // TODO: implement this
}

void YangAccess::createLeafListInstance(const std::string& path)
{
    auto node = m_schema->dataNodeFromPath(path);
    mergeToStaging(node);
}

void YangAccess::deleteLeafListInstance(const std::string&)
{
    // TODO: implement this
}

void YangAccess::commitChanges()
{
    m_datastore = m_staging;
}

void YangAccess::discardChanges()
{
    m_staging = m_datastore;
}

DatastoreAccess::Tree YangAccess::executeRpc(const std::string&, const Tree&)
{
    // TODO: implement this..? maybe?
    return {};
}

void YangAccess::copyConfig(const Datastore, const Datastore)
{
    // TODO: probably don't implement this... yet?
}

std::shared_ptr<Schema> YangAccess::schema()
{
    return m_schema;
}

std::vector<ListInstance> YangAccess::listInstances(const std::string&)
{
    // TODO: implement this..?
    return {};
}
