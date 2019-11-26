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

#include <memory>
#include <set>

class DatastoreAccess;
class Schema;
struct schemaPath_;

using keyValue_ = std::pair<std::string, std::string>;
class DataQuery {
public:
    DataQuery(DatastoreAccess& datastore);
    std::set<keyValue_> listKeys(const schemaPath_& path) const;

private:
    DatastoreAccess& m_datastore;
    std::shared_ptr<Schema> m_schema;
};
