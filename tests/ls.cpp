/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/

#include "trompeloeil_catch.h"
#include "ast_commands.hpp"
#include "parser.hpp"
#include "static_schema.hpp"

TEST_CASE("ls")
{
    auto schema = std::make_shared<StaticSchema>();
    schema->addModule("example");
    schema->addModule("second");
    schema->addContainer("", "example:a");
    schema->addContainer("", "second:a");
    schema->addContainer("", "example:b");
    schema->addContainer("example:a", "example:a2");
    schema->addContainer("example:b", "example:b2");
    schema->addContainer("example:a/example:a2", "example:a3");
    schema->addContainer("example:b/example:b2", "example:b3");
    schema->addList("", "example:list", {"number"});
    schema->addContainer("example:list", "example:contInList");
    schema->addList("", "example:twoKeyList", {"number", "name"});
    Parser parser(schema);
    std::string input;
    std::ostringstream errorStream;

    SECTION("valid input")
    {
        ls_ expected;

        SECTION("no arguments")
        {
            input = "ls";
        }

        SECTION("with path argument")
        {
            input = "ls example:a";
            expected.m_path = path_{false, {node_(module_{"example"}, container_{"a"})}};
        }

        command_ command = parser.parseCommand(input, errorStream);
        REQUIRE(command.type() == typeid(ls_));
        REQUIRE(boost::get<ls_>(command) == expected);
    }
    SECTION("invalid input")
    {
        SECTION("invalid path")
        {
            input = "ls example:nonexistent";
        }

        REQUIRE_THROWS(parser.parseCommand(input, errorStream));
    }
}
