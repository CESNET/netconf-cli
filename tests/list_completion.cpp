/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/

#include "trompeloeil_catch.h"
#include <iostream>
#include "ast_commands.hpp"
#include "parser.hpp"
#include "static_schema.hpp"

TEST_CASE("path_completion")
{
    auto schema = std::make_shared<StaticSchema>();
    schema->addModule("example");
    schema->addContainer("", "example:ano");
    schema->addList("example:ano", "example:listInCont", {"number"});
    schema->addList("", "example:list", {"number"});
    schema->addContainer("example:list", "example:contInList");
    schema->addList("", "example:twoKeyList", {"number", "name"});
    Parser parser(schema);
    std::string input;
    std::ostringstream errorStream;
    std::set<std::string> expected;

    SECTION("cd example:lis")
    {
        input = "cd example:lis";
        expected = {"t"};
    }

    SECTION("cd example:list[")
    {
        input = "cd example:list[";
        expected = {"number="};
    }

    SECTION("cd example:list[number=12]")
    {
        input = "cd example:list[number=12]";
        expected = {};
    }

    SECTION("cd example:twoKeyList[")
    {
        input = "cd example:twoKeyList[";
        expected = {"name=", "number="};
    }

    SECTION("cd example:twoKeyList[name=\"AHOJ\"][")
    {
        input = "cd example:twoKeyList[name=\"AHOJ\"][";
        expected = {"number="};
    }

    SECTION("cd example:twoKeyList[number=42][")
    {
        input = "cd example:twoKeyList[number=42][";
        expected = {"name="};
    }

    SECTION("cd example:twoKeyList[name=\"AHOJ\"][number=123]")
    {
        input = "cd example:twoKeyList[name=\"AHOJ\"][number=123]";
        expected = {};
    }

    REQUIRE(parser.completeCommand(input, errorStream) == expected);
}
