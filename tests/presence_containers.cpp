
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
    std::string inputCreate;
    std::string inputDelete;
    std::ostringstream errorStream;

    SECTION("valid inputCreate")
    {
        create_ expectedCreate;
        delete_ expectedDelete;

        SECTION("a")
        {
            inputCreate = "create a";
            inputDelete = "delete a";
            expectedCreate.m_path.m_nodes.push_back(container_("a"));
            expectedDelete.m_path.m_nodes.push_back(container_("a"));
        }

        SECTION("b/b2")
        {
            inputCreate = "create b/b2";
            inputDelete = "delete b/b2";
            expectedCreate.m_path.m_nodes.push_back(container_("b"));
            expectedDelete.m_path.m_nodes.push_back(container_("b"));
            expectedCreate.m_path.m_nodes.push_back(container_("b2"));
            expectedDelete.m_path.m_nodes.push_back(container_("b2"));
        }

        SECTION("a/a2/a3")
        {
            inputCreate = "create a/a2/a3";
            inputDelete = "delete a/a2/a3";
            expectedCreate.m_path.m_nodes.push_back(container_("a"));
            expectedDelete.m_path.m_nodes.push_back(container_("a"));
            expectedCreate.m_path.m_nodes.push_back(container_("a2"));
            expectedDelete.m_path.m_nodes.push_back(container_("a2"));
            expectedCreate.m_path.m_nodes.push_back(container_("a3"));
            expectedDelete.m_path.m_nodes.push_back(container_("a3"));
        }

        SECTION("list[quote=lol]/contInList")
        {
            inputCreate = "create list[quote=lol]/contInList";
            inputDelete = "delete list[quote=lol]/contInList";
            auto keys = std::map<std::string, std::string>{
                {"quote", "lol"}};
            expectedCreate.m_path.m_nodes.push_back(listElement_("list", keys));
            expectedDelete.m_path.m_nodes.push_back(listElement_("list", keys));
            expectedCreate.m_path.m_nodes.push_back(container_("contInList"));
            expectedDelete.m_path.m_nodes.push_back(container_("contInList"));
        }

        command_ commandCreate = parser.parseCommand(inputCreate, errorStream);
        REQUIRE(commandCreate.type() == typeid(create_));
        create_ create = boost::get<create_>(commandCreate);
        REQUIRE(create == expectedCreate);

        command_ commandDelete = parser.parseCommand(inputDelete, errorStream);
        REQUIRE(commandDelete.type() == typeid(delete_));
        delete_ delet = boost::get<delete_>(commandDelete);
        REQUIRE(delet == expectedDelete);
    }
    SECTION("invalid input")
    {
        SECTION("c")
        {
            inputCreate = "create c";
            inputDelete = "delete c";
        }

        SECTION("a/a2")
        {
            inputCreate = "create a/a2";
            inputDelete = "delete a/a2";
        }

        REQUIRE_THROWS(parser.parseCommand(inputCreate, errorStream));
        REQUIRE_THROWS(parser.parseCommand(inputDelete, errorStream));
    }
}
