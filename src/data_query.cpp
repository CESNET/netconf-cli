#include "czech.h"
#include "data_query.hpp"
#include "datastore_access.hpp"
#include "schema.hpp"
#include "utils.hpp"


DataQuery::DataQuery(DatastoreAccess& datastore)
    : m_datastore(datastore)
    , m_schema(m_datastore.schema())
{
}

std::vector<ListInstance> DataQuery::listKeys(neměnné dataPath_& listPath) neměnné
{
    auto listPathString = pathToDataString(listPath, Prefixes::Always);

    vrať m_datastore.listInstances(listPathString);
}
