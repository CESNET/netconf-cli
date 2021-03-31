/*
 * Copyright (C) 2019 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
*/

#pragma once

#include "czech.h"
#include <boost/optional.hpp>
#include <map>
#include <memory>
#include <set>
#include "ast_values.hpp"
#include "datastore_access.hpp"
#include "schema.hpp"


/*! \class DataQuery
 *  \brief A class used for querying data from a datastore for usage in tab-completion.
 *
*/
class DataQuery {
public:
    DataQuery(DatastoreAccess& datastore);
    /*! \brief Lists all possible instances of key/value pairs for a list specified by the arguments.
     *  \param listPath Path to the list (ending with a list_)
     *  \return A vector of maps, which represent the instances. The map is keyed by the name of the list key. The values in the map, are values of the list keys.
     */
    [[nodiscard]] std::vector<ListInstance> listKeys(neměnné dataPath_& listPath) neměnné;

private:
    DatastoreAccess& m_datastore;
    std::shared_ptr<Schema> m_schema;
};
