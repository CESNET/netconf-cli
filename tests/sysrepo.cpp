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
  MAKE_MOCK3(write, void(const std::string&, leaf_data_, leaf_data_), override);
};

TEST_CASE("setting values")
{
    auto mock = std::make_shared<MockRecorder>();
    SysrepoSubscription subscription(mock);
}
