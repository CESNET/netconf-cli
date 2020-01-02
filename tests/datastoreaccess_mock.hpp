/*
 * Copyright (C) 2020 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
*/

#include <map>
#include "datastore_access.hpp"
#include "trompeloeil_doctest.h"
#include "utils.hpp"

// https://github.com/rollbear/trompeloeil/blob/master/docs/FAQ.md#return_template
using GetItemsType = std::map<std::string, leaf_data_>(const std::string&);
using SchemaType = std::shared_ptr<Schema>();
namespace trompeloeil {
    template <>
    inline void print(std::ostream& os, const leaf_data_& data)
    {
        os << leafDataToString(data);
    }
}

class MockDatastoreAccess : public trompeloeil::mock_interface<DatastoreAccess> {
    IMPLEMENT_MOCK1(getItems);
    IMPLEMENT_MOCK2(setLeaf);
    IMPLEMENT_MOCK1(createPresenceContainer);
    IMPLEMENT_MOCK1(deletePresenceContainer);
    IMPLEMENT_MOCK1(createListInstance);
    IMPLEMENT_MOCK1(deleteListInstance);

    // Can't use IMPLEMENT_MOCK for private methods - IMPLEMENT_MOCK needs full visibility of the method
    MAKE_MOCK1(listInstances, std::vector<ListInstance>(const std::string&), override);


    IMPLEMENT_MOCK0(schema);

    IMPLEMENT_MOCK0(commitChanges);
    IMPLEMENT_MOCK0(discardChanges);
};


