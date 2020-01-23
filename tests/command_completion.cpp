/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/

#include "trompeloeil_doctest.h"
#include "parser.hpp"
#include "static_schema.hpp"

TEST_CASE("command completion")
{
    auto schema = std::make_shared<StaticSchema>();
    Parser parser(schema);
    std::string input;
    std::ostringstream errorStream;
    std::set<std::string> expected;
    SECTION("")
    {
        input = "";
        expected = {"cd", "create", "delete", "set", "commit", "get", "ls", "discard", "help"};
    }

    SECTION(" ")
    {
        input = " ";
        expected = {"cd", "create", "delete", "set", "commit", "get", "ls", "discard", "help"};
    }

    SECTION("c")
    {
        input = "c";
        expected = {"cd", "commit", "create"};
    }

    SECTION("d")
    {
        input = "d";
        expected = {"delete", "discard"};
    }

    SECTION("x")
    {
        input = "x";
        expected = {};
    }

    SECTION("cd")
    {
        input = "cd";
        // TODO: depending on how Readline works, this will have to be changed to include a space
        expected = {"cd"};
    }

    SECTION("create")
    {
        input = "create";
        expected = {"create"};
    }

    REQUIRE(parser.completeCommand(input, errorStream).m_completions == expected);
}
