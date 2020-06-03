#include <boost/algorithm/string/predicate.hpp>
#include <experimental/iterator>
#include <sstream>
#include <libyang/Tree_Data.hpp>
#include "libyang_utils.hpp"
#include "utils.hpp"
#include "yang_access.hpp"
#include "yang_schema.hpp"

YangAccess::YangAccess()
    : m_ctx(std::make_shared<libyang::Context>())
    , m_schema(std::make_shared<YangSchema>(m_ctx))
{
}

YangAccess::~YangAccess() = default;

void YangAccess::saveToStorage(const libyang::S_Data_Node& node)
{
    if (!m_datastore) {
        auto topLevelNode = node;
        while (topLevelNode->parent()) {
            topLevelNode = topLevelNode->parent();
        }
        m_datastore = topLevelNode;
        return;
    }

    m_datastore->merge(node, 0);
}

void YangAccess::removeFromStorage(const std::shared_ptr<libyang::Data_Node>& node)
{
    auto firstNonContainerParent = node;
    while (firstNonContainerParent->parent() && firstNonContainerParent->parent()->schema()->nodetype() == LYS_CONTAINER) {
        firstNonContainerParent = firstNonContainerParent->parent();
    }

    if (m_datastore->path() == firstNonContainerParent->path()) {
        auto firstSibling = m_datastore->first_sibling();
        auto siblings = firstSibling->tree_for();
        // FIXME: what is a nonSameSibling, find a better name for this, it's a sibling we're gonna use to save to m_datastore, because the current is being unlinked
        auto nonSameSibling = std::find_if_not(siblings.begin(), siblings.end(), [this] (const auto& sibling) { return m_datastore->path() == sibling->path(); });
        if (nonSameSibling != siblings.end() && nonSameSibling->get()->path() != "/ietf-yang-library:yang-library") {
            m_datastore = *nonSameSibling;
        } else {
            m_datastore = nullptr;
        }
    }
}

void YangAccess::validate()
{
    if (!m_datastore) {
        return;
    }
    m_datastore->validate(LYD_OPT_DATA | LYD_OPT_DATA_NO_YANGLIB | LYD_OPT_STRICT, m_ctx);
}

DatastoreAccess::Tree YangAccess::getItems(const std::string& path)
{
    DatastoreAccess::Tree res;
    if (!m_datastore) {
        return res;
    }

    auto filtered = m_datastore->find_path(path.c_str())->data();
    for (auto it = filtered.begin(); it < filtered.end(); it++) {
        if ((*it)->schema()->nodetype() == LYS_LEAFLIST) {
            auto leafListPath = stripLeafListValueFromPath((*it)->path());
            res.emplace_back(leafListPath, special_{SpecialValue::LeafList});
            while (it != filtered.end() && boost::starts_with((*it)->path(), leafListPath)) {
                lyNodesToTree(res, (*it)->tree_dfs());
                it++;
            }
        } else {
            lyNodesToTree(res, (*it)->tree_dfs());
        }
    }
    return res;
}

void YangAccess::setLeaf(const std::string& path, leaf_data_ value)
{
    auto lyValue = value.type() == typeid(empty_) ? std::nullopt : std::optional(leafDataToString(value));
    auto node = m_schema->dataNodeFromPath(path, lyValue);
    saveToStorage(node);
}

void YangAccess::createPresenceContainer(const std::string& path)
{
    auto node = m_schema->dataNodeFromPath(path);
    saveToStorage(node);
}

void YangAccess::deletePresenceContainer(const std::string& path)
{
    auto node = m_datastore->find_path(path.c_str())->data().front();
    removeFromStorage(node);
}

void YangAccess::createListInstance(const std::string& path)
{
    auto node = m_schema->dataNodeFromPath(path);
    saveToStorage(node);
}

void YangAccess::deleteListInstance(const std::string& path)
{
    auto node = m_datastore->find_path(path.c_str())->data().front();
    removeFromStorage(node);
}

void YangAccess::createLeafListInstance(const std::string& path)
{
    auto node = m_schema->dataNodeFromPath(path);
    saveToStorage(node);
}

void YangAccess::deleteLeafListInstance(const std::string& path)
{
    auto node = m_datastore->find_path(path.c_str())->data().front();
    removeFromStorage(node);
}

void YangAccess::moveItem(const std::string&, std::variant<yang::move::Absolute, yang::move::Relative>)
{
    // TODO: implement this
}

void YangAccess::commitChanges()
{
    validate();
}

void YangAccess::discardChanges()
{
    // TODO: implement this
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

std::string impl_dumpConfig(const std::shared_ptr<libyang::Data_Node>& datastore, LYD_FORMAT format)
{
    if (!datastore) {
        return "";
    }
    return datastore->print_mem(format, LYP_WITHSIBLINGS | LYP_FORMAT);
}

std::string YangAccess::dumpXML() const
{
    return impl_dumpConfig(m_datastore, LYD_XML);
}

std::string YangAccess::dumpJSON() const
{
    return impl_dumpConfig(m_datastore, LYD_JSON);
}

void YangAccess::addSchemaFile(const std::string& path)
{
    m_schema->addSchemaFile(path.c_str());
}

void YangAccess::addSchemaDir(const std::string& path)
{
    m_schema->addSchemaDirectory(path.c_str());
}
