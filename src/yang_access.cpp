#include <boost/algorithm/string/predicate.hpp>
#include <experimental/iterator>
#include <fstream>
#include <iostream>
#include <libyang-cpp/DataNode.hpp>
#include <libyang/libyang.h>
#include "UniqueResource.hpp"
#include "libyang_utils.hpp"
#include "utils.hpp"
#include "yang_access.hpp"
#include "yang_schema.hpp"

namespace {
// Convenient for functions that take m_datastore as an argument
using DatastoreType = std::optional<libyang::DataNode>;
}

YangAccess::YangAccess()
    : m_ctx(nullptr, libyang::ContextOptions::DisableSearchCwd)
    , m_datastore(std::nullopt)
    , m_schema(std::make_shared<YangSchema>(m_ctx))
    // , m_validation_mode(LYD_OPT_DATA)
{
}

YangAccess::YangAccess(std::shared_ptr<YangSchema> schema)
    : m_ctx(schema->m_context)
    , m_datastore(std::nullopt)
    , m_schema(schema)
    // , m_validation_mode(LYD_OPT_RPC)
{
}

YangAccess::~YangAccess() = default;

[[noreturn]] void YangAccess::getErrorsAndThrow() const
{
    // auto errors = libyang::get_ly_errors(libyang::create_new_Context(m_ctx.get()));
    // std::vector<DatastoreError> errorsRes;
    // for (const auto& error : errors) {
    //     using namespace std::string_view_literals;
    //     errorsRes.emplace_back(error->errmsg(), error->errpath() != ""sv ? std::optional{error->errpath()} : std::nullopt);
    // }

    // throw DatastoreException(errorsRes);
    throw std::runtime_error("");
}

void YangAccess::impl_newPath(const std::string& path, const std::optional<std::string>& value)
{
    if (m_datastore) {
        m_datastore->newPath(path.c_str(), value ? value->c_str() : nullptr, libyang::CreationOptions::Update);
    } else {
        m_datastore = m_ctx.newPath(path.c_str(), value ? value->c_str() : nullptr, libyang::CreationOptions::Update);
    }
    // if (!newNode) {
    //     getErrorsAndThrow();
    // }
    // if (!m_datastore) {
    //     m_datastore = lyWrap(newNode);
    // }
}

namespace {
void impl_unlink(DatastoreType& datastore, libyang::DataNode what)
{
    // If the node to be unlinked is the one our datastore variable points to, we need to find a new one to point to (one of its siblings)
    if (datastore == what) {
        auto oldDatastore = datastore;
        if (oldDatastore->previousSibling() != oldDatastore) {
            datastore = oldDatastore->previousSibling();
        } else {
            datastore = oldDatastore->nextSibling();
        }
    }

    what.unlink();
}
}

void YangAccess::impl_removeNode(const std::string& path)
{
    auto toRemove = m_datastore->findPath(path.c_str());
    if (!toRemove) {
        // Otherwise the datastore just doesn't contain the wanted node.
        throw DatastoreException{{DatastoreError{"Data node doesn't exist.", path}}};
    }
    // auto set = lyWrap(lyd_find_path(m_datastore.get(), path.c_str()));
    // if (!set || set->number == 0) {
    //     // Check if schema node exists - lyd_find_path first checks if the first argument is non-null before checking for path validity
    //     if (!ly_ctx_get_node(m_ctx.get(), nullptr, path.c_str(), 0)) {
    //         throw DatastoreException{{DatastoreError{"Schema node doesn't exist.", path}}};
    //     }
    //     // Check if libyang found another error
    //     if (ly_err_first(m_ctx.get())) {
    //         getErrorsAndThrow();
    //     }

    // }

    impl_unlink(m_datastore, *toRemove);
}

void YangAccess::validate()
{
    // auto datastore = m_datastore.release();

    // if (m_validation_mode == LYD_OPT_RPC) {
    //     lyd_validate(&datastore, m_validation_mode, nullptr);
    // } else {
    //     lyd_validate(&datastore, m_validation_mode | LYD_OPT_DATA_NO_YANGLIB, m_ctx.get());
    // }
    if (m_datastore) {
        m_datastore->validateAll();
    }
}

DatastoreAccess::Tree YangAccess::getItems(const std::string& path) const
{
    DatastoreAccess::Tree res;
    if (!m_datastore) {
        return res;
    }

    auto set = m_datastore->findXPath(path == "/" ? "/*" : path.c_str());

    lyNodesToTree(res, set);
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
    libyang::DataNode m_sourceNode;

    void operator()(yang::move::Absolute absolute) const
    {
        auto set = m_sourceNode.findXPath(m_sourceNode.path().get().get());
        if (set.begin()++ == set.end()) { // m_sourceNode is the sole instance, do nothing
            return;
        }

        doUnlink();
        switch (absolute) {
        case yang::move::Absolute::Begin:
            if (*set.begin() == m_sourceNode) { // List is already at the beginning, do nothing
                return;
            }
            set.begin()->insertBefore(m_sourceNode);
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
