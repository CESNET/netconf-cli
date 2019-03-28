/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/

#include "trompeloeil_doctest.h"

#include "sysrepo_access.hpp"
#include "sysrepo_subscription.hpp"
#include "sysrepo_vars.hpp"

class MockRecorder : public Recorder {
public:
    MAKE_MOCK3(write, void(const std::string&, const std::string&, const std::string&), override);
};

TEST_CASE("setting values")
{
    if (system(SYSREPOCTLEXECUTABLE " --uninstall --module example-schema > /dev/null") != 0) {
        // uninstall can fail if it isn't already installed -> do nothing
        // This crappy comment is here due to https://gcc.gnu.org/bugzilla/show_bug.cgi?id=25509
    }
    REQUIRE(system(SYSREPOCTLEXECUTABLE " --install --yang " EXAMPLE_SCHEMA_LOCATION " > /dev/null") == 0);

    trompeloeil::sequence seq1;
    MockRecorder mock;
    SysrepoSubscription subscription(&mock);
    SysrepoAccess datastore("netconf-cli-test");

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

    waitForCompletionAndBitMore(seq1);
}
