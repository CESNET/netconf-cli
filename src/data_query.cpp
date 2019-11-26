#include "data_query.hpp"
#include "datastore_access.hpp"
#include "schema.hpp"
#include "utils.hpp"

DataQuery::DataQuery(DatastoreAccess& datastore)
    : m_datastore(datastore)
{
    m_schema = m_datastore.schema();
}

std::string keyValueToString(const leaf_data_& value)
{
    if (value.type() == typeid(std::string)) {
        return "'" + leafDataToString(value) + "'";
    }
    return leafDataToString(value);
}

std::set<keyValue_> DataQuery::listKeys(const dataPath_& location, const ModuleNodePair& node) const
{
    std::set<keyValue_> res;

    for (const auto& key : m_schema->listKeys(dataPathToSchemaPath(location), node)) {
        auto pathToKey = joinPaths(joinPaths(pathToDataString(location), fullNodeName(location, node)), key);
        for (const auto& value : m_datastore.getItems(pathToKey)) {
            res.insert({key, keyValueToString(value.second)});
        }
    }

    return res;
}
