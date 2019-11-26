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

std::map<std::string, std::set<std::string>> DataQuery::listKeys(const dataPath_& location, const ModuleNodePair& node) const
{
    std::map<std::string, std::set<std::string>> res;

    for (const auto& key : m_schema->listKeys(dataPathToSchemaPath(location), node)) {
        auto pathToKey = joinPaths(joinPaths(pathToDataString(location), fullNodeName(location, node)), key);
        std::set<std::string> values;
        for (const auto& value : m_datastore.getItems(pathToKey)) {
            values.insert(keyValueToString(value.second));
        }
        res.insert({key, values});
    }

    return res;
}
