/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/

#include <experimental/iterator>
#include "trompeloeil_doctest.h"
#include "datastoreaccess_mock.hpp"
#include "parser.hpp"
#include "static_schema.hpp"

namespace std {
std::ostream& operator<<(std::ostream& s, const Completions& completion)
{
    s << std::endl << "Completions {" << std::endl << "    m_completions: ";
    std::transform(completion.m_completions.begin(), completion.m_completions.end(),
            std::experimental::make_ostream_joiner(s, ", "),
            [] (auto it) { return '"' + it + '"'; });
    s << std::endl << "    m_contextLength: " << completion.m_contextLength << std::endl;
    s << "}" << std::endl;
    return s;
}
}

TEST_CASE("path_completion")
{
    auto schema = std::make_shared<StaticSchema>();
    schema->addModule("example");
    schema->addModule("second");
    schema->addContainer("/", "example:ano");
    schema->addContainer("/", "example:anoda");
    schema->addList("/example:ano", "example:listInCont", {"number"});
    schema->addContainer("/", "second:amelie");
    schema->addContainer("/", "example:bota");
    schema->addContainer("/example:ano", "example:a2");
    schema->addContainer("/example:bota", "example:b2");
    schema->addContainer("/example:ano/example:a2", "example:a3");
    schema->addContainer("/example:bota/example:b2", "example:b3");
    schema->addList("/", "example:list", {"number"});
    schema->addLeaf("/example:list", "example:number", yang::LeafDataTypes::Int32);
    schema->addContainer("/example:list", "example:contInList");
    schema->addList("/", "example:ovoce", {"name"});
    schema->addLeaf("/example:ovoce", "example:name", yang::LeafDataTypes::String);
    schema->addList("/", "example:ovocezelenina", {"name"});
    schema->addLeaf("/example:ovocezelenina", "example:name", yang::LeafDataTypes::String);
    schema->addList("/", "example:twoKeyList", {"number", "name"});
    schema->addLeaf("/example:twoKeyList", "example:name", yang::LeafDataTypes::String);
    schema->addLeaf("/example:twoKeyList", "example:number", yang::LeafDataTypes::Int32);
    schema->addLeaf("/", "example:leafInt", yang::LeafDataTypes::Int32);
    auto mockDatastore = std::make_shared<MockDatastoreAccess>();

    // The parser will use DataQuery for key value completion, but I'm not testing that here, so I don't return anything.
    ALLOW_CALL(*mockDatastore, listInstances("/example:list"))
        .RETURN(std::vector<ListInstance>{});
    ALLOW_CALL(*mockDatastore, listInstances("/example:twoKeyList"))
        .RETURN(std::vector<ListInstance>{});

    // DataQuery gets the schema from DatastoreAccess once
    auto expectation = NAMED_REQUIRE_CALL(*mockDatastore, schema())
        .RETURN(schema);
    auto dataQuery = std::make_shared<DataQuery>(*mockDatastore);
    expectation.reset();
    Parser parser(schema, dataQuery);
    std::string input;
    std::ostringstream errorStream;

    std::set<std::string> expectedCompletions;
    // GCC complains here with -Wmaybe-uninitialized if I don't assign
    // something here. I suspect it's because of nested SECTIONs. -1 is an
    // invalid value (as in, I'll never expect expectedContextLength to be -1),
    // so let's go with that.
    int expectedContextLength = -1;

    SECTION("node name completion")
    {
        SECTION("ls ")
        {
            input = "ls ";
            expectedCompletions = {"example:ano/", "example:anoda/", "example:bota/", "example:leafInt ", "example:list[", "example:ovoce[", "example:ovocezelenina[", "example:twoKeyList[", "second:amelie/"};
            expectedContextLength = 0;
        }

        SECTION("ls e")
        {
            input = "ls e";
            expectedCompletions = {"example:ano/", "example:anoda/", "example:bota/", "example:leafInt ", "example:list[", "example:ovoce[", "example:ovocezelenina[", "example:twoKeyList["};
            expectedContextLength = 1;
        }

        SECTION("ls example:ano")
        {
            input = "ls example:ano";
            expectedCompletions = {"example:ano/", "example:anoda/"};
            expectedContextLength = 11;
        }

        SECTION("ls example:ano/example:a")
        {
            input = "ls example:ano/example:a";
            expectedCompletions = {"example:a2/"};
            expectedContextLength = 9;
        }

        SECTION("ls x")
        {
            input = "ls x";
            expectedCompletions = {};
            expectedContextLength = 1;
        }

        SECTION("ls /")
        {
            input = "ls /";
            expectedCompletions = {"example:ano/", "example:anoda/", "example:bota/", "example:leafInt ", "example:list[", "example:ovoce[", "example:ovocezelenina[", "example:twoKeyList[", "second:amelie/"};
            expectedContextLength = 0;
        }

        SECTION("ls /e")
        {
            input = "ls /e";
            expectedCompletions = {"example:ano/", "example:anoda/", "example:bota/", "example:leafInt ", "example:list[", "example:ovoce[", "example:ovocezelenina[", "example:twoKeyList["};
            expectedContextLength = 1;
        }

        SECTION("ls example:bota")
        {
            input = "ls example:bota";
            expectedCompletions = {"example:bota/"};
            expectedContextLength = 12;
        }

        SECTION("ls /example:bota")
        {
            input = "ls /example:bota";
            expectedCompletions = {"example:bota/"};
            expectedContextLength = 12;
        }

        SECTION("ls /s")
        {
            input = "ls /s";
            expectedCompletions = {"second:amelie/"};
            expectedContextLength = 1;
        }

        SECTION("ls /example:list[number=3]/")
        {
            input = "ls /example:list[number=3]/";
            expectedCompletions = {"example:contInList/", "example:number "};
            expectedContextLength = 0;
        }

        SECTION("ls /example:list[number=3]/c")
        {
            input = "ls /example:list[number=3]/e";
            expectedCompletions = {"example:contInList/", "example:number "};
            expectedContextLength = 1;
        }

        SECTION("ls /example:list[number=3]/a")
        {
            input = "ls /example:list[number=3]/a";
            expectedCompletions = {};
            expectedContextLength = 1;
        }
    }

    SECTION("list keys completion")
    {
        SECTION("cd example:lis")
        {
            input = "cd example:lis";
            expectedCompletions = {"example:list["};
            expectedContextLength = 11;
        }

        SECTION("set example:list")
        {
            input = "set example:list";
            expectedCompletions = {"example:list["};
            expectedContextLength = 12;
        }

        SECTION("cd example:list")
        {
            input = "cd example:list";
            expectedCompletions = {"example:list["};
            expectedContextLength = 12;
        }

        SECTION("cd example:list[")
        {
            input = "cd example:list[";
            expectedCompletions = {"number="};
            expectedContextLength = 0;
        }

        SECTION("cd example:list[numb")
        {
            input = "cd example:list[numb";
            expectedCompletions = {"number="};
            expectedContextLength = 4;
        }

        SECTION("cd example:list[number")
        {
            input = "cd example:list[number";
            expectedCompletions = {"number="};
            expectedContextLength = 6;
        }

        SECTION("cd example:list[number=")
        {
            input = "cd example:list[number=";
            expectedCompletions = {};
            expectedContextLength = 0;
        }

        SECTION("cd example:list[number=12")
        {
            input = "cd example:list[number=12";
            expectedCompletions = {"]/"};
            expectedContextLength = 0;
        }

        SECTION("cd example:list[number=12]")
        {
            input = "cd example:list[number=12]";
            expectedCompletions = {"]/"};
            expectedContextLength = 1;
        }

        SECTION("cd example:twoKeyList[")
        {
            input = "cd example:twoKeyList[";
            expectedCompletions = {"name=", "number="};
            expectedContextLength = 0;
        }

        SECTION("cd example:twoKeyList[name=\"AHOJ\"")
        {
            input = "cd example:twoKeyList[name=\"AHOJ\"";
            expectedCompletions = {"]["};
            expectedContextLength = 0;
        }

        SECTION("cd example:twoKeyList[name=\"AHOJ\"]")
        {
            input = "cd example:twoKeyList[name=\"AHOJ\"]";
            expectedCompletions = {"]["};
            expectedContextLength = 1;
        }

        SECTION("cd example:twoKeyList[name=\"AHOJ\"][")
        {
            input = "cd example:twoKeyList[name=\"AHOJ\"][";
            expectedCompletions = {"number="};
            expectedContextLength = 0;
        }

        SECTION("cd example:twoKeyList[number=42][")
        {
            input = "cd example:twoKeyList[number=42][";
            expectedCompletions = {"name="};
            expectedContextLength = 0;
        }

        SECTION("cd example:twoKeyList[name=\"AHOJ\"][number=123")
        {
            input = "cd example:twoKeyList[name=\"AHOJ\"][number=123";
            expectedCompletions = {"]/"};
            expectedContextLength = 0;
        }

        SECTION("cd example:twoKeyList[name=\"AHOJ\"][number=123]")
        {
            input = "cd example:twoKeyList[name=\"AHOJ\"][number=123]";
            expectedCompletions = {"]/"};
            expectedContextLength = 1;
        }

        SECTION("cd example:ovoce")
        {
            input = "cd example:ovoce";
            expectedCompletions = {"example:ovoce[", "example:ovocezelenina["};
            expectedContextLength = 13;
        }
    }

    SECTION("clear completions when no longer inputting path")
    {
        input = "set example:leafInt ";
        expectedCompletions = {};
        expectedContextLength = 0;
    }

    REQUIRE(parser.completeCommand(input, errorStream) == (Completions{expectedCompletions, expectedContextLength}));
}
