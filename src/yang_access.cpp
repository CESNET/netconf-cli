#include <boost/algorithm/string/predicate.hpp>
#include <experimental/iterator>
#include <fstream>
#include <iostream>
#include <libyang-cpp/DataNode.hpp>
#include <libyang-cpp/Utils.hpp>
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
    : m_ctx(std::nullopt, libyang::ContextOptions::DisableSearchCwd | libyang::ContextOptions::SetPrivParsed | libyang::ContextOptions::NoYangLibrary)
    , m_datastore(std::nullopt)
    , m_schema(std::make_shared<YangSchema>(m_ctx))
{
}

YangAccess::YangAccess(std::shared_ptr<YangSchema> schema)
    : m_ctx(schema->m_context)
    , m_datastore(std::nullopt)
    , m_schema(schema)
{
}

YangAccess::~YangAccess() = default;

[[noreturn]] void YangAccess::getErrorsAndThrow() const
{
    std::vector<DatastoreError> errorsRes;

    for (const auto& err : m_ctx.getErrors()) {
        errorsRes.emplace_back(err.message, err.dataPath ? err.dataPath : err.schemaPath ? err.schemaPath : std::nullopt);
    }
    throw DatastoreException(errorsRes);
}

void YangAccess::impl_newPath(const std::string& path, const std::optional<std::string>& value)
{
    try {
        if (m_datastore) {
            m_datastore->newPath(path, value, libyang::CreationOptions::Update);
        } else {
            m_datastore = m_ctx.newPath(path, value, libyang::CreationOptions::Update);
        }
    } catch (libyang::Error&) {
        getErrorsAndThrow();
    }
}

namespace {
void impl_unlink(DatastoreType& datastore, libyang::DataNode what)
{
    // If the node to be unlinked is the one our datastore variable points to, we need to find a new one to point to (one of its siblings)

    if (datastore == what) {
        auto oldDatastore = datastore;
        do {
            datastore = datastore->previousSibling();
            if (datastore == oldDatastore) {
                // We have gone all the way back to our original node, which means it's the only node in our
                // datastore.
                datastore = std::nullopt;
                break;
            }
        } while (datastore->schema().module().name() == "ietf-yang-library");
    }

    what.unlink();
}
}

void YangAccess::impl_removeNode(const std::string& path)
{
    if (!m_datastore) {
        // Otherwise the datastore just doesn't contain the wanted node.
        throw DatastoreException{{DatastoreError{"Datastore is empty.", path}}};
    }
    auto toRemove = m_datastore->findPath(path);
    if (!toRemove) {
        // Otherwise the datastore just doesn't contain the wanted node.
        throw DatastoreException{{DatastoreError{"Data node doesn't exist.", path}}};
    }

    impl_unlink(m_datastore, *toRemove);
}

void YangAccess::validate()
{
    if (m_datastore) {
        libyang::validateAll(m_datastore);
    }
}

DatastoreAccess::Tree YangAccess::getItems(const std::string& path) const
{
    DatastoreAccess::Tree res;
    if (!m_datastore) {
        return res;
    }

    auto set = m_datastore->findXPath(path == "/" ? "/*" : path);

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
        auto set = m_sourceNode.findXPath(m_sourceNode.schema().path());
        if (set.size() == 1) { // m_sourceNode is the sole instance, do nothing
            return;
        }

        switch (absolute) {
        case yang::move::Absolute::Begin:
            if (set.front() == m_sourceNode) { // List is already at the beginning, do nothing
                return;
            }
            set.front().insertBefore(m_sourceNode);
            break;
        case yang::move::Absolute::End:
            if (set.back() == m_sourceNode) { // List is already at the end, do nothing
                return;
            }
            set.back().insertAfter(m_sourceNode);
            break;
        }
        m_datastore = m_datastore->firstSibling();
    }

    void operator()(const yang::move::Relative& relative) const
    {
        auto keySuffix = m_sourceNode.schema().nodeType() == libyang::NodeType::List ? instanceToString(relative.m_path)
                                                                    : leafDataToString(relative.m_path.at("."));
        auto destNode = m_sourceNode.findSiblingVal(m_sourceNode.schema(), keySuffix);

        if (relative.m_position == yang::move::Relative::Position::After) {
            destNode->insertAfter(m_sourceNode);
        } else {
            destNode->insertBefore(m_sourceNode);
        }
    }
};
}

