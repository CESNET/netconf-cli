/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/

#include "trompeloeil_doctest.hpp"
#include "parser.hpp"
#include "pretty_printers.hpp"
#include "static_schema.hpp"

TEST_CASE("command completion")
{
    auto schema = std::make_shared<StaticSchema>();
    Parser parser(schema);
    std::string input;
    std::ostringstream errorStream;
    std::set<std::string> expectedCompletions;
    int expectedContextLength;
    SECTION("no prefix")
    {
        expectedCompletions = {"cd", "copy", "create", "delete", "set", "commit", "get", "ls", "discard", "help", "describe", "move", "dump"};
        expectedContextLength = 0;
        SECTION("no space") {
            input = "";
        }
        SECTION("space") {
            input = " ";
        }
    }

    SECTION("c")
    {
        input = "c";
        expectedCompletions = {"cd", "commit", "copy", "create"};
        expectedContextLength = 1;
    }

    SECTION("d")
    {
        input = "d";
        expectedCompletions = {"delete", "discard", "describe", "dump"};
        expectedContextLength = 1;
    }

    SECTION("x")
    {
        input = "x";
        expectedCompletions = {};
        expectedContextLength = 1;
    }

    SECTION("cd")
    {
        input = "cd";
        expectedCompletions = {"cd "};
        expectedContextLength = 2;
    }

    SECTION("create")
    {
        input = "create";
        expectedCompletions = {"create "};
        expectedContextLength = 6;
    }

    SECTION("copy datastores")
    {
        input = "copy ";
        expectedCompletions = {"running", "startup"};
        expectedContextLength = 0;
    }

    REQUIRE(parser.completeCommand(input, errorStream) == (Completions{expectedCompletions, expectedContextLength}));
}
