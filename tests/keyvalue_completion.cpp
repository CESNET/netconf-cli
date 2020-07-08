/*
 * Copyright (C) 2020 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/

#include <experimental/iterator>
#include <iostream>
#include "trompeloeil_doctest.hpp"
#include "ast_commands.hpp"
#include "datastoreaccess_mock.hpp"
#include "parser.hpp"
#include "pretty_printers.hpp"
#include "static_schema.hpp"


TEST_CASE("keyvalue_completion")
{
    auto schema = std::make_shared<StaticSchema>();
    schema->addModule("example");
    schema->addContainer("/", "example:a");
    schema->addContainer("/", "example:b");
    schema->addList("/", "example:list", {"number"});
    schema->addLeaf("/example:list", "example:number", yang::Int32{});
    schema->addList("/", "example:twoKeyList", {"number", "name"});
    schema->addLeaf("/example:twoKeyList", "example:number", yang::Int32{});
    schema->addLeaf("/example:twoKeyList", "example:name", yang::String{});
    auto mockDatastore = std::make_shared<MockDatastoreAccess>();

    // DataQuery gets the schema from DatastoreAccess once
    auto expectation = NAMED_REQUIRE_CALL(*mockDatastore, schema())
        .RETURN(schema);
    auto dataQuery = std::make_shared<DataQuery>(*mockDatastore);
    expectation.reset();
    Parser parser(schema, WritableOps::No, dataQuery);
    std::string input;
    std::ostringstream errorStream;

    std::set<std::string> expected;
    std::vector<std::unique_ptr<trompeloeil::expectation>> queryExpectations;
    std::vector<ListInstance> queryReturn;

    SECTION("get example:list[number=")
    {
        input = "get example:list[number=";
        queryReturn = {
            {{"number", 1}},
            {{"number", 7}},
            {{"number", 9}},
            {{"number", 42}}
        };
        expected = {
            "1",
            "7",
            "9",
            "42"
        };
        queryExpectations.emplace_back(NAMED_REQUIRE_CALL(*mockDatastore, listInstances("/example:list")).RETURN(queryReturn));
    }

    SECTION("get example:twoKeyList[number=")
    {
        queryReturn = {
            {{"number", 1}, {"name", std::string{"Petr"}}},
            {{"number", 7}, {"name", std::string{"Petr"}}},
            {{"number", 10}, {"name", std::string{"Petr"}}},
            {{"number", 10}, {"name", std::string{"Honza"}}},
            {{"number", 100}, {"name", std::string{"Honza"}}},
        };
        input = "get example:twoKeyList[";
        SECTION("no keys set")
        {
            SECTION("number")
            {
                input += "number=";
                expected = { "1", "7", "10", "100" };
            }
            SECTION("name")
            {
                input += "name=";
                expected = { "'Petr'", "'Honza'"};
            }
            queryExpectations.emplace_back(NAMED_REQUIRE_CALL(*mockDatastore, listInstances("/example:twoKeyList")).RETURN(queryReturn));
        }

        SECTION("name is set")
        {
            input += "name=";
            SECTION("Petr")
            {
                input += "'Petr'";
                expected = { "1", "7", "10"};
            }
            SECTION("Honza")
            {
                input += "'Honza'";
                expected = { "10", "100" };
            }
            input += "][number=";
            queryExpectations.emplace_back(NAMED_REQUIRE_CALL(*mockDatastore, listInstances("/example:twoKeyList"))
                    .TIMES(2)
                    .RETURN(queryReturn));
        }

        SECTION("number is set")
        {
            input += "number=";
            SECTION("1")
            {
                input += "1";
                expected = { "'Petr'" };
            }
            SECTION("7")
            {
                input += "7";
                expected = { "'Petr'" };
            }
            SECTION("10")
            {
                input += "10";
                expected = { "'Honza'", "'Petr'" };
            }
            SECTION("100")
            {
                input += "100";
                expected = { "'Honza'" };
            }
            input += "][name=";
            queryExpectations.emplace_back(NAMED_REQUIRE_CALL(*mockDatastore, listInstances("/example:twoKeyList"))
                    .TIMES(2)
                    .RETURN(queryReturn));
        }

        SECTION("both keys are set")
        {
            SECTION("get example:twoKeyList[number=123][name='Petr'")
            {
                input = "get example:twoKeyList[number=123][name='Petr'";
                expected = {"]/"};
            }
            SECTION("get example:twoKeyList[number=123][name='Petr']")
            {
                input = "get example:twoKeyList[number=123][name='Petr']";
                expected = {"]/"};
            }
            SECTION("get example:twoKeyList[number=123][name='Petr'][")
            {
                input = "get example:twoKeyList[number=123][name='Petr'][";
            }
            // I use ALLOW_CALL here, all this stuff calls it different number of times
            queryExpectations.emplace_back(NAMED_ALLOW_CALL(*mockDatastore, listInstances("/example:twoKeyList"))
                    .RETURN(queryReturn));
        }

    }

    REQUIRE(parser.completeCommand(input, errorStream).m_completions == expected);
}
