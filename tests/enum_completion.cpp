
/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
 */

#include "trompeloeil_doctest.hpp"
#include "datastoreaccess_mock.hpp"
#include "parser.hpp"
#include "pretty_printers.hpp"
#include "static_schema.hpp"

TEST_CASE("enum completion")
{
    auto schema = std::make_shared<StaticSchema>();
    schema->addModule("mod");
    schema->addContainer("/", "mod:contA");
    schema->addLeaf("/", "mod:leafEnum", yang::Enum{"lala", "lol", "data", "coze"});
    schema->addLeaf("/mod:contA", "mod:leafInCont", yang::Enum{"abc", "def"});
    schema->addList("/", "mod:list", {"number"});
    schema->addLeaf("/mod:list", "mod:number", yang::Int32{});
    schema->addLeaf("/mod:list", "mod:leafInList", yang::Enum{"ano", "anoda", "ne", "katoda"});
    auto mockDatastore = std::make_shared<MockDatastoreAccess>();
    // The parser will use DataQuery for key value completion, but I'm not testing that here, so I don't return anything.
    ALLOW_CALL(*mockDatastore, listInstances("/mod:list"))
        .RETURN(std::vector<ListInstance>{});

    // DataQuery gets the schema from DatastoreAccess once
    auto expectation = NAMED_REQUIRE_CALL(*mockDatastore, schema())
        .RETURN(schema);
    auto dataQuery = std::make_shared<DataQuery>(*mockDatastore);
    Parser parser(schema, dataQuery);
    std::string input;
    std::ostringstream errorStream;

    std::set<std::string> expectedCompletions;
    int expectedContextLength;

    SECTION("set mod:leafEnum ")
    {
        input = "set mod:leafEnum ";
        expectedCompletions = {"lala", "lol", "data", "coze"};
        expectedContextLength = 0;
    }

    SECTION("set mod:leafEnum c")
    {
        input = "set mod:leafEnum c";
        expectedCompletions = {"coze"};
        expectedContextLength = 1;
    }

    SECTION("set mod:leafEnum l")
    {
        input = "set mod:leafEnum l";
        expectedCompletions = {"lala", "lol"};
        expectedContextLength = 1;
    }


    SECTION("set mod:contA/leafInCont ")
    {
        input = "set mod:contA/leafInCont ";
        expectedCompletions = {"abc", "def"};
        expectedContextLength = 0;
    }

    SECTION("set mod:list[number=42]/leafInList ")
    {
        input = "set mod:list[number=42]/leafInList ";
        expectedCompletions = {"ano", "anoda", "ne", "katoda"};
        expectedContextLength = 0;
    }

    REQUIRE(parser.completeCommand(input, errorStream) == (Completions{expectedCompletions, expectedContextLength}));
}
