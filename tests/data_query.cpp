/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/

#include "trompeloeil_doctest.h"

#ifdef sysrepo_BACKEND
#include "sysrepo_access.hpp"
#elif defined(netconf_BACKEND)
#include "netconf_access.hpp"
#include "netopeer_vars.hpp"
#else
#error "Unknown backend"
#endif
#include "sysrepo_subscription.hpp"
#include "utils.hpp"

class MockRecorder : public Recorder {
public:
    MAKE_MOCK3(write, void(const std::string&, const std::string&, const std::string&), override);
};

namespace std {
std::ostream& operator<<(std::ostream& s, const std::map<std::string, leaf_data_> map)
{
    s << std::endl
      << "{";
    for (const auto& it : map) {
        s << "{\"" << it.first << "\", " << leafDataToString(it.second) << "}" << std::endl;
    }
    s << "}" << std::endl;
    return s;
}
}

TEST_CASE("setting/getting values")
{
    trompeloeil::sequence seq1;

#ifdef sysrepo_BACKEND
    SysrepoAccess datastore("netconf-cli-test");
#elif defined(netconf_BACKEND)
    NetconfAccess datastore(NETOPEER_SOCKET_PATH);
#else
#error "Unknown backend"
#endif

    DataQuery dataquery(datastore);

    SECTION("listKeys")
    {
        schemaPath_ path;
        std::set<std::string> expected;
        SECTION("example-schema:person")
        {
            datastore.createListInstance("/example-schema:person[name='ahoj']");
            datastore.createListInstance("/example-schema:person[name='CAU']");
            datastore.createListInstance("/example-schema:person[name='c u s']");
            path.m_nodes.push_back(schemaNode_{module_{"example-schema"}, list_("person")});
            dataquery.listKeys("example-schema:person");
            expected = {"ahoj", "CAU", "c u s"};
        }

        REQUIRE(dataquery.listKeys(path) == expected);
    }

    waitForCompletionAndBitMore(seq1);
}