void YangAccess::moveItem(const std::string& source, std::variant<yang::move::Absolute, yang::move::Relative> move)
{
    if (!m_datastore) {
        throw DatastoreException{{DatastoreError{"Datastore is empty.", source}}};
    }

    auto sourceNode = m_datastore->findPath(source);

    if (!sourceNode) {
        // The datastore doesn't contain the wanted node.
        throw DatastoreException{{DatastoreError{"Data node doesn't exist.", source}}};
    }
    std::visit(impl_moveItem{m_datastore, *sourceNode}, move);
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
    auto root = [&path, this]  {
        try {
            return m_ctx.newPath(path);
        } catch (libyang::ErrorWithCode& err) {
            getErrorsAndThrow();
        }
    }();

    for (const auto& [k, v] : input) {
        if (v.type() == typeid(special_) && boost::get<special_>(v).m_value != SpecialValue::PresenceContainer) {
            continue;
        }

        try {
            root.newPath(k, leafDataToString(v), libyang::CreationOptions::Update);
        } catch (libyang::ErrorWithCode& err) {
            getErrorsAndThrow();
        }
    }
    throw std::logic_error("in-memory datastore doesn't support executing RPC/action");
}

void YangAccess::copyConfig(const Datastore source, const Datastore dest)
{
    if (source == Datastore::Startup && dest == Datastore::Running) {
        m_datastore = std::nullopt;
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

    auto instances = m_datastore->findXPath(path);
    for (const auto& list : instances) {
        ListInstance instance;
        for (const auto& child : list.immediateChildren()) {
            if (child.schema().nodeType() == libyang::NodeType::Leaf) {
                auto leafSchema(child.schema().asLeaf());
                if (leafSchema.isKey()) {
                    instance.insert({leafSchema.name(), leafValueFromNode(child.asTerm())});
                }
            }
        }
        res.emplace_back(instance);
    }
    return res;
}

std::string YangAccess::dump(const DataFormat format) const
{
    if (!m_datastore) {
        return "";
    }

    auto str = m_datastore->firstSibling().printStr(format == DataFormat::Xml ? libyang::DataFormat::XML : libyang::DataFormat::JSON, libyang::PrintFlags::WithSiblings);
    if (!str) {
        return "";
    }

    return *str;
}

void YangAccess::loadModule(const std::string& name)
{
    m_schema->loadModule(name);
}

void YangAccess::addSchemaFile(const std::filesystem::path& path)
{
    m_schema->addSchemaFile(path);
}

void YangAccess::addSchemaDir(const std::filesystem::path& path)
{
    m_schema->addSchemaDirectory(path);
}

void YangAccess::setEnabledFeatures(const std::string& module, const std::vector<std::string>& features)
{
    m_schema->setEnabledFeatures(module, features);
}

void YangAccess::addDataFile(const std::string& path, const StrictDataParsing strict)
{
    std::ifstream fs(path);
    char firstChar;
    fs >> firstChar;

    std::cout << "Parsing \"" << path << "\" as " << (firstChar == '{' ? "JSON" : "XML") << "...\n";

    auto dataNode = m_ctx.parseData(
            std::filesystem::path{path},
            firstChar == '{' ? libyang::DataFormat::JSON : libyang::DataFormat::XML,
            strict == StrictDataParsing::Yes ? std::optional{libyang::ParseOptions::Strict} : std::nullopt,
            libyang::ValidationOptions::Present);

    if (!m_datastore) {
        m_datastore = dataNode;
    } else {
        m_datastore->merge(*dataNode);
    }

    validate();
}
