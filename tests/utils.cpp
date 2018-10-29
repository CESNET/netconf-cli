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
    SECTION("filterAndErasePrefix")
    {
        std::set<std::string> set{"ahoj", "coze", "copak", "aha", "polivka"};

        REQUIRE((filterAndErasePrefix(set, "a") == std::set<std::string>{"hoj", "ha"}));
        REQUIRE((filterAndErasePrefix(set, "ah") == std::set<std::string>{"oj", "a"}));
        REQUIRE((filterAndErasePrefix(set, "aho") == std::set<std::string>{"j"}));
        REQUIRE((filterAndErasePrefix(set, "polivka") == std::set<std::string>{""}));
        REQUIRE((filterAndErasePrefix(set, "polivka") == std::set<std::string>{""}));
        REQUIRE((filterAndErasePrefix(set, "polivkax") == std::set<std::string>{}));
        REQUIRE((filterAndErasePrefix(set, "co") == std::set<std::string>{"pak", "ze"}));
    }

}
