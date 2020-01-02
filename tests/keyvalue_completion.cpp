/*
 * Copyright (C) 2020 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/

#include <experimental/iterator>
#include <iostream>
#include "trompeloeil_doctest.h"
#include "ast_commands.hpp"
#include "datastoreaccess_mock.hpp"
#include "parser.hpp"
#include "static_schema.hpp"

namespace std {
std::ostream& operator<<(std::ostream& s, const std::set<std::string> set)
{
    s << std::endl << "{";
    std::copy(set.begin(), set.end(), std::experimental::make_ostream_joiner(s, ", "));
    s << "}" << std::endl;
    return s;
}
}

TEST_CASE("keyvalue_completion")
{
    auto schema = std::make_shared<StaticSchema>();
    schema->addModule("example");
    schema->addContainer("/", "example:a");
    schema->addContainer("/", "example:b");
    schema->addList("/", "example:list", {"number"});
    schema->addLeaf("/example:list", "example:number", yang::LeafDataTypes::Int32);
    schema->addList("/", "example:twoKeyList", {"number", "name"});
    schema->addLeaf("/example:twoKeyList", "example:number", yang::LeafDataTypes::Int32);
    schema->addLeaf("/example:twoKeyList", "example:name", yang::LeafDataTypes::String);
    auto mockDatastore = std::make_shared<MockDatastoreAccess>();

    // DataQuery gets the schema from DatastoreAccess once
    auto expectation = NAMED_REQUIRE_CALL(*mockDatastore, schema())
        .RETURN(schema);
    auto dataQuery = std::make_shared<DataQuery>(*mockDatastore);
    expectation.reset();
    Parser parser(schema, dataQuery);
    std::string input;
    std::ostringstream errorStream;

    std::set<std::string> expected;
    std::vector<std::shared_ptr<trompeloeil::expectation>> queryExpectations;
    std::map<std::string, leaf_data_> queryReturn;

    SECTION("get example:list[number=")
    {
        input = "get example:list[number=";
        queryReturn = {
            {"/example:list[number=1]", special_{SpecialValue::List}},
            {"/example:list[number=7]", special_{SpecialValue::List}},
            {"/example:list[number=9]", special_{SpecialValue::List}},
            {"/example:list[number=42]", special_{SpecialValue::List}},
            {"/example:list[number=1]/number", 1},
            {"/example:list[number=7]/number", 7},
            {"/example:list[number=9]/number", 9},
            {"/example:list[number=42]/number", 42}
        };
        expected = {
            "1",
            "7",
            "9",
            "42"
        };
        queryExpectations.push_back(NAMED_REQUIRE_CALL(*mockDatastore, getItems("/example:list"))
            .RETURN(queryReturn));
    }

    SECTION("get example:twoKeyList[number=")
    {
        queryReturn = {
            {"/example:twoKeyList[name='Petr'][number=1]", special_{SpecialValue::List}},
            {"/example:twoKeyList[name='Petr'][number=7]", special_{SpecialValue::List}},
            {"/example:twoKeyList[name='Petr'][number=10]", special_{SpecialValue::List}},
            {"/example:twoKeyList[name='Honza'][number=10]", special_{SpecialValue::List}},
            {"/example:twoKeyList[name='Honza'][number=100]", special_{SpecialValue::List}},
            {"/example:twoKeyList[name='Petr'][number=1]/number", 1},
            {"/example:twoKeyList[name='Petr'][number=7]/number", 7},
            {"/example:twoKeyList[name='Petr'][number=10]/number", 10},
            {"/example:twoKeyList[name='Honza'][number=10]/number", 10},
            {"/example:twoKeyList[name='Honza'][number=100]/number", 100},
            {"/example:twoKeyList[name='Petr'][number=1]/name", std::string{"Petr"}},
            {"/example:twoKeyList[name='Petr'][number=7]/name", std::string{"Petr"}},
            {"/example:twoKeyList[name='Petr'][number=10]/name", std::string{"Petr"}},
            {"/example:twoKeyList[name='Honza'][number=10]/name", std::string{"Honza"}},
            {"/example:twoKeyList[name='Honza'][number=100]/name", std::string{"Honza"}},
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
            queryExpectations.push_back(NAMED_REQUIRE_CALL(*mockDatastore, getItems("/example:twoKeyList"))
                    .RETURN(queryReturn));
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
            queryExpectations.push_back(NAMED_REQUIRE_CALL(*mockDatastore, getItems("/example:twoKeyList"))
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
            queryExpectations.push_back(NAMED_REQUIRE_CALL(*mockDatastore, getItems("/example:twoKeyList"))
                    .TIMES(2)
                    .RETURN(queryReturn));
        }

    }

    REQUIRE(parser.completeCommand(input, errorStream) == expected);
    for (auto& it : queryExpectations) {
        it.reset();
    }
}
