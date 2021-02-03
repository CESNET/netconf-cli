#include <boost/algorithm/string/predicate.hpp>
#include <experimental/iterator>
#include <fstream>
#include <iostream>
#include <libyang/Tree_Data.hpp>
#include <libyang/libyang.h>
#include "UniqueResource.hpp"
#include "libyang_utils.hpp"
#include "utils.hpp"
#include "yang_access.hpp"
#include "yang_schema.hpp"

namespace {
template <typename Type> using lyPtrDeleter_type = void (*)(Type*);
template <typename Type> const lyPtrDeleter_type<Type> lyPtrDeleter;
template <> const auto lyPtrDeleter<ly_set> = ly_set_free;
template <> const auto lyPtrDeleter<ly_ctx> = static_cast<lyPtrDeleter_type<ly_ctx>>([] (auto* ptr) {ly_ctx_destroy(ptr, nullptr);});
template <> const auto lyPtrDeleter<lyd_node> = lyd_free_withsiblings;

template <typename Type>
auto lyWrap(Type* ptr)
{
    return std::unique_ptr<Type, lyPtrDeleter_type<Type>>{ptr, lyPtrDeleter<Type>};
}

// Convenient for functions that take m_datastore as an argument
using DatastoreType = std::unique_ptr<lyd_node, lyPtrDeleter_type<lyd_node>>;
}

YangAccess::YangAccess()
    : m_ctx(lyWrap(ly_ctx_new(nullptr, LY_CTX_DISABLE_SEARCHDIR_CWD)))
    , m_datastore(lyWrap<lyd_node>(nullptr))
    , m_schema(std::make_shared<YangSchema>(libyang::create_new_Context(m_ctx.get())))
    , m_validation_mode(LYD_OPT_DATA)
{
}

YangAccess::YangAccess(std::shared_ptr<YangSchema> schema)
    : m_ctx(schema->m_context->swig_ctx(), [](auto) {})
    , m_datastore(lyWrap<lyd_node>(nullptr))
    , m_schema(schema)
    , m_validation_mode(LYD_OPT_RPC)
{
}

YangAccess::~YangAccess() = default;

[[noreturn]] void YangAccess::getErrorsAndThrow() const
{
    auto errors = libyang::get_ly_errors(libyang::create_new_Context(m_ctx.get()));
    std::vector<DatastoreError> errorsRes;
    for (const auto& error : errors) {
        using namespace std::string_view_literals;
        errorsRes.emplace_back(error->errmsg(), error->errpath() != ""sv ? std::optional{error->errpath()} : std::nullopt);
    }

    throw DatastoreException(errorsRes);
}

void YangAccess::impl_newPath(const std::string& path, const std::optional<std::string>& value)
{
    auto newNode = lyd_new_path(m_datastore.get(), m_ctx.get(), path.c_str(), value ? (void*)value->c_str() : nullptr, LYD_ANYDATA_CONSTSTRING, LYD_PATH_OPT_UPDATE);
    if (!newNode) {
        getErrorsAndThrow();
    }
    if (!m_datastore) {
        m_datastore = lyWrap(newNode);
    }
}

namespace {
void impl_unlink(DatastoreType& datastore, lyd_node* what)
{
    // If the node to be unlinked is the one our datastore variable points to, we need to find a new one to point to (one of its siblings)
    if (datastore.get() == what) {
        auto oldDatastore = datastore.release();
        if (oldDatastore->prev != oldDatastore) {
            datastore = lyWrap(oldDatastore->prev);
        } else {
            datastore = lyWrap(oldDatastore->next);
        }
    }

    lyd_unlink(what);
}
}

