/*
 * Copyright (C) 2019 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
*/

#include "trompeloeil_doctest.hpp"
#include "parser.hpp"
#include "pretty_printers.hpp"
#include "static_schema.hpp"

TEST_CASE("list manipulation")
{
    using namespace std::string_literals;
    auto schema = std::make_shared<StaticSchema>();
    schema->addModule("mod");
    schema->addModule("other");
    schema->addList("/", "mod:list", {"number"});
    schema->addLeaf("/mod:list", "mod:number", yang::Int32{});
    schema->addLeaf("/mod:list", "mod:leafInList", yang::String{});
    schema->addLeafList("/", "mod:addresses", yang::String{});
    schema->addIdentity(std::nullopt, identityRef_{"other", "deptypes"});
    schema->addIdentity(identityRef_{"other", "deptypes"}, identityRef_{"other", "engineering"});
    schema->addList("/", "mod:company", {"department"});
    schema->addLeaf("/mod:company", "mod:department", schema->validIdentities("other", "deptypes"));
    schema->addList("/mod:company", "mod:inventory", {"id"});
    schema->addLeaf("/mod:company/mod:inventory", "mod:id", yang::Int32{});
    Parser parser(schema);
    std::string input;
    std::ostringstream errorStream;

    SECTION("creating/deleting list instances")
    {
        dataPath_ expectedPath;
        SECTION("mod:list[number=3]")
        {
            input = "mod:list[number=3]";
            auto keys = std::map<std::string, leaf_data_>{
                {"number", int32_t{3}}};
            expectedPath.m_nodes.push_back(dataNode_{module_{"mod"}, listElement_("list", keys)});
        }

        SECTION("mod:company[department=other:engineering]/inventory[id=1337]")
        {
            input = "mod:company[department=other:engineering]/inventory[id=1337]";
            auto keys = std::map<std::string, leaf_data_>{
                {"department", identityRef_{"other", "engineering"}}};
            expectedPath.m_nodes.push_back(dataNode_{module_{"mod"}, listElement_("company", keys)});
            keys = std::map<std::string, leaf_data_>{
                {"id", int32_t{1337}}};
            expectedPath.m_nodes.push_back(dataNode_{listElement_("inventory", keys)});
        }

        SECTION("create mod:addresses['0.0.0.0']")
        {
            input = "mod:addresses['0.0.0.0']";
            expectedPath.m_nodes.push_back(dataNode_{module_{"mod"}, leafListElement_{"addresses", "0.0.0.0"s}});
        }


        command_ parsedCreate = parser.parseCommand("create " + input, errorStream);
        command_ parsedDelete = parser.parseCommand("delete " + input, errorStream);
        create_ expectedCreate;
        expectedCreate.m_path = expectedPath;
        delete_ expectedDelete;
        expectedDelete.m_path = expectedPath;
        REQUIRE(parsedCreate.type() == typeid(create_));
        REQUIRE(parsedDelete.type() == typeid(delete_));
        REQUIRE(boost::get<create_>(parsedCreate) == expectedCreate);
        REQUIRE(boost::get<delete_>(parsedDelete) == expectedDelete);
    }

    SECTION("retrieving all leaflist instances")
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

    SECTION("moving (leaf)list instances")
    {
        dataPath_ expectedPath;
        Move expectedMove;
        SECTION("begin")
        {
            input = "move mod:addresses['1.2.3.4'] begin";
            expectedPath.m_nodes.push_back(dataNode_{module_{"mod"}, leafListElement_{"addresses", "1.2.3.4"s}});
            expectedMove.m_mode = MoveMode::Begin;
        }

        SECTION("end")
        {
            input = "move mod:addresses['1.2.3.4'] end";
            expectedPath.m_nodes.push_back(dataNode_{module_{"mod"}, leafListElement_{"addresses", "1.2.3.4"s}});
            expectedMove.m_mode = MoveMode::End;
        }

        SECTION("after")
        {
            input = "move mod:addresses['1.2.3.4'] after '0.0.0.0'";
            expectedPath.m_nodes.push_back(dataNode_{module_{"mod"}, leafListElement_{"addresses", "1.2.3.4"s}});
            expectedMove.m_mode = MoveMode::After;
            expectedMove.m_destination = leaf_data_{"0.0.0.0"s};
        }

        SECTION("before")
        {
            input = "move mod:addresses['1.2.3.4'] before '0.0.0.0'";
            expectedPath.m_nodes.push_back(dataNode_{module_{"mod"}, leafListElement_{"addresses", "1.2.3.4"s}});
            expectedMove.m_mode = MoveMode::Before;
            expectedMove.m_destination = leaf_data_{"0.0.0.0"s};
        }

        SECTION("list instance with destination")
        {
            input = "move mod:list[number=12] before [number=15]";
            auto keys = std::map<std::string, leaf_data_>{
                {"number", int32_t{12}}};
            expectedPath.m_nodes.push_back(dataNode_{module_{"mod"}, listElement_("list", keys)});
            expectedMove.m_mode = MoveMode::Before;
            expectedMove.m_destination = ListInstance{{"number", int32_t{15}}};
        }

        SECTION("list instance without destination")
        {
            input = "move mod:list[number=3] begin";
            auto keys = std::map<std::string, leaf_data_>{
                {"number", int32_t{3}}};
            expectedPath.m_nodes.push_back(dataNode_{module_{"mod"}, listElement_("list", keys)});
            expectedMove.m_mode = MoveMode::Begin;
        }

        move_ expected;
        expected.m_path = expectedPath;
        expected.m_move = expectedMove;

        command_ commandMove = parser.parseCommand(input, errorStream);
        REQUIRE(commandMove.type() == typeid(move_));
        REQUIRE(boost::get<move_>(commandMove) == expected);
    }
}
