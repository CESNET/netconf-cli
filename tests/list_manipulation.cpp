/*
 * Copyright (C) 2019 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
*/

#include "trompeloeil_doctest.h"
#include "parser.hpp"
#include "static_schema.hpp"

TEST_CASE("list manipulation")
{
    auto schema = std::make_shared<StaticSchema>();
    schema->addModule("mod");
    schema->addList("", "mod:list", {"number"});
    schema->addLeaf("mod:list", "mod:leafInList", yang::LeafDataTypes::String);
    Parser parser(schema);
    std::string input;
    std::ostringstream errorStream;

    SECTION("creating/deleting list instances")
    {
        dataPath_ expectedPath;
        SECTION("mod:list[number=3]")
        {
            input = "mod:list[number=3]";
            auto keys = std::map<std::string, std::string>{
                {"number", "3"}};
            expectedPath.m_nodes.push_back(dataNode_{module_{"mod"}, listElement_("list", keys)});
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
}