void YangAccess::impl_removeNode(const std::string& path)
{
    auto set = lyWrap(lyd_find_path(m_datastore.get(), path.c_str()));
    if (!set || set->number == 0) {
        // Check if schema node exists - lyd_find_path first checks if the first argument is non-null before checking for path validity
        if (!ly_ctx_get_node(m_ctx.get(), nullptr, path.c_str(), 0)) {
            throw DatastoreException{{DatastoreError{"Schema node doesn't exist.", path}}};
        }
        // Check if libyang found another error
        if (ly_err_first(m_ctx.get())) {
            getErrorsAndThrow();
        }

        // Otherwise the datastore just doesn't contain the wanted node.
        throw DatastoreException{{DatastoreError{"Data node doesn't exist.", path}}};
    }

    auto toRemove = set->set.d[0];

    impl_unlink(m_datastore, toRemove);

    lyd_free(toRemove);
}

void YangAccess::validate()
{
    auto datastore = m_datastore.release();

    if (m_validation_mode == LYD_OPT_RPC) {
        lyd_validate(&datastore, m_validation_mode, nullptr);
    } else {
        lyd_validate(&datastore, m_validation_mode | LYD_OPT_DATA_NO_YANGLIB, m_ctx.get());
    }
    m_datastore = lyWrap(datastore);
}

DatastoreAccess::Tree YangAccess::getItems(const std::string& path) const
{
    DatastoreAccess::Tree res;
    if (!m_datastore) {
        return res;
    }

    auto set = lyWrap(lyd_find_path(m_datastore.get(), path == "/" ? "/*" : path.c_str()));
    auto setWrapper = libyang::Set(set.get(), nullptr);
    std::optional<std::string> ignoredXPathPrefix;
    lyNodesToTree(res, setWrapper.data());
    return res;
}

void YangAccess::setLeaf(const std::string& path, leaf_data_ value)
{
    auto lyValue = value.type() == typeid(empty_) ? std::nullopt : std::optional(leafDataToString(value));
    impl_newPath(path, lyValue);
}

void YangAccess::createItem(const std::string& path)
{
    impl_newPath(path);
}

void YangAccess::deleteItem(const std::string& path)
{
    impl_removeNode(path);
}

namespace {
struct impl_moveItem {
    DatastoreType& m_datastore;
    lyd_node* m_sourceNode;

    void operator()(yang::move::Absolute absolute) const
    {
        auto set = lyWrap(lyd_find_instance(m_sourceNode, m_sourceNode->schema));
        if (set->number == 1) { // m_sourceNode is the sole instance, do nothing
            return;
        }

        doUnlink();
        switch (absolute) {
        case yang::move::Absolute::Begin:
            if (set->set.d[0] == m_sourceNode) { // List is already at the beginning, do nothing
                return;
            }
            lyd_insert_before(set->set.d[0], m_sourceNode);
            return;
        case yang::move::Absolute::End:
            if (set->set.d[set->number - 1] == m_sourceNode) { // List is already at the end, do nothing
                return;
            }
            lyd_insert_after(set->set.d[set->number - 1], m_sourceNode);
            return;
        }
    }

    void operator()(const yang::move::Relative& relative) const
    {
        auto keySuffix = m_sourceNode->schema->nodetype == LYS_LIST ? instanceToString(relative.m_path)
                                                                    : leafDataToString(relative.m_path.at("."));
        lyd_node* destNode;
        lyd_find_sibling_val(m_sourceNode, m_sourceNode->schema, keySuffix.c_str(), &destNode);

        doUnlink();
        if (relative.m_position == yang::move::Relative::Position::After) {
            lyd_insert_after(destNode, m_sourceNode);
        } else {
            lyd_insert_before(destNode, m_sourceNode);
        }
    }

private:
    void doUnlink() const
    {
        impl_unlink(m_datastore, m_sourceNode);
    }
};
}

void YangAccess::moveItem(const std::string& source, std::variant<yang::move::Absolute, yang::move::Relative> move)
{
    auto set = lyWrap(lyd_find_path(m_datastore.get(), source.c_str()));
    if (!set) { // Error, the node probably doesn't exist in the schema
        getErrorsAndThrow();
    }
    if (set->number == 0) {
        return;
    }
    auto sourceNode = set->set.d[0];
    std::visit(impl_moveItem{m_datastore, sourceNode}, move);
}

