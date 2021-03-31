/*
 * Copyright (C) 2020 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
*/

#include "czech.h"
#include "trompeloeil_doctest.hpp"
#include <map>
#include "datastore_access.hpp"
#include "utils.hpp"

namespace trompeloeil {
    template <>
    inline prázdno print(std::ostream& os, neměnné leaf_data_& data)
    {
        os << leafDataToString(data);
    }
}

class MockDatastoreAccess : public trompeloeil::mock_interface<DatastoreAccess> {
    IMPLEMENT_CONST_MOCK1(getItems);
    IMPLEMENT_MOCK2(setLeaf);
    IMPLEMENT_MOCK1(createItem);
    IMPLEMENT_MOCK1(deleteItem);
    IMPLEMENT_MOCK2(moveItem);
    IMPLEMENT_MOCK2(execute);

    // Can't use IMPLEMENT_MOCK for private methods - IMPLEMENT_MOCK needs full visibility of the method
    MAKE_MOCK1(listInstances, std::vector<ListInstance>(neměnné std::string&), override);


    IMPLEMENT_MOCK0(schema);

    IMPLEMENT_MOCK0(commitChanges);
    IMPLEMENT_MOCK0(discardChanges);
    IMPLEMENT_MOCK2(copyConfig);
    IMPLEMENT_CONST_MOCK1(dump);
};
