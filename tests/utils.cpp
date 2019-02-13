/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/

#include "trompeloeil_catch.h"
#include "utils.hpp"

TEST_CASE("utils")
{
    SECTION("filterByPrefix")
    {
        std::set<std::string> set{"ahoj", "coze", "copak", "aha", "polivka"};

        REQUIRE((filterByPrefix(set, "a") == std::set<std::string>{"ahoj", "aha"}));
        REQUIRE((filterByPrefix(set, "ah") == std::set<std::string>{"ahoj", "aha"}));
        REQUIRE((filterByPrefix(set, "aho") == std::set<std::string>{"ahoj"}));
        REQUIRE((filterByPrefix(set, "polivka") == std::set<std::string>{"polivka"}));
        REQUIRE((filterByPrefix(set, "polivkax") == std::set<std::string>{}));
        REQUIRE((filterByPrefix(set, "co") == std::set<std::string>{"copak", "coze"}));
    }

}
