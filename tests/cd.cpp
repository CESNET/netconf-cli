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

TEST_CASE("cd")
{
    auto schema = std::make_shared<StaticSchema>();
    schema->addModule("example");
    schema->addModule("second");
    schema->addContainer("", "example:a");
    schema->addContainer("", "second:a");
    schema->addContainer("", "example:b");
    schema->addContainer("example:a", "example:a2");
    schema->addContainer("example:b", "example:b2");
    schema->addContainer("example:a/example:a2", "example:a3");
    schema->addContainer("example:b/example:b2", "example:b3");
    schema->addList("", "example:list", {"number"});
    schema->addContainer("example:list", "example:contInList");
    schema->addList("", "example:twoKeyList", {"number", "name"});
    auto mockDatastore = std::make_shared<MockDatastoreAccess>();

    // The parser will use DataQuery for key value completion, but I'm not testing that here, so I don't return anything.
    ALLOW_CALL(*mockDatastore, getItems("/example:list"))
        .RETURN(std::map<std::string, leaf_data_>{});
    ALLOW_CALL(*mockDatastore, getItems("/example:twoKeyList"))
        .RETURN(std::map<std::string, leaf_data_>{});

    // DataQuery gets the schema from DatastoreAccess once
    auto expectation = NAMED_REQUIRE_CALL(*mockDatastore, schema())
        .RETURN(schema);
    auto dataQuery = std::make_shared<DataQuery>(*mockDatastore);
    expectation.reset();
    Parser parser(schema, dataQuery);
    std::string input;
    std::ostringstream errorStream;


    SECTION("valid input")
    {
        cd_ expected;

        SECTION("container")
        {
            SECTION("example:a")
            {
                SECTION("trailing slash")
                {
                    input = "cd example:a/";
                    expected.m_path.m_trailingSlash = TrailingSlash::Present;
                }
                SECTION("no trailing slash")
                {
                    input = "cd example:a";
                }
                expected.m_path.m_nodes.push_back(dataNode_(module_{"example"}, container_("a")));
            }

            SECTION("second:a")
            {
                SECTION("trailing slash")
                {
                    input = "cd second:a/";
                    expected.m_path.m_trailingSlash = TrailingSlash::Present;
                }
                SECTION("no trailing slash")
                {
                    input = "cd second:a";
                }
                expected.m_path.m_nodes.push_back(dataNode_(module_{"second"}, container_("a")));
            }

            SECTION("example:b")
            {
                input = "cd example:b";
                expected.m_path.m_nodes.push_back(dataNode_(module_{"example"}, container_("b")));
            }

            SECTION("example:a/a2")
            {
                input = "cd example:a/a2";
                expected.m_path.m_nodes.push_back(dataNode_(module_{"example"}, container_("a")));
                expected.m_path.m_nodes.push_back(dataNode_(container_("a2")));
            }

            SECTION("example:a/example:a2")
            {
                input = "cd example:a/example:a2";
                expected.m_path.m_nodes.push_back(dataNode_(module_{"example"}, container_("a")));
                expected.m_path.m_nodes.push_back(dataNode_(module_{"example"}, container_("a2")));
            }

            SECTION("example:b/b2")
            {
                input = "cd example:b/b2";
                expected.m_path.m_nodes.push_back(dataNode_(module_{"example"}, container_("b")));
                expected.m_path.m_nodes.push_back(dataNode_(container_("b2")));
            }
        }

        SECTION("list elements")
        {
            SECTION("example:list[number=1]")
            {
                input = "cd example:list[number=1]";
                auto keys = std::map<std::string, std::string>{
                    {"number", "1"}};
                expected.m_path.m_nodes.push_back(dataNode_(module_{"example"}, listElement_("list", keys)));
            }

            SECTION("example:list[number=1]/contInList")
            {
                input = "cd example:list[number=1]/contInList";
                auto keys = std::map<std::string, std::string>{
                    {"number", "1"}};
                expected.m_path.m_nodes.push_back(dataNode_(module_{"example"}, listElement_("list", keys)));
                expected.m_path.m_nodes.push_back(dataNode_(container_("contInList")));
            }

            SECTION("example:twoKeyList[number=4][name='abcd']")
            {
                input = "cd example:twoKeyList[number=4][name='abcd']";
                auto keys = std::map<std::string, std::string>{
                    {"number", "4"},
                    {"name", "abcd"}};
                expected.m_path.m_nodes.push_back(dataNode_(module_{"example"}, listElement_("twoKeyList", keys)));
            }
        }

        SECTION("whitespace handling")
        {
            SECTION("  cd   example:a     ")
            {
                input = "  cd   example:a     ";
                expected.m_path.m_nodes.push_back(dataNode_(module_{"example"}, container_("a")));
            }
        }

        SECTION("moving up")
        {
            SECTION("example:a/..")
            {
                input = "cd example:a/..";
                expected.m_path.m_nodes.push_back(dataNode_(module_{"example"}, container_("a")));
                expected.m_path.m_nodes.push_back(dataNode_(nodeup_()));
            }

            SECTION("example:a/../example:a")
            {
                input = "cd example:a/../example:a";
                expected.m_path.m_nodes.push_back(dataNode_(module_{"example"}, container_("a")));
                expected.m_path.m_nodes.push_back(dataNode_(nodeup_()));
                expected.m_path.m_nodes.push_back(dataNode_(module_{"example"}, container_("a")));
            }

            SECTION("example:a/../example:a/a2")
            {
                input = "cd example:a/../example:a/a2";
                expected.m_path.m_nodes.push_back(dataNode_(module_{"example"}, container_("a")));
                expected.m_path.m_nodes.push_back(dataNode_(nodeup_()));
                expected.m_path.m_nodes.push_back(dataNode_(module_{"example"}, container_("a")));
                expected.m_path.m_nodes.push_back(dataNode_(container_("a2")));
            }
        }

        command_ command = parser.parseCommand(input, errorStream);
        REQUIRE(command.type() == typeid(cd_));
        REQUIRE(boost::get<cd_>(command) == expected);
    }
    SECTION("invalid input")
    {
        SECTION("missing space between a command and its arguments")
        {
            SECTION("cdexample:a")
            {
                input = "cdexample:a";
            }
        }

        SECTION("whitespace between module and nodename")
        {
            SECTION("cd example: a")
            {
                input = "cd example: a";
            }

            SECTION("cd example : a")
            {
                input = "cd example : a";
            }

            SECTION("cd example :a")
            {
                input = "cd example :a";
            }
        }

        SECTION("entering modules")
        {
            SECTION("cd example")
            {
                input = "cd example";
            }

            SECTION("cd example:")
            {
                input = "cd example:";
            }
        }

        SECTION("garbage arguments handling")
        {
            SECTION("cd example:a garbage")
            {
                input = "cd example:a garbage";
            }
            SECTION("cd example:a/a2 garbage")
            {
                input = "cd example:a/a2 garbage";
            }
        }

        SECTION("invalid node identifiers")
        {
            SECTION("example:nonexistent")
            {
                input = "cd example:nonexistent";
            }

            SECTION("example:nonexistent/lol")
            {
                input = "cd example:nonexistent/lol";
            }
        }

        SECTION("invalid module identifiers")
        {
            SECTION("elpmaxe:nonexistent")
            {
                input = "cd elpmaxe:nonexistent";
            }

            SECTION("elpmaxe:nonexistent/example:lol")
            {
                input = "cd elpmaxe:nonexistent/example:lol";
            }
        }

        SECTION("no top-level module")
        {
            SECTION("cd a")
            {
                input = "cd a";
            }

            SECTION("cd example:a/../a")
            {
                input = "cd example:a/../a";
            }
        }

        SECTION("invalid list key identifiers")
        {
            SECTION("example:list")
            {
                input = "cd example:list";
            }

            SECTION("example:list[]")
            {
                input = "cd example:list[]";
            }

            SECTION("example:twoKeyList[invalidKey='4']")
            {
                input = "cd example:twoKeyList[invalidKey='4']";
            }

            SECTION("example:twoKeyList[number=4][number=5]")
            {
                input = "cd example:twoKeyList[number=4][number=5]";
            }

            SECTION("example:twoKeyList[number=4][name='lol'][number=7]")
            {
                input = "cd example:twoKeyList[number=4][name='lol'][number=7]";
            }

            SECTION("example:twoKeyList[number=4]")
            {
                input = "cd example:twoKeyList[number=4]";
            }

            SECTION("strings must be quoted")
            {
                input = "cd example:twoKeyList[number=4][name=abcd]";
            }
        }
        REQUIRE_THROWS_AS(parser.parseCommand(input, errorStream), InvalidCommandException);
    }
}
