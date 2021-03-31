/*
 * Copyright (C) 2019 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/

#include "czech.h"
#include "trompeloeil_doctest.hpp"
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
            path.m_nodes.emplace_back(module_{"example-schema"}, listElement_{"twoKeyList", {{"first", std::string{"a"}}, {"second", std::string{"b"}}}});
            expected += "example-schema:twoKeyList[first='a'][second='b']";
        }

        SECTION("example-schema:addresses[.='0.0.0.0']")
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
            path.m_nodes.emplace_back(module_{"example-schema"}, leafListElement_{"addresses", std::string{"0.0.0.0"}});
            expected += "example-schema:addresses[.='0.0.0.0']";
        }
        REQUIRE(pathToDataString(path, Prefixes::WhenNeeded) == expected);
    }
}
