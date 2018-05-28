
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

TEST_CASE("presence containers")
{
    Schema schema;
    schema.addContainer("", "a", yang::ContainerTraits::Presence);
    schema.addContainer("", "b");
    schema.addContainer("a", "a2");
    schema.addContainer("a/a2", "a3", yang::ContainerTraits::Presence);
    schema.addContainer("b", "b2", yang::ContainerTraits::Presence);
    schema.addList("", "list", {"quote"});
    schema.addContainer("list", "contInList", yang::ContainerTraits::Presence);
    Parser parser(schema);
    std::string input;
    std::ostringstream errorStream;

    SECTION("valid input")
    {
        path_ expectedPath;

        SECTION("a")
        {
            input = " a";
            expectedPath.m_nodes = { container_("a") };
        }

        SECTION("b/b2")
        {
            input = " b/b2";
            expectedPath.m_nodes = { container_("b"), container_("b2") };
        }

        SECTION("a/a2/a3")
        {
            input = " a/a2/a3";
            expectedPath.m_nodes = { container_("a"), container_("a2"), container_("a3") };
        }

        SECTION("list[quote=lol]/contInList")
        {
            input = " list[quote=lol]/contInList";
            auto keys = std::map<std::string, std::string>{
                {"quote", "lol"}};
            expectedPath.m_nodes = { listElement_("list", keys), container_("contInList") };
        }

        create_ expectedCreate;
        expectedCreate.m_path = expectedPath;
        delete_ expectedDelete;
        expectedDelete.m_path = expectedPath;

        command_ commandCreate = parser.parseCommand("create " + input, errorStream);
        REQUIRE(commandCreate.type() == typeid(create_));
        create_ create = boost::get<create_>(commandCreate);
        REQUIRE(create == expectedCreate);

        command_ commandDelete = parser.parseCommand("delete " + input, errorStream);
        REQUIRE(commandDelete.type() == typeid(delete_));
        delete_ delet = boost::get<delete_>(commandDelete);
        REQUIRE(delet == expectedDelete);
    }
    SECTION("invalid input")
    {
        SECTION("c")
        {
            input = " c";
        }

        SECTION("a/a2")
        {
            input = " a/a2";
        }

        SECTION("list[quote=lol]")
        {
            input = " list[quote=lol]";
        }

        REQUIRE_THROWS(parser.parseCommand("create " + input, errorStream));
        REQUIRE_THROWS(parser.parseCommand("delete " + input, errorStream));
    }
}
