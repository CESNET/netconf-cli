/*
 * Copyright (C) 2019 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
*/

#pragma once

/*! \class DataQuery
 *  \brief A class used for querying data from a datastore for usage in tab-completion.
 *
*/

#include <boost/optional.hpp>
#include <map>
#include <memory>
#include <set>
#include "ast_values.hpp"

class DatastoreAccess;
class Schema;
struct dataPath_;

using ModuleNodePair = std::pair<boost::optional<std::string>, std::string>;

class DataQuery {
public:
    DataQuery(DatastoreAccess& datastore);
    /*! \brief Lists all possible instances of key/value pairs for a list specified by the arguments.
     *  \param location Location of the list.
     *  \param node Name (and optional module name) of the list.
     *  \return A vector of maps, which represent the instances. The map is keyed by the name of the list key. The values in the map, are values of the list keys.
     */
    std::vector<std::map<std::string, leaf_data_>> listKeys(const dataPath_& location, const ModuleNodePair& node) const;

private:
    DatastoreAccess& m_datastore;
    std::shared_ptr<Schema> m_schema;
};
