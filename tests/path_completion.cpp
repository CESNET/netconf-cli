/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/

#include <experimental/iterator>
#include "trompeloeil_doctest.h"
#include "parser.hpp"
#include "static_schema.hpp"

namespace std {
std::ostream& operator<<(std::ostream& s, const std::set<std::string> set)
{
    s << std::endl << "{";
    std::copy(set.begin(), set.end(), std::experimental::make_ostream_joiner(s, ", "));
    s << "}" << std::endl;
    return s;
}
}

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
    schema->addList("", "example:ovoce", {"name"});
    schema->addList("", "example:ovocezelenina", {"name"});
    schema->addContainer("example:list", "example:contInList");
    schema->addList("", "example:twoKeyList", {"number", "name"});
    schema->addLeaf("", "example:leafInt", yang::LeafDataTypes::Int32);
    Parser parser(schema);
    std::string input;
    std::ostringstream errorStream;
    std::set<std::string> expected;

    SECTION("node name completion")
    {
        SECTION("ls ")
        {
            input = "ls ";
            expected = {"example:ano/", "example:anoda/", "example:bota/", "example:leafInt ", "example:list[", "example:ovoce[", "example:ovocezelenina[", "example:twoKeyList[", "second:amelie/"};
        }

        SECTION("ls e")
        {
            input = "ls e";
            expected = {"example:ano/", "example:anoda/", "example:bota/", "example:leafInt ", "example:list[", "example:ovoce[", "example:ovocezelenina[", "example:twoKeyList["};
        }

        SECTION("ls example:ano")
        {
            input = "ls example:ano";
            expected = {"example:ano/", "example:anoda/"};
        }

        SECTION("ls example:ano/example:a")
        {
            input = "ls example:ano/example:a";
            expected = {"example:a2/"};
        }

        SECTION("ls x")
        {
            input = "ls x";
            expected = {};
        }

        SECTION("ls /")
        {
            input = "ls /";
            expected = {"example:ano/", "example:anoda/", "example:bota/", "example:leafInt ", "example:list[", "example:ovoce[", "example:ovocezelenina[", "example:twoKeyList[", "second:amelie/"};
        }

        SECTION("ls /e")
        {
            input = "ls /e";
            expected = {"example:ano/", "example:anoda/", "example:bota/", "example:leafInt ", "example:list[", "example:ovoce[", "example:ovocezelenina[", "example:twoKeyList["};

        }

        SECTION("ls example:bota")
        {
            input = "ls example:bota";
            expected = {"example:bota/"};
        }

        SECTION("ls /example:bota")
        {
            input = "ls /example:bota";
            expected = {"example:bota/"};
        }

        SECTION("ls /s")
        {
            input = "ls /s";
            expected = {"second:amelie/"};
        }

        SECTION("ls /example:list[number=3]/")
        {
            input = "ls /example:list[number=3]/";
            expected = {"example:contInList/"};
        }

        SECTION("ls /example:list[number=3]/c")
        {
            input = "ls /example:list[number=3]/e";
            expected = {"example:contInList/"};
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
            expected = {"example:list["};
        }

        SECTION("set example:list")
        {
            input = "set example:list";
            expected = {"example:list["};
        }

        SECTION("cd example:list")
        {
            input = "cd example:list";
            expected = {"example:list["};
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

        SECTION("cd example:twoKeyList[name=\"AHOJ\"")
        {
            input = "cd example:twoKeyList[name=\"AHOJ\"";
            expected = {"]["};
        }

        SECTION("cd example:twoKeyList[name=\"AHOJ\"]")
        {
            input = "cd example:twoKeyList[name=\"AHOJ\"]";
            expected = {"]["};
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

        SECTION("cd example:ovoce")
        {
            input = "cd example:ovoce";
            expected = {"example:ovoce[", "example:ovocezelenina["};
        }
    }

    SECTION("clear completions when no longer inputting path")
    {
        input = "set example:leafInt ";
        expected = {};
    }

    REQUIRE(parser.completeCommand(input, errorStream) == expected);
}
