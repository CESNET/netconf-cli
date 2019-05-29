
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

TEST_CASE("enum completion")
{
    auto schema = std::make_shared<StaticSchema>();
    schema->addModule("mod");
    schema->addContainer("", "mod:contA");
    schema->addLeafEnum("", "mod:leafEnum", {"lala", "lol", "data", "coze"});
    schema->addLeafEnum("mod:contA", "mod:leafInCont", {"abc", "def"});
    schema->addList("", "mod:list", {"number"});
    schema->addLeafEnum("mod:list", "mod:leafInList", {"ano", "anoda", "ne", "katoda"});
    Parser parser(schema);
    std::string input;
    std::ostringstream errorStream;

    std::set<std::string> expected;

    SECTION("set mod:leafEnum ")
    {
        input = "set mod:leafEnum ";
        expected = {"lala", "lol", "data", "coze"};
    }

    SECTION("set mod:leafEnum c")
    {
        input = "set mod:leafEnum c";
        expected = {"coze"};
    }

    SECTION("set mod:leafEnum l")
    {
        input = "set mod:leafEnum l";
        expected = {"lala", "lol"};
    }


    SECTION("set mod:contA/leafInCont ")
    {
        input = "set mod:contA/leafInCont ";
        expected = {"abc", "def"};
    }

    SECTION("set mod:list[number=42]/leafInList ")
    {
        input = "set mod:list[number=42]/leafInList ";
        expected = {"ano", "anoda", "ne", "katoda"};
    }

    REQUIRE(parser.completeCommand(input, errorStream) == expected);
}
