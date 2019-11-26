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
    auto listPath = joinPaths(pathToDataString(location, Prefixes::Always), fullNodeName(location, node));

    return m_datastore.listInstances(listPath);
}
