#include "data_query.hpp"
#include "datastore_access.hpp"
#include "schema.hpp"
#include "utils.hpp"


DataQuery::DataQuery(DatastoreAccess& datastore)
    : m_datastore(datastore)
{
    m_schema = m_datastore.schema();
}

std::vector<std::map<std::string, leaf_data_>> DataQuery::listKeys(const dataPath_& location, const ModuleNodePair& node) const
{
    std::vector<std::map<std::string, leaf_data_>> res;
    auto listPath = joinPaths(pathToDataString(location, Prefixes::Always), fullNodeName(location, node));

    auto lists = m_datastore.getItems(listPath);

    decltype(lists) allInstances;
    std::copy_if(lists.begin(), lists.end(), std::inserter(allInstances, allInstances.end()), [] (const auto& item) {
        return item.second.type() == typeid(special_) && boost::get<special_>(item.second).m_value == SpecialValue::List;
    });

    auto keys = m_schema->listKeys(dataPathToSchemaPath(location), node);
    for (const auto& instance : allInstances) {
        std::map<std::string, leaf_data_> x;
        for (const auto& key : keys) {
            x.insert({key, lists.at(joinPaths(instance.first, key))});
        }
        res.push_back(x);
    }


    return res;
}
