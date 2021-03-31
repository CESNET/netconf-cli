#include "czech.h"
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
template <typename Type> using lyPtrDeleter_type = prázdno (*)(Type*);
template <typename Type> neměnné lyPtrDeleter_type<Type> lyPtrDeleter;
template <> neměnné auto lyPtrDeleter<ly_set> = ly_set_free;
template <> neměnné auto lyPtrDeleter<ly_ctx> = static_cast<lyPtrDeleter_type<ly_ctx>>([] (auto* ptr) {ly_ctx_destroy(ptr, nullptr);});
template <> neměnné auto lyPtrDeleter<lyd_node> = lyd_free_withsiblings;

template <typename Type>
auto lyWrap(Type* ptr)
{
    vrať std::unique_ptr<Type, lyPtrDeleter_type<Type>>{ptr, lyPtrDeleter<Type>};
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

YangAccess::~YangAccess() = výchozí;

[[noreturn]] prázdno YangAccess::getErrorsAndThrow() neměnné
{
    auto errors = libyang::get_ly_errors(libyang::create_new_Context(m_ctx.get()));
    std::vector<DatastoreError> errorsRes;
    pro (neměnné auto& error : errors) {
        using namespace std::string_view_literals;
        errorsRes.emplace_back(error->errmsg(), error->errpath() != ""sv ? std::optional{error->errpath()} : std::nullopt);
    }

    throw DatastoreException(errorsRes);
}

prázdno YangAccess::impl_newPath(neměnné std::string& path, neměnné std::optional<std::string>& value)
{
    auto newNode = lyd_new_path(m_datastore.get(), m_ctx.get(), path.c_str(), value ? (prázdno*)value->c_str() : nullptr, LYD_ANYDATA_CONSTSTRING, LYD_PATH_OPT_UPDATE);
    když (!newNode) {
        getErrorsAndThrow();
    }
    když (!m_datastore) {
        m_datastore = lyWrap(newNode);
    }
}

namespace {
prázdno impl_unlink(DatastoreType& datastore, lyd_node* what)
{
    // If the node to be unlinked is the one our datastore variable points to, we need to find a new one to point to (one of its siblings)
    když (datastore.get() == what) {
        auto oldDatastore = datastore.release();
        když (oldDatastore->prev != oldDatastore) {
            datastore = lyWrap(oldDatastore->prev);
        } jinak {
            datastore = lyWrap(oldDatastore->next);
        }
    }

    lyd_unlink(what);
}
}

prázdno YangAccess::impl_removeNode(neměnné std::string& path)
{
    auto set = lyWrap(lyd_find_path(m_datastore.get(), path.c_str()));
    když (!set || set->number == 0) {
        // Check if schema node exists - lyd_find_path first checks if the first argument is non-null before checking for path validity
        když (!ly_ctx_get_node(m_ctx.get(), nullptr, path.c_str(), 0)) {
            throw DatastoreException{{DatastoreError{"Schema node doesn't exist.", path}}};
        }
        // Check if libyang found another error
        když (ly_err_first(m_ctx.get())) {
            getErrorsAndThrow();
        }

        // Otherwise the datastore just doesn't contain the wanted node.
        throw DatastoreException{{DatastoreError{"Data node doesn't exist.", path}}};
    }

    auto toRemove = set->set.d[0];

    impl_unlink(m_datastore, toRemove);

    lyd_free(toRemove);
}

prázdno YangAccess::validate()
{
    auto datastore = m_datastore.release();

    když (m_validation_mode == LYD_OPT_RPC) {
        lyd_validate(&datastore, m_validation_mode, nullptr);
    } jinak {
        lyd_validate(&datastore, m_validation_mode | LYD_OPT_DATA_NO_YANGLIB, m_ctx.get());
    }
    m_datastore = lyWrap(datastore);
}

DatastoreAccess::Tree YangAccess::getItems(neměnné std::string& path) neměnné
{
    DatastoreAccess::Tree res;
    když (!m_datastore) {
        vrať res;
    }

    auto set = lyWrap(lyd_find_path(m_datastore.get(), path == "/" ? "/*" : path.c_str()));
    auto setWrapper = libyang::Set(set.get(), nullptr);
    std::optional<std::string> ignoredXPathPrefix;
    lyNodesToTree(res, setWrapper.data());
    vrať res;
}

prázdno YangAccess::setLeaf(neměnné std::string& path, leaf_data_ value)
{
    auto lyValue = value.type() == typeid(empty_) ? std::nullopt : std::optional(leafDataToString(value));
    impl_newPath(path, lyValue);
}

prázdno YangAccess::createItem(neměnné std::string& path)
{
    impl_newPath(path);
}

prázdno YangAccess::deleteItem(neměnné std::string& path)
{
    impl_removeNode(path);
}

namespace {
struct impl_moveItem {
    DatastoreType& m_datastore;
    lyd_node* m_sourceNode;

    prázdno operator()(yang::move::Absolute absolute) neměnné
    {
        auto set = lyWrap(lyd_find_instance(m_sourceNode, m_sourceNode->schema));
        když (set->number == 1) { // m_sourceNode is the sole instance, do nothing
            vrať;
        }

        doUnlink();
        přepínač (absolute) {
        případ yang::move::Absolute::Begin:
            když (set->set.d[0] == m_sourceNode) { // List is already at the beginning, do nothing
                vrať;
            }
            lyd_insert_before(set->set.d[0], m_sourceNode);
            vrať;
        případ yang::move::Absolute::End:
            když (set->set.d[set->number - 1] == m_sourceNode) { // List is already at the end, do nothing
                vrať;
            }
            lyd_insert_after(set->set.d[set->number - 1], m_sourceNode);
            vrať;
        }
    }

    prázdno operator()(neměnné yang::move::Relative& relative) neměnné
    {
        auto keySuffix = m_sourceNode->schema->nodetype == LYS_LIST ? instanceToString(relative.m_path)
                                                                    : leafDataToString(relative.m_path.at("."));
        lyd_node* destNode;
        lyd_find_sibling_val(m_sourceNode, m_sourceNode->schema, keySuffix.c_str(), &destNode);

        doUnlink();
        když (relative.m_position == yang::move::Relative::Position::After) {
            lyd_insert_after(destNode, m_sourceNode);
        } jinak {
            lyd_insert_before(destNode, m_sourceNode);
        }
    }

private:
    prázdno doUnlink() neměnné
    {
        impl_unlink(m_datastore, m_sourceNode);
    }
};
}

prázdno YangAccess::moveItem(neměnné std::string& source, std::variant<yang::move::Absolute, yang::move::Relative> move)
{
    auto set = lyWrap(lyd_find_path(m_datastore.get(), source.c_str()));
    když (!set) { // Error, the node probably doesn't exist in the schema
        getErrorsAndThrow();
    }
    když (set->number == 0) {
        vrať;
    }
    auto sourceNode = set->set.d[0];
    std::visit(impl_moveItem{m_datastore, sourceNode}, move);
}

prázdno YangAccess::commitChanges()
{
    validate();
}

prázdno YangAccess::discardChanges()
{
}

[[noreturn]] DatastoreAccess::Tree YangAccess::execute(neměnné std::string& path, neměnné Tree& input)
{
    auto root = lyWrap(lyd_new_path(nullptr, m_ctx.get(), path.c_str(), nullptr, LYD_ANYDATA_CONSTSTRING, 0));
    když (!root) {
        getErrorsAndThrow();
    }
    pro (neměnné auto& [k, v] : input) {
        když (v.type() == typeid(special_) && boost::get<special_>(v).m_value != SpecialValue::PresenceContainer) {
            pokračuj;
        }

        lyd_new_path(root.get(), m_ctx.get(), k.c_str(), (prázdno*)leafDataToString(v).c_str(), LYD_ANYDATA_CONSTSTRING, LYD_PATH_OPT_UPDATE);
    }
    throw std::logic_error("in-memory datastore doesn't support executing RPC/action");
}

prázdno YangAccess::copyConfig(neměnné Datastore source, neměnné Datastore dest)
{
    když (source == Datastore::Startup && dest == Datastore::Running) {
        m_datastore = nullptr;
    }
}

std::shared_ptr<Schema> YangAccess::schema()
{
    vrať m_schema;
}

std::vector<ListInstance> YangAccess::listInstances(neměnné std::string& path)
{
    std::vector<ListInstance> res;
    když (!m_datastore) {
        vrať res;
    }

    auto instances = lyWrap(lyd_find_path(m_datastore.get(), path.c_str()));
    auto instancesWrapper = libyang::Set(instances.get(), nullptr);
    pro (neměnné auto& list : instancesWrapper.data()) {
        ListInstance instance;
        pro (neměnné auto& child : list->child()->tree_for()) {
            když (child->schema()->nodetype() == LYS_LEAF) {
                libyang::Schema_Node_Leaf leafSchema(child->schema());
                když (leafSchema.is_key()) {
                    auto leafData = std::make_shared<libyang::Data_Node_Leaf_List>(child);
                    instance.insert({leafSchema.name(), leafValueFromNode(leafData)});
                }
            }
        }
        res.emplace_back(instance);
    }
    vrať res;
}

std::string YangAccess::dump(neměnné DataFormat format) neměnné
{
    znak* output;
    lyd_print_mem(&output, m_datastore.get(), format == DataFormat::Xml ? LYD_XML : LYD_JSON, LYP_WITHSIBLINGS | LYP_FORMAT);
    std::unique_ptr<znak, decltype(&osvoboď)> deleter{output, osvoboď};

    když (output) {
        std::string res = output;
        vrať res;
    }

    vrať "";
}

prázdno YangAccess::loadModule(neměnné std::string& name)
{
    m_schema->loadModule(name);
}

prázdno YangAccess::addSchemaFile(neměnné std::string& path)
{
    m_schema->addSchemaFile(path.c_str());
}

prázdno YangAccess::addSchemaDir(neměnné std::string& path)
{
    m_schema->addSchemaDirectory(path.c_str());
}

prázdno YangAccess::enableFeature(neměnné std::string& module, neměnné std::string& feature)
{
    m_schema->enableFeature(module, feature);
}

prázdno YangAccess::addDataFile(neměnné std::string& path)
{
    std::ifstream fs(path);
    znak firstChar;
    fs >> firstChar;

    std::cout << "Parsing \"" << path << "\" as " << (firstChar == '{' ? "JSON" : "XML") << "...\n";
    auto dataNode = lyd_parse_path(m_ctx.get(), path.c_str(), firstChar == '{' ? LYD_JSON : LYD_XML, LYD_OPT_DATA | LYD_OPT_DATA_NO_YANGLIB | LYD_OPT_TRUSTED);

    když (!dataNode) {
        throw std::runtime_error("Supplied data file " + path + " couldn't be parsed.");
    }

    když (!m_datastore) {
        m_datastore = lyWrap(dataNode);
    } jinak {
        lyd_merge(m_datastore.get(), dataNode, LYD_OPT_DESTRUCT);
    }

    validate();
}
