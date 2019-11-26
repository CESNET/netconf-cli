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
#include <memory>
#include <map>
#include <set>

class DatastoreAccess;
class Schema;
struct dataPath_;

using keyValue_ = std::pair<std::string, std::string>;
using ModuleNodePair = std::pair<boost::optional<std::string>, std::string>;
class DataQuery {
public:
    DataQuery(DatastoreAccess& datastore);
    /*! \brief Lists all possible key/value pairs for a list specified by the arguments.
     *  \param location Location of the list.
     *  \param node Name (and optional module name) of the list.
     */
    std::map<std::string, std::set<std::string>> listKeys(const dataPath_& location, const ModuleNodePair& node) const;

private:
    DatastoreAccess& m_datastore;
    std::shared_ptr<Schema> m_schema;
};
