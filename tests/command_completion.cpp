/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/

#include "trompeloeil_catch.h"
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
        expected = {"cd", "create", "delete", "set", "commit", "get", "ls", "discard"};
    }

    SECTION(" ")
    {
        input = " ";
        expected = {"cd", "create", "delete", "set", "commit", "get", "ls", "discard"};
    }

    SECTION("c")
    {
        input = "c";
        expected = {"d", "ommit", "reate"};
    }

    SECTION("d")
    {
        input = "d";
        expected = {"elete", "iscard"};
    }

    SECTION("x")
    {
        input = "x";
        expected = {};
    }

    SECTION("cd")
    {
        input = "cd";
        expected = {""};
    }

    SECTION("create")
    {
        input = "create";
        expected = {""};
    }

    REQUIRE(parser.completeCommand(input, errorStream) == expected);
}