void YangAccess::commitChanges()
{
    validate();
}

void YangAccess::discardChanges()
{
}

[[noreturn]] DatastoreAccess::Tree YangAccess::execute(const std::string& path, const Tree& input)
{
    auto root = lyWrap(lyd_new_path(nullptr, m_ctx.get(), path.c_str(), nullptr, LYD_ANYDATA_CONSTSTRING, 0));
    if (!root) {
        getErrorsAndThrow();
    }
    for (const auto& [k, v] : input) {
        if (v.type() == typeid(special_) && boost::get<special_>(v).m_value != SpecialValue::PresenceContainer) {
            continue;
        }

        lyd_new_path(root.get(), m_ctx.get(), k.c_str(), (void*)leafDataToString(v).c_str(), LYD_ANYDATA_CONSTSTRING, LYD_PATH_OPT_UPDATE);
    }
    throw std::logic_error("in-memory datastore doesn't support executing RPC/action");
}

void YangAccess::copyConfig(const Datastore source, const Datastore dest)
{
    if (source == Datastore::Startup && dest == Datastore::Running) {
        m_datastore = nullptr;
    }
}

std::shared_ptr<Schema> YangAccess::schema()
{
    return m_schema;
}

std::vector<ListInstance> YangAccess::listInstances(const std::string& path)
{
    std::vector<ListInstance> res;
    if (!m_datastore) {
        return res;
    }

    auto instances = lyWrap(lyd_find_path(m_datastore.get(), path.c_str()));
    auto instancesWrapper = libyang::Set(instances.get(), nullptr);
    for (const auto& list : instancesWrapper.data()) {
        ListInstance instance;
        for (const auto& child : list->child()->tree_for()) {
            if (child->schema()->nodetype() == LYS_LEAF) {
                libyang::Schema_Node_Leaf leafSchema(child->schema());
                if (leafSchema.is_key()) {
                    auto leafData = std::make_shared<libyang::Data_Node_Leaf_List>(child);
                    instance.insert({leafSchema.name(), leafValueFromNode(leafData)});
                }
            }
        }
        res.emplace_back(instance);
    }
    return res;
}

std::string YangAccess::dump(const DataFormat format) const
{
    char* output;
    lyd_print_mem(&output, m_datastore.get(), format == DataFormat::Xml ? LYD_XML : LYD_JSON, LYP_WITHSIBLINGS | LYP_FORMAT);
    std::unique_ptr<char, decltype(&free)> deleter{output, free};

    if (output) {
        std::string res = output;
        return res;
    }

    return "";
}

void YangAccess::loadModule(const std::string& name)
{
    m_schema->loadModule(name);
}

void YangAccess::addSchemaFile(const std::string& path)
{
    m_schema->addSchemaFile(path.c_str());
}

void YangAccess::addSchemaDir(const std::string& path)
{
    m_schema->addSchemaDirectory(path.c_str());
}

void YangAccess::enableFeature(const std::string& module, const std::string& feature)
{
    m_schema->enableFeature(module, feature);
}

void YangAccess::addDataFile(const std::string& path)
{
    std::ifstream fs(path);
    char firstChar;
    fs >> firstChar;

    std::cout << "Parsing \"" << path << "\" as " << (firstChar == '{' ? "JSON" : "XML") << "...\n";
    auto dataNode = lyd_parse_path(m_ctx.get(), path.c_str(), firstChar == '{' ? LYD_JSON : LYD_XML, LYD_OPT_DATA | LYD_OPT_DATA_NO_YANGLIB | LYD_OPT_TRUSTED);

    if (!dataNode) {
        throw std::runtime_error("Supplied data file " + path + " couldn't be parsed.");
    }

    if (!m_datastore) {
        m_datastore = lyWrap(dataNode);
    } else {
        lyd_merge(m_datastore.get(), dataNode, LYD_OPT_DESTRUCT);
    }

    validate();
}
