#include <libyang/Tree_Data.hpp>
#include "libyang_utils.hpp"
#include "utils.hpp"
#include "yang_access.hpp"
#include "yang_schema.hpp"

YangAccess::YangAccess()
    : m_schema(std::make_shared<YangSchema>())
{
}

YangAccess::~YangAccess() = default;

void YangAccess::mergeToStaging(const libyang::S_Data_Node& node)
{
    if (!m_staging) {
        m_staging = node;
        validateStaging();
        return;
    }

    m_staging->merge(node, LYD_OPT_DESTRUCT);
    validateStaging();
}

void YangAccess::validateStaging()
{
    m_staging->validate(LYD_OPT_DATA | LYD_OPT_DATA_NO_YANGLIB, m_staging);
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

void YangAccess::deletePresenceContainer(const std::string& path)
{
    m_staging->find_path(path.c_str())->data().front()->unlink();
    validateStaging();
}

void YangAccess::createListInstance(const std::string& path)
{
    auto node = m_schema->dataNodeFromPath(path);
    mergeToStaging(node);
}

void YangAccess::deleteListInstance(const std::string& path)
{
    // This leaves m_staging in kind of a weird state, if I unlink the last node... hmm. I need to somehow check if m_staging is completely empty
    m_staging->find_path(path.c_str())->data().front()->unlink();
    validateStaging();
}

void YangAccess::createLeafListInstance(const std::string& path)
{
    auto node = m_schema->dataNodeFromPath(path);
    mergeToStaging(node);
}

void YangAccess::deleteLeafListInstance(const std::string& path)
{
    m_staging->find_path(path.c_str())->data().front()->unlink();
    validateStaging();
}

void YangAccess::moveItem(const std::string&, std::variant<yang::move::Absolute, yang::move::Relative>)
{
    // TODO: implement this
}

void YangAccess::commitChanges()
{
    m_datastore = m_staging;// ? m_staging->dup_withsiblings(1) : nullptr;
}

void YangAccess::discardChanges()
{
    m_staging = m_datastore;// ? m_datastore->dup_withsiblings(1) : nullptr;
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

std::string YangAccess::dumpConfig()
{
    return m_datastore->print_mem(LYD_XML, LYP_WITHSIBLINGS);
}

void YangAccess::addSchemaFile(const std::string& path)
{
    m_schema->addSchemaFile(path.c_str());
}

void YangAccess::addSchemaDir(const std::string& path)
{
    m_schema->addSchemaDirectory(path.c_str());
}
