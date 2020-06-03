#include <boost/algorithm/string/predicate.hpp>
#include <experimental/iterator>
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
    auto topLevelNode = node;
    while (topLevelNode->parent()) {
        topLevelNode = topLevelNode->parent();
    }
    auto existingTopLevelNode = std::find_if(m_datastore.begin(), m_datastore.end(), [topLevelNode](const std::shared_ptr<libyang::Data_Node>& existing) {return topLevelNode->path() == existing->path();});
    if (existingTopLevelNode == m_datastore.end()) {
        m_datastore.push_back(topLevelNode);
    } else {
        (*existingTopLevelNode)->merge(topLevelNode, 0);
    }

    validate();
}

void YangAccess::removeFromStorage(const std::shared_ptr<libyang::Data_Node>& node)
{
    auto firstNonContainerParent = node;
    while (firstNonContainerParent->parent() && firstNonContainerParent->parent()->schema()->nodetype() == LYS_CONTAINER) {
        firstNonContainerParent = firstNonContainerParent->parent();
    }
    if (!firstNonContainerParent->parent()) {
        auto it = std::remove_if(m_datastore.begin(), m_datastore.end(), [firstNonContainerParent] (const std::shared_ptr<libyang::Data_Node>& existing) {return firstNonContainerParent->path() == existing->path();});
        m_datastore.erase(it, m_datastore.end());
    } else {
        node->unlink();
    }
    validate();
}

void YangAccess::validate()
{
    for (const auto& it : m_datastore) {
        it->validate(LYD_OPT_DATA | LYD_OPT_DATA_NO_YANGLIB | LYD_OPT_STRICT, m_ctx);
    }
}

DatastoreAccess::Tree YangAccess::getItems(const std::string& path)
{
    DatastoreAccess::Tree res;
    if (m_datastore.empty()) {
        return res;
    }

    for (const auto& topLevelNodes : m_datastore) {
        auto filtered = topLevelNodes->find_path(path.c_str())->data();
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
    auto node = m_datastore.front()->find_path(path.c_str())->data().front();
    removeFromStorage(node);
}

void YangAccess::createListInstance(const std::string& path)
{
    auto node = m_schema->dataNodeFromPath(path);
    saveToStorage(node);
}

void YangAccess::deleteListInstance(const std::string& path)
{
    auto node = m_datastore.front()->find_path(path.c_str())->data().front();
    removeFromStorage(node);
}

void YangAccess::createLeafListInstance(const std::string& path)
{
    auto node = m_schema->dataNodeFromPath(path);
    saveToStorage(node);
}

void YangAccess::deleteLeafListInstance(const std::string& path)
{
    auto node = m_datastore.front()->find_path(path.c_str())->data().front();
    removeFromStorage(node);
}

void YangAccess::moveItem(const std::string&, std::variant<yang::move::Absolute, yang::move::Relative>)
{
    // TODO: implement this
}

void YangAccess::commitChanges()
{
    // TODO: implement this
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

std::string YangAccess::dumpConfig()
{
    std::stringstream ss;
    std::transform(m_datastore.begin(), m_datastore.end(), std::experimental::make_ostream_joiner(ss, "\n"), [] (const auto& tree) { return tree->print_mem(LYD_XML, LYP_WITHSIBLINGS); });
    return ss.str();
}

void YangAccess::addSchemaFile(const std::string& path)
{
    m_schema->addSchemaFile(path.c_str());
}

void YangAccess::addSchemaDir(const std::string& path)
{
    m_schema->addSchemaDirectory(path.c_str());
}
