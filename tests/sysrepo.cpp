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

class MockRecorder : public Recorder
{
public:
  MAKE_MOCK3(write, void(const std::string&, const std::string&, const std::string&), override);
};

TEST_CASE("setting values")
{
    MockRecorder mock;
    SysrepoSubscription subscription(&mock);
    SysrepoAccess datastore("netconf-cli-test");

    REQUIRE_CALL(mock, write("/example-schema:leafInt", "", "123"));
    datastore.setLeaf("/example-schema:leafInt", 123);
    datastore.commitChanges();


}
