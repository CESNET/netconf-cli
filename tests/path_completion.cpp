/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/

#include "trompeloeil_doctest.h"
#include "ast_commands.hpp"
#include "parser.hpp"
#include "static_schema.hpp"

TEST_CASE("path_completion")
{
    auto schema = std::make_shared<StaticSchema>();
    schema->addModule("example");
    schema->addModule("second");
    schema->addContainer("", "example:ano");
    schema->addContainer("", "example:anoda");
    schema->addList("example:ano", "example:listInCont", {"number"});
    schema->addContainer("", "second:amelie");
    schema->addContainer("", "example:bota");
    schema->addContainer("example:ano", "example:a2");
    schema->addContainer("example:bota", "example:b2");
    schema->addContainer("example:ano/example:a2", "example:a3");
    schema->addContainer("example:bota/example:b2", "example:b3");
    schema->addList("", "example:list", {"number"});
    schema->addContainer("example:list", "example:contInList");
    schema->addList("", "example:twoKeyList", {"number", "name"});
    Parser parser(schema);
    std::string input;
    std::ostringstream errorStream;
    std::set<std::string> expected;

    SECTION("node name completion")
    {
        SECTION("ls ")
        {
            input = "ls ";
            expected = {"example:ano", "example:anoda", "example:bota", "second:amelie", "example:list", "example:twoKeyList"};
        }

        SECTION("ls e")
        {
            input = "ls e";
            expected = {"example:ano", "example:anoda", "example:bota", "example:list", "example:twoKeyList"};
        }

        SECTION("ls example:ano")
        {
            input = "ls example:ano";
            expected = {"example:ano", "example:anoda"};
        }

        SECTION("ls example:ano/example:a")
        {
            input = "ls example:ano/example:a";
            expected = {"example:a2"};
        }

        SECTION("ls x")
        {
            input = "ls x";
            expected = {};
        }

        SECTION("ls /")
        {
            input = "ls /";
            expected = {"example:ano", "example:anoda", "example:bota", "second:amelie", "example:list", "example:twoKeyList"};
        }

        SECTION("ls /e")
        {
            input = "ls /e";
            expected = {"example:ano", "example:anoda", "example:bota", "example:list", "example:twoKeyList"};
        }

        SECTION("ls /s")
        {
            input = "ls /s";
            expected = {"second:amelie"};
        }

        SECTION("ls /example:list[number=3]/")
        {
            input = "ls /example:list[number=3]/";
            expected = {"example:contInList"};
        }

        SECTION("ls /example:list[number=3]/c")
        {
            input = "ls /example:list[number=3]/e";
            expected = {"example:contInList"};
        }

        SECTION("ls /example:list[number=3]/a")
        {
            input = "ls /example:list[number=3]/a";
            expected = {};
        }
    }

    SECTION("list keys completion")
    {
        SECTION("cd example:lis")
        {
            input = "cd example:lis";
            expected = {"example:list"};
        }

        SECTION("set example:list")
        {
            input = "set example:list";
            expected = {"example:list"};
        }

        SECTION("cd example:list[")
        {
            input = "cd example:list[";
            expected = {"number="};
        }

        SECTION("cd example:list[numb")
        {
            input = "cd example:list[numb";
            expected = {"number="};
        }

        SECTION("cd example:list[number")
        {
            input = "cd example:list[number";
            expected = {"number="};
        }

        SECTION("cd example:list[number=12")
        {
            input = "cd example:list[number=12";
            expected = {"]/"};
        }

        SECTION("cd example:list[number=12]")
        {
            input = "cd example:list[number=12]";
            expected = {"]/"};
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

        SECTION("cd example:twoKeyList[name=\"AHOJ\"][number=123")
        {
            input = "cd example:twoKeyList[name=\"AHOJ\"][number=123";
            expected = {"]/"};
        }

        SECTION("cd example:twoKeyList[name=\"AHOJ\"][number=123]")
        {
            input = "cd example:twoKeyList[name=\"AHOJ\"][number=123]";
            expected = {"]/"};
        }
    }

    REQUIRE(parser.completeCommand(input, errorStream) == expected);
}
