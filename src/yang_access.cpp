#include <iostream>
#include <boost/algorithm/string/predicate.hpp>
#include <experimental/iterator>
#include <libyang/libyang.h>
#include <libyang/Tree_Data.hpp>
#include "UniqueResource.hpp"
#include "libyang_utils.hpp"
#include "utils.hpp"
#include "yang_access.hpp"
#include "yang_schema.hpp"

YangAccess::YangAccess()
    : m_ctx(ly_ctx_new(nullptr, LY_CTX_DISABLE_SEARCHDIR_CWD))
    , m_datastore(nullptr)
    , m_schema(std::make_shared<YangSchema>(libyang::create_new_Context(m_ctx)))
{
}

YangAccess::~YangAccess()
{
    lyd_free_withsiblings(m_datastore);
    ly_ctx_destroy(m_ctx, nullptr);
}

void YangAccess::impl_newPath(const std::string& path, const std::optional<std::string>& value)
{
    // FIXME: libyang wants void*, I don't know which *_cast I should use, but c-style cast seems to work :(
    auto newNode = lyd_new_path(m_datastore, m_ctx, path.c_str(), value ? (void*)(value->c_str()) : nullptr, LYD_ANYDATA_CONSTSTRING, LYD_PATH_OPT_UPDATE);
    if (!m_datastore) {
        m_datastore = newNode;
    }
}

void impl_unlink(lyd_node*& from, const lyd_node* what)
{
    if (from == what) {
        if (from->prev != from) {
            from = from->prev;
        } else {
            from = from->next;
        }
    }
}

void YangAccess::impl_removeNode(const std::string& path)
{
    auto set = lyd_find_path(m_datastore, path.c_str());
    auto toRemove = set->set.d[0];

    impl_unlink(m_datastore, toRemove);

    lyd_free(toRemove);
    ly_set_free(set);
}

void YangAccess::validate()
{
    lyd_validate(&m_datastore, LYD_OPT_DATA | LYD_OPT_DATA_NO_YANGLIB, m_ctx);
}

DatastoreAccess::Tree YangAccess::getItems(const std::string& path)
{
    DatastoreAccess::Tree res;
    if (!m_datastore) {
        return res;
    }

    auto set = lyd_find_path(m_datastore, path.c_str());
    auto setWrapper = libyang::Set(set, nullptr);

    auto siblings = setWrapper.data();
    for (auto it = siblings.begin(); it < siblings.end(); it++) {
        if ((*it)->schema()->nodetype() == LYS_LEAFLIST) {
            auto leafListPath = stripLeafListValueFromPath((*it)->path());
            res.emplace_back(leafListPath, special_{SpecialValue::LeafList});
            while (it != siblings.end() && boost::starts_with((*it)->path(), leafListPath)) {
                lyNodesToTree(res, (*it)->tree_dfs());
                it++;
            }
        } else {
            lyNodesToTree(res, (*it)->tree_dfs());
        }
    }


    ly_set_free(set);
    return res;
}

void YangAccess::setLeaf(const std::string& path, leaf_data_ value)
{
    auto lyValue = value.type() == typeid(empty_) ? std::nullopt : std::optional(leafDataToString(value));
    impl_newPath(path, lyValue);
}

void YangAccess::createPresenceContainer(const std::string& path)
{
    impl_newPath(path);
}

void YangAccess::deletePresenceContainer(const std::string& path)
{
    impl_removeNode(path);
}

void YangAccess::createListInstance(const std::string& path)
{
    impl_newPath(path);
}

void YangAccess::deleteListInstance(const std::string& path)
{
    impl_removeNode(path);
}

void YangAccess::createLeafListInstance(const std::string& path)
{
    impl_newPath(path);
}

void YangAccess::deleteLeafListInstance(const std::string& path)
{
    impl_removeNode(path);
}

struct impl_LeafData {
    lyd_node*& m_datastore;
    lyd_node* m_sourceNode;
    const std::string& m_sourcePath;

    impl_LeafData(lyd_node*& datastore, const std::string& source)
        : m_datastore(datastore)
        , m_sourcePath(source)
    {
        auto set = lyd_find_path(m_datastore, source.c_str());
        m_sourceNode = set->set.d[0];
        ly_set_free(set);

        impl_unlink(m_datastore, m_sourceNode);
    }

    void operator()(yang::move::Absolute) const
    {
        // TODO shouldn't be too difficult with lyd_find_sibling_set, but I have to solve this "unlink first problem"
    }

    void operator()(const yang::move::Relative& relative) const
    {
        // FIXME: change this to use lyd_find_sibling_val maybe? Not sure, because I don't know what to type in for the siblings argument, and the source node is already unlinked...
        auto keySuffix =
            m_sourceNode->schema->nodetype == LYS_LIST ? instanceToString(m_sourceNode->schema->module->name, relative.m_path)
                                                       : "[.=" + escapeListKeyString(leafDataToString(relative.m_path.at("."))) + "]";
        auto destPath = stripLeafListValueFromPath(m_sourcePath) + keySuffix;
        auto set = lyd_find_path(m_datastore, destPath.c_str());
        auto destNode = set->set.d[0];
        ly_set_free(set);

        if (relative.m_position == yang::move::Relative::Position::After) {
            lyd_insert_after(destNode, m_sourceNode);
        } else {
            lyd_insert_before(destNode, m_sourceNode);
        }
    }
};

void YangAccess::moveItem(const std::string& source, std::variant<yang::move::Absolute, yang::move::Relative> move)
{
    std::visit(impl_LeafData(m_datastore, source), move);
}

void YangAccess::commitChanges()
{
    validate();
}

void YangAccess::discardChanges()
{
}

DatastoreAccess::Tree YangAccess::executeRpc(const std::string&, const Tree&)
{
    // TODO: implement this..? maybe?
    return {};
}

void YangAccess::copyConfig(const Datastore source, const Datastore dest)
{
    if (source == Datastore::Startup && dest == Datastore::Running) {
        lyd_free_withsiblings(m_datastore);
        m_datastore = nullptr;
    }
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

std::string impl_dumpConfig(const lyd_node* datastore, LYD_FORMAT format)
{
    char* output;
    lyd_print_mem(&output, datastore, format, LYP_WITHSIBLINGS);

    if (output) {
        std::string res = output;
        free(output);
        return res;
    }

    return "";
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
