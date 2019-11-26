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

std::set<keyValue_> DataQuery::listKeys(const schemaPath_& path) const
{
    std::set<keyValue_> res;
    for (const auto& keyInfo : m_schema->listKeyInfo(path)) {
        for (const auto& value : m_datastore.getItems(keyInfo.m_path)) {
            res.insert({keyInfo.m_leafName, keyValueToString(value.second)});
        }
    }

    return res;
}
