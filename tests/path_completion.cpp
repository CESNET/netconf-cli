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
    schema->addLeaf("/example:list", "example:number", yang::Int32{});
    schema->addContainer("/example:list", "example:contInList");
    schema->addList("/", "example:ovoce", {"name"});
    schema->addLeaf("/example:ovoce", "example:name", yang::String{});
    schema->addList("/", "example:ovocezelenina", {"name"});
    schema->addLeaf("/example:ovocezelenina", "example:name", yang::String{});
    schema->addList("/", "example:twoKeyList", {"number", "name"});
    schema->addLeaf("/example:twoKeyList", "example:name", yang::String{});
    schema->addLeaf("/example:twoKeyList", "example:number", yang::Int32{});
    schema->addLeaf("/", "example:leafInt", yang::Int32{});
    schema->addLeaf("/", "example:readonly", yang::Int32{}, yang::AccessType::ReadOnly);
    schema->addLeafList("/", "example:addresses", yang::String{});
    schema->addRpc("/", "second:fire");
    schema->addLeaf("/second:fire", "second:whom", yang::String{});
    schema->addContainer("/", "example:system");
    schema->addContainer("/example:system", "example:thing");
    schema->addContainer("/", "example:system-state");
    schema->addLeaf("/example:anoda", "example:iid", yang::InstanceIdentifier{});
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
    Parser parser(schema, WritableOps::No, dataQuery);
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
            expectedCompletions = {"example:addresses/", "example:ano/", "example:anoda/", "example:bota/", "example:leafInt ", "example:list/", "example:ovoce/", "example:readonly ", "example:ovocezelenina/", "example:twoKeyList/", "second:amelie/", "second:fire/", "example:system/", "example:system-state/"};
            expectedContextLength = 0;
        }

        SECTION("ls e")
        {
            input = "ls e";
            expectedCompletions = {"example:addresses/", "example:ano/", "example:anoda/", "example:bota/", "example:leafInt ", "example:list/", "example:ovoce/", "example:readonly ", "example:ovocezelenina/", "example:twoKeyList/", "example:system/", "example:system-state/"};
            expectedContextLength = 1;
        }

        SECTION("ls example:ano")
        {
            input = "ls example:ano";
            expectedCompletions = {"example:ano/", "example:anoda/"};
            expectedContextLength = 11;
        }

        SECTION("ls example:ano/a")
        {
            input = "ls example:ano/a";
            expectedCompletions = {"a2/"};
            expectedContextLength = 1;
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
            expectedCompletions = {"example:addresses/", "example:ano/", "example:anoda/", "example:bota/", "example:leafInt ", "example:list/", "example:ovoce/", "example:readonly ", "example:ovocezelenina/", "example:twoKeyList/", "second:amelie/", "second:fire/", "example:system/", "example:system-state/"};
            expectedContextLength = 0;
        }

        SECTION("ls /e")
        {
            input = "ls /e";
            expectedCompletions = {"example:addresses/", "example:ano/", "example:anoda/", "example:bota/", "example:leafInt ", "example:list/", "example:ovoce/", "example:readonly ", "example:ovocezelenina/", "example:twoKeyList/", "example:system/", "example:system-state/"};
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
            expectedCompletions = {"second:amelie/", "second:fire/"};
            expectedContextLength = 1;
        }

        SECTION("ls /example:list")
        {
            input = "ls /example:list";
            expectedCompletions = {"example:list/"};
            expectedContextLength = 12;
        }

        SECTION("ls /example:list/")
        {
            input = "ls /example:list/";
            expectedCompletions = {"contInList/", "number "};
            expectedContextLength = 0;
        }

        SECTION("ls /example:list[number=3]/")
        {
            input = "ls /example:list[number=3]/";
            expectedCompletions = {"contInList/", "number "};
            expectedContextLength = 0;
        }

        SECTION("ls /example:list[number=3]/c")
        {
            input = "ls /example:list[number=3]/c";
            expectedCompletions = {"contInList/"};
            expectedContextLength = 1;
        }

        SECTION("ls /example:list[number=3]/n")
        {
            input = "ls /example:list[number=3]/n";
            expectedCompletions = {"number "};
            expectedContextLength = 1;
        }

        SECTION("ls /example:list[number=3]/a")
        {
            input = "ls /example:list[number=3]/a";
            expectedCompletions = {};
            expectedContextLength = 1;
        }

        SECTION("ls /example:list/contInList/")
        {
            input = "ls /example:list/contInList/";
            expectedCompletions = {};
            expectedContextLength = 0;
        }

        SECTION("ls /example:system-")
        {
            input = "ls /example:system-";
            expectedCompletions = {"example:system-state/"};
            expectedContextLength = 15;
        }

        SECTION("set example:anoda/iid ")
        {
            input = "set example:anoda/iid ";
            expectedCompletions = {"/"};
            expectedContextLength = 0;
        }

        SECTION("set example:anoda/iid /")
        {
            input = "set example:anoda/iid /";
            expectedCompletions = {
                "example:addresses",
                "example:ano/",
                "example:anoda/",
                "example:bota/",
                "example:leafInt ",
                "example:list",
                "example:ovoce",
                "example:ovocezelenina",
                "example:readonly ",
                "example:system-state/",
                "example:system/",
                "example:twoKeyList",
                "second:amelie/",
                "second:fire/",
            };
            expectedContextLength = 0;
        }

        SECTION("set example:anoda/iid /example:read")
        {
            input = "set example:anoda/iid /example:read";
            expectedCompletions = {"example:readonly "};
            expectedContextLength = 12;
        }
    }

    SECTION("get completion")
    {
        SECTION("get /example:ano/l")
        {
            input = "get /example:ano/l";
            expectedCompletions = {"listInCont"};
            expectedContextLength = 1;
        }

        SECTION("get /example:list/")
        {
            input = "get /example:list/";
            expectedCompletions = {};
            // The expectedContextLength is 13, because the completion isn't actually generated after the slash.
            expectedContextLength = 13;
        }

        SECTION("get -")
        {
            input = "get -";
            expectedCompletions = {"-datastore "};
            expectedContextLength = 0;
        }

        SECTION("get --datastore ")
        {
            input = "get --datastore ";
            expectedCompletions = {"operational", "running", "startup"};
            expectedContextLength = 0;
        }

        SECTION("get --datastore running /second")
        {
            input = "get --datastore running /second";
            expectedCompletions = {"second:amelie/"};
            expectedContextLength = 6;
        }
    }

    SECTION("describe completion")
    {
        SECTION("path with list at the end")
        {
            input = "describe /example:list/";
            expectedCompletions = {"contInList/", "number "};
            expectedContextLength = 0;
        }
    }

    SECTION("list keys completion")
    {
        SECTION("cd example:lis")
        {
            input = "cd example:lis";
            expectedCompletions = {"example:list"};
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
            expectedCompletions = {"example:ovoce", "example:ovocezelenina"};
            expectedContextLength = 13;
        }

        SECTION("cd example:ovoceze")
        {
            input = "cd example:ovoceze";
            expectedCompletions = {"example:ovocezelenina"};
            expectedContextLength = 15;
        }

        SECTION("cd example:ovocezelenina")
        {
            input = "cd example:ovocezelenina";
            expectedCompletions = {"example:ovocezelenina["};
            expectedContextLength = 21;
        }
    }

    SECTION("no readonly data leafs in set")
    {
        input = "set example:read";
        expectedContextLength = 12;
    }

    SECTION("clear completions when no longer inputting path")
    {
        input = "set example:leafInt ";
        expectedCompletions = {};
        expectedContextLength = 0;
    }

    SECTION("rpc input nodes NOT completed for prepare command")
    {
        input = "prepare example:fire/";
        expectedCompletions = {};
        expectedContextLength = 13;
    }

    SECTION("rpc input nodes completed for set command")
    {
        parser.changeNode({{}, {{module_{"second"}, rpcNode_{"fire"}}}});
        input = "set ";
        expectedCompletions = {"whom "};
        expectedContextLength = 0;
    }

    SECTION("completion for other stuff while inside an rpc")
    {
        parser.changeNode({{}, {{module_{"second"}, rpcNode_{"fire"}}}});
        input = "set ../";
        expectedCompletions = {"example:addresses", "example:ano/", "example:anoda/", "example:bota/", "example:leafInt ", "example:list", "example:ovoce", "example:ovocezelenina", "example:twoKeyList", "second:amelie/", "second:fire/", "example:system/", "example:system-state/"};
        expectedContextLength = 0;
    }

    SECTION("rpc nodes not completed for the get command")
    {
        input = "get ";
        expectedCompletions = {"example:addresses", "example:ano/", "example:anoda/", "example:bota/", "example:leafInt ", "example:list", "example:ovoce", "example:ovocezelenina", "example:readonly ", "example:twoKeyList", "second:amelie/", "example:system/", "example:system-state/"};
        expectedContextLength = 0;
    }

    SECTION("leafs not completed for cd")
    {
        input = "cd ";
        expectedCompletions = {"example:addresses", "example:ano/", "example:anoda/", "example:bota/","example:list", "example:ovoce", "example:ovocezelenina", "example:twoKeyList", "second:amelie/", "example:system/", "example:system-state/"};
        expectedContextLength = 0;
    }

    REQUIRE(parser.completeCommand(input, errorStream) == (Completions{expectedCompletions, expectedContextLength}));
}
