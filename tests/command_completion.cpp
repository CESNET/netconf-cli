/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/

#include "trompeloeil_doctest.h"
#include "datastoreaccess_mock.hpp"
#include "parser.hpp"
#include "pretty_printers.hpp"
#include "static_schema.hpp"

TEST_CASE("command completion")
{
    auto schema = std::make_shared<StaticSchema>();
    auto mockDatastore = std::make_shared<MockDatastoreAccess>();

    // DataQuery gets the schema from DatastoreAccess once
    auto expectation = NAMED_REQUIRE_CALL(*mockDatastore, schema())
        .RETURN(schema);
    auto dataQuery = std::make_shared<DataQuery>(*mockDatastore);
    expectation.reset();
    Parser parser(schema, dataQuery);
    std::string input;
    std::ostringstream errorStream;
    std::set<std::string> expectedCompletions;
    int expectedContextLength;
    SECTION("")
    {
        input = "";
        expectedCompletions = {"cd", "create", "delete", "set", "commit", "get", "ls", "discard", "help"};
        expectedContextLength = 0;
    }

    SECTION(" ")
    {
        input = " ";
        expectedCompletions = {"cd", "create", "delete", "set", "commit", "get", "ls", "discard", "help"};
        expectedContextLength = 0;
    }

    SECTION("c")
    {
        input = "c";
        expectedCompletions = {"cd", "commit", "create"};
        expectedContextLength = 1;
    }

    SECTION("d")
    {
        input = "d";
        expectedCompletions = {"delete", "discard"};
        expectedContextLength = 1;
    }

    SECTION("x")
    {
        input = "x";
        expectedCompletions = {};
        expectedContextLength = 1;
    }

    SECTION("cd")
    {
        input = "cd";
        // TODO: depending on how Readline works, this will have to be changed to include a space
        expectedCompletions = {"cd"};
        expectedContextLength = 2;
    }

    SECTION("create")
    {
        input = "create";
        expectedCompletions = {"create"};
        expectedContextLength = 6;
    }

    REQUIRE(parser.completeCommand(input, errorStream) == (Completions{expectedCompletions, expectedContextLength}));
}
