/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/

#include "trompeloeil_catch.h"
#include "ast.hpp"
#include "parser.hpp"
#include "schema.hpp"

TEST_CASE("create")
{
    Schema schema;
    schema.addContainer("", "a", true);
    schema.addContainer("", "b");
    schema.addContainer("a", "a2");
    schema.addContainer("b", "b2", true);
    schema.addList("", "list", {"quote"});
    schema.addContainer("list", "contInList", true);
    Parser parser(schema);
    std::string input;
    std::ostringstream errorStream;

    SECTION("valid input")
    {
        create_ expected;

        SECTION("create a")
        {
            input = "create a";
            expected.m_path.m_nodes.push_back(container_("a"));
        }

        SECTION("create b/b2")
        {
            input = "create b/b2";
            expected.m_path.m_nodes.push_back(container_("b"));
            expected.m_path.m_nodes.push_back(container_("b2"));
        }

        SECTION("create list[quote=lol]/contInList")
        {
            input = "create list[quote=lol]/contInList";
            auto keys = std::map<std::string, std::string>{
                {"quote", "lol"}};
            expected.m_path.m_nodes.push_back(listElement_("list", keys));
            expected.m_path.m_nodes.push_back(container_("contInList"));
        }

        command_ command = parser.parseCommand(input, errorStream);
        REQUIRE(command.type() == typeid(create_));
        create_ create = boost::get<create_>(command);
        REQUIRE(create == expected);
    }
    SECTION("invalid input")
    {
        SECTION("create c")
        {
            input = "create c";
        }

        SECTION("create a/a2")
        {
            input = "create a/a2";
        }

        REQUIRE_THROWS(parser.parseCommand(input, errorStream));
    }
}
