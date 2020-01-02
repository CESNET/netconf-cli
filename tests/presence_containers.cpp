
/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/

#include "trompeloeil_doctest.h"
#include "ast_commands.hpp"
#include "datastoreaccess_mock.hpp"
#include "parser.hpp"
#include "static_schema.hpp"

TEST_CASE("presence containers")
{
    auto schema = std::make_shared<StaticSchema>();
    schema->addModule("mod");
    schema->addContainer("", "mod:a", yang::ContainerTraits::Presence);
    schema->addContainer("", "mod:b");
    schema->addContainer("mod:a", "mod:a2");
    schema->addContainer("mod:a/mod:a2", "mod:a3", yang::ContainerTraits::Presence);
    schema->addContainer("mod:b", "mod:b2", yang::ContainerTraits::Presence);
    schema->addList("", "mod:list", {"quote"});
    schema->addContainer("mod:list", "mod:contInList", yang::ContainerTraits::Presence);
    std::shared_ptr<DatastoreAccess> mockDatastore = std::make_shared<MockDatastoreAccess>();
    auto dataQuery = std::make_shared<DataQuery>(*mockDatastore);
    Parser parser(schema, dataQuery);
    std::string input;
    std::ostringstream errorStream;

    SECTION("valid input")
    {
        dataPath_ expectedPath;

        SECTION("mod:a")
        {
            input = "mod:a";
            expectedPath.m_nodes = {{module_{"mod"}, {container_("a")}}};
        }

        SECTION("mod:b/b2")
        {
            input = "mod:b/b2";
            expectedPath.m_nodes = {{{module_{"mod"}}, container_("b")}, {container_("b2")}};
        }

        SECTION("mod:a/a2/a3")
        {
            input = "mod:a/a2/a3";
            expectedPath.m_nodes = {{{module_{"mod"}}, container_("a")}, {container_("a2")}, {container_("a3")}};
        }

        SECTION("mod:list[quote='lol']/contInList")
        {
            input = "mod:list[quote='lol']/contInList";
            auto keys = std::map<std::string, std::string>{
                {"quote", "lol"}};
            expectedPath.m_nodes = {{{module_{"mod"}}, listElement_("list", keys)}, {container_("contInList")}};
        }

        SECTION("mod:list[quote='double\"quote']/contInList")
        {
            input = "mod:list[quote='double\"quote']/contInList";
            auto keys = std::map<std::string, std::string>{
                {"quote", "double\"quote"}};
            expectedPath.m_nodes = {{{module_{"mod"}}, listElement_("list", keys)}, {container_("contInList")}};
        }

        SECTION("mod:list[quote=\"single'quote\"]/contInList")
        {
            input = "mod:list[quote=\"single'quote\"]/contInList";
            auto keys = std::map<std::string, std::string>{
                {"quote", "single'quote"}};
            expectedPath.m_nodes = {{{module_{"mod"}}, listElement_("list", keys)}, {container_("contInList")}};
        }

        create_ expectedCreate;
        expectedCreate.m_path = expectedPath;
        command_ commandCreate = parser.parseCommand("create " + input, errorStream);
        REQUIRE(commandCreate.type() == typeid(create_));
        create_ create = boost::get<create_>(commandCreate);
        REQUIRE(create == expectedCreate);

        REQUIRE(pathToDataString(create.m_path) == input);

        delete_ expectedDelete;
        expectedDelete.m_path = expectedPath;
        command_ commandDelete = parser.parseCommand("delete " + input, errorStream);
        REQUIRE(commandDelete.type() == typeid(delete_));
        delete_ delet = boost::get<delete_>(commandDelete);
        REQUIRE(delet == expectedDelete);
    }
    SECTION("invalid input")
    {
        SECTION("c")
        {
            input = "c";
        }

        SECTION("a/a2")
        {
            input = "a/a2";
        }

        SECTION("list[quote='lol']")
        {
            input = "list[quote='lol']";
        }

        REQUIRE_THROWS_AS(parser.parseCommand("create " + input, errorStream), InvalidCommandException);
        REQUIRE_THROWS_AS(parser.parseCommand("delete " + input, errorStream), InvalidCommandException);
    }
}
