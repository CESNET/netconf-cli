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
    // Only string-like values need to be quoted
    if (value.type() == typeid(std::string) ||
            value.type() == typeid(enum_) ||
            value.type() == typeid(binary_) ||
            value.type() == typeid(identityRef_) ||
            value.type() == typeid(bool)) {
        return escapeListKeyString(leafDataToString(value));
    }
    return leafDataToString(value);
}

std::set<std::map<std::string, std::string>> DataQuery::listKeys(const dataPath_& location, const ModuleNodePair& node) const
{
    std::set<std::map<std::string, std::string>> res;
    auto listPath = joinPaths(pathToDataString(location, Prefixes::Always), fullNodeName(location, node));

    auto lists = m_datastore.getItems(listPath);

    decltype(lists) allInstances;
    std::copy_if(lists.begin(), lists.end(), std::inserter(allInstances, allInstances.end()), [] (const auto& item) {
        return item.second.type() == typeid(special_) && boost::get<special_>(item.second).m_value == SpecialValue::List;
    });

    auto keys = m_schema->listKeys(dataPathToSchemaPath(location), node);
    for (const auto& instance : allInstances) {
        std::map<std::string, std::string> x;
        for (const auto& key : keys) {
            x.insert({key, keyValueToString(lists.at(joinPaths(instance.first, key)))});
        }
        res.insert(x);
    }


    return res;
}
