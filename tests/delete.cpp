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

TEST_CASE("delete")
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
        delete_ expected;

        SECTION("delete a")
        {
            input = "delete a";
            expected.m_path.m_nodes.push_back(container_("a"));
        }

        SECTION("delete b/b2")
        {
            input = "delete b/b2";
            expected.m_path.m_nodes.push_back(container_("b"));
            expected.m_path.m_nodes.push_back(container_("b2"));
        }

        SECTION("delete list[quote=lol]/contInList")
        {
            input = "delete list[quote=lol]/contInList";
            auto keys = std::map<std::string, std::string>{
                {"quote", "lol"}};
            expected.m_path.m_nodes.push_back(listElement_("list", keys));
            expected.m_path.m_nodes.push_back(container_("contInList"));
        }

        command_ command = parser.parseCommand(input, errorStream);
        REQUIRE(command.type() == typeid(delete_));
        delete_ delete_cmd = boost::get<delete_>(command);
        REQUIRE(delete_cmd == expected);
    }
    SECTION("invalid input")
    {
        SECTION("delete c")
        {
            input = "delete c";
        }

        SECTION("delete a/a2")
        {
            input = "delete a/a2";
        }

        REQUIRE_THROWS(parser.parseCommand(input, errorStream));
    }
}
