/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/

#include "trompeloeil_catch.h"

#include "sysrepo_access.hpp"
#include "sysrepo_subscription.hpp"
#include "sysrepoctl.hpp"

class MockRecorder : public Recorder
{
public:
    MAKE_MOCK3(write, void(const std::string&, const std::string&, const std::string&), override);
};

TEST_CASE("setting values")
{
    system(SYSREPOCTLEXECUTABLE " --uninstall --module \"example-schema\" > /dev/null");
    system(SYSREPOCTLEXECUTABLE " --install --yang \"example-schema.yang\" > /dev/null");
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
