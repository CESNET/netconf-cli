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

class MockRecorder : public Recorder {
public:
    MAKE_MOCK3(write, void(const std::string&, const std::string&, const std::string&), override);
};

TEST_CASE("setting values")
{
    trompeloeil::sequence seq1;
    MockRecorder mock;
    SysrepoSubscription subscription(&mock);

#ifdef sysrepo_BACKEND
    SysrepoAccess datastore("netconf-cli-test");
#elif defined(netconf_BACKEND)
    NetconfAccess datastore(NETOPEER_SOCKET_PATH);
#else
#error "Unknown backend"
#endif

    SECTION("set leafInt to 123")
    {
        REQUIRE_CALL(mock, write("/example-schema:leafInt", "", "123"));
        datastore.setLeaf("/example-schema:leafInt", 123);
        datastore.commitChanges();
    }

    SECTION("set leafEnum to coze")
    {
        REQUIRE_CALL(mock, write("/example-schema:leafEnum", "", "coze"));
        datastore.setLeaf("/example-schema:leafEnum", enum_{"coze"});
        datastore.commitChanges();
    }

    SECTION("set leafDecimal to 123.544")
    {
        REQUIRE_CALL(mock, write("/example-schema:leafDecimal", "", "123.544"));
        datastore.setLeaf("/example-schema:leafDecimal", 123.544);
        datastore.commitChanges();
    }

    SECTION("create presence container")
    {
        REQUIRE_CALL(mock, write("/example-schema:pContainer", "", ""));
        datastore.createPresenceContainer("/example-schema:pContainer");
        datastore.commitChanges();
    }

    SECTION("create a list instance")
    {
        REQUIRE_CALL(mock, write("/example-schema:person[name='Nguyen']", "", ""));
        REQUIRE_CALL(mock, write("/example-schema:person[name='Nguyen']/name", "", "Nguyen"));
        datastore.createListInstance("/example-schema:person[name='Nguyen']");
        datastore.commitChanges();
    }

    waitForCompletionAndBitMore(seq1);
}
