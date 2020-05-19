/*
 * Copyright (C) 2020 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
*/

#include "trompeloeil_doctest.hpp"
#include "ast_commands.hpp"
#include "parser.hpp"
#include "static_schema.hpp"

TEST_CASE("leaflist")
{
    using namespace std::string_literals;
    auto schema = std::make_shared<StaticSchema>();
    schema->addModule("mod");
    schema->addLeafList("/", "mod:addresses", yang::String{});

    Parser parser(schema);
    std::string input;
    std::ostringstream errorStream;

    SECTION("creating/deleting")
    {
        dataPath_ expected;

        SECTION("create mod:addresses['0.0.0.0']")
        {
            input = "mod:addresses['0.0.0.0']";
            expected.m_nodes.push_back(dataNode_{module_{"mod"}, leafListElement_{"addresses", "0.0.0.0"s}});
        }

        create_ expectedCreate;
        delete_ expectedDelete;
        expectedCreate.m_path = expected;
        expectedDelete.m_path = expected;
        command_ commandCreate = parser.parseCommand("create " + input, errorStream);
        command_ commandDelete = parser.parseCommand("delete " + input, errorStream);
        REQUIRE(commandCreate.type() == typeid(create_));
        REQUIRE(commandDelete.type() == typeid(delete_));
        REQUIRE(boost::get<create_>(commandCreate) == expectedCreate);
        REQUIRE(boost::get<delete_>(commandDelete) == expectedDelete);
    }

    SECTION("retrieving all intances")
    {
        dataPath_ expected;
        input = "get mod:addresses";
        expected.m_nodes.push_back(dataNode_{module_{"mod"}, leafList_{"addresses"}});

        get_ expectedGet;
        expectedGet.m_path = expected;
        command_ commandGet = parser.parseCommand(input, errorStream);
        REQUIRE(commandGet.type() == typeid(get_));
        REQUIRE(boost::get<get_>(commandGet) == expectedGet);
    }


}
