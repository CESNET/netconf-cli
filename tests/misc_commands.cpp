/*
 * Copyright (C) 2021 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubervac@cesnet.cz>
 *
*/

#include "trompeloeil_doctest.hpp"
#include "parser.hpp"
#include "pretty_printers.hpp"
#include "static_schema.hpp"

TEST_CASE("miscellaneous commands")
{
    auto schema = std::make_shared<StaticSchema>();
    Parser parser(schema);
    std::ostringstream errorStream;
    SECTION("switch")
    {
        std::string input;
        switch_ expected;
        SECTION("operational")
        {
            expected.m_target = DatastoreTarget::Operational;
            input = "switch operational";
        }

        SECTION("running")
        {
            expected.m_target = DatastoreTarget::Running;
            input = "switch running";
        }

        SECTION("startup")
        {
            expected.m_target = DatastoreTarget::Startup;
            input = "switch startup";
        }
        REQUIRE(boost::get<switch_>(parser.parseCommand(input, errorStream)) == expected);
    }

    SECTION("help")
    {
        std::string input;
        bool expectedCommand = false;
        SECTION("Short help")
        {
            input = "help";
        }
        SECTION("Short help with trailing whitespace")
        {
            input = "help ";
        }
        SECTION("Long help")
        {
            input = "help cd";
            expectedCommand = true;
        }
        SECTION("Long help with trailing whitespace")
        {
            input = "help cd ";
            expectedCommand = true;
        }

        auto result = boost::get<help_>(parser.parseCommand(input, errorStream));
        REQUIRE(!!result.m_cmd == expectedCommand);
    }
}
