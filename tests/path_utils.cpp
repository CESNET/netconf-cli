/*
 * Copyright (C) 2019 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/

#include "trompeloeil_doctest.h"
#include "ast_path.hpp"

TEST_CASE("path utils")
{
    SECTION("pathToDataString")
    {
        dataPath_ path;
        std::string expected;
        SECTION("example-schema:twoKeyList[first='a'][second='b']")
        {
            SECTION("absolute")
            {
                path.m_scope = Scope::Absolute;
                expected += "/";
            }
            SECTION("relative")
            {
                path.m_scope = Scope::Relative;
            }
            path.m_nodes.push_back(dataNode_{module_{"example-schema"}, listElement_{"twoKeyList", {{"first", "a"}, {"second", "b"}}}});
            expected += "example-schema:twoKeyList[first='a'][second='b']";
        }
        REQUIRE(pathToDataString(path) == expected);
    }
}
