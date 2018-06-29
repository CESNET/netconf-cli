/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/

#include "trompeloeil_catch.h"
#include "ast_commands.hpp"
#include "parser.hpp"
#include "static_schema.hpp"

TEST_CASE("cd")
{
    auto schema = std::make_shared<StaticSchema>();
    schema->addModule("example");
    schema->addContainer("", "example:a");
    schema->addContainer("", "example:b");
    schema->addContainer("example:a", "example:a2");
    schema->addContainer("example:b", "example:b2");
    schema->addContainer("example:a/example:a2", "example:a3");
    schema->addContainer("example:b/example:b2", "example:b3");
    schema->addList("", "example:list", {"number"});
    schema->addContainer("example:list", "example:contInList");
    schema->addList("", "example:twoKeyList", {"number", "name"});
    Parser parser(schema);
    std::string input;
    std::ostringstream errorStream;


    SECTION("valid input")
    {
        cd_ expected;

        SECTION("container")
        {
            SECTION("example:a")
            {
                input = "cd example:a";
                expected.m_path.m_nodes.push_back(node_(module_{"example"}, container_("a")));
            }

            SECTION("example:b")
            {
                input = "cd example:b";
                expected.m_path.m_nodes.push_back(node_(module_{"example"}, container_("b")));
            }

            SECTION("example:a/a2")
            {
                input = "cd example:a/a2";
                expected.m_path.m_nodes.push_back(node_(module_{"example"}, container_("a")));
                expected.m_path.m_nodes.push_back(node_(container_("a2")));
            }

            SECTION("example:b/b2")
            {
                input = "cd example:b/b2";
                expected.m_path.m_nodes.push_back(node_(module_{"example"}, container_("b")));
                expected.m_path.m_nodes.push_back(node_(container_("b2")));
            }
        }

        SECTION("list elements")
        {
            SECTION("example:list[number=1]")
            {
                input = "cd example:list[number=1]";
                auto keys = std::map<std::string, std::string>{
                    {"number", "1"}};
                expected.m_path.m_nodes.push_back(node_(module_{"example"}, listElement_("list", keys)));
            }

            SECTION("example:list[number=1]/contInList")
            {
                input = "cd example:list[number=1]/contInList";
                auto keys = std::map<std::string, std::string>{
                    {"number", "1"}};
                expected.m_path.m_nodes.push_back(node_(module_{"example"}, listElement_("list", keys)));
                expected.m_path.m_nodes.push_back(node_(container_("contInList")));
            }

            SECTION("example:twoKeyList[number=4 name=abcd]")
            {
                input = "cd example:twoKeyList[number=4 name=abcd]";
                auto keys = std::map<std::string, std::string>{
                    {"number", "4"},
                    {"name", "abcd"}};
                expected.m_path.m_nodes.push_back(node_(module_{"example"}, listElement_("twoKeyList", keys)));
            }
        }

        SECTION("whitespace handling")
        {
            SECTION("  cd   example:a     ")
            {
                input = "  cd   example:a     ";
                expected.m_path.m_nodes.push_back(node_(module_{"example"}, container_("a")));
            }
        }

        SECTION("moving up")
        {
            SECTION("example:a/..")
            {
                input = "cd example:a/..";
                expected.m_path.m_nodes.push_back(node_(module_{"example"}, container_("a")));
                expected.m_path.m_nodes.push_back(node_(nodeup_()));
            }

            SECTION("example:a/../example:a")
            {
                input = "cd example:a/../example:a";
                expected.m_path.m_nodes.push_back(node_(module_{"example"}, container_("a")));
                expected.m_path.m_nodes.push_back(node_(nodeup_()));
                expected.m_path.m_nodes.push_back(node_(module_{"example"}, container_("a")));
            }

            SECTION("example:a/../example:a/a2")
            {
                input = "cd example:a/../example:a/a2";
                expected.m_path.m_nodes.push_back(node_(module_{"example"}, container_("a")));
                expected.m_path.m_nodes.push_back(node_(nodeup_()));
                expected.m_path.m_nodes.push_back(node_(module_{"example"}, container_("a")));
                expected.m_path.m_nodes.push_back(node_(container_("a2")));
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
            SECTION("cda")
            {
                input = "cda";
            }
        }
        SECTION("garbage arguments handling")
        {
            SECTION("cd a garbage")
            {
                input = "cd a garbage";
            }
            SECTION("cd a/a2 garbage")
            {
                input = "cd a/a2 garbage";
            }
        }
        SECTION("invalid identifiers")
        {
            SECTION("nonexistent")
            {
                input = "cd nonexistent";
            }

            SECTION("nonexistent/lol")
            {
                input = "cd nonexistent/lol";
            }
        }

        SECTION("invalid list key identifiers")
        {
            SECTION("list")
            {
                input = "cd list";
            }

            SECTION("list[]")
            {
                input = "cd list[]";
            }

            SECTION("twoKeyList[invalidKey=4]")
            {
                input = "cd twoKeyList[invalidKey=4]";
            }

            SECTION("twoKeyList[number=4 number=5]")
            {
                input = "cd twoKeyList[number=4 number=5]";
            }

            SECTION("twoKeyList[number=4 name=lol number=7]")
            {
                input = "cd twoKeyList[number=4 name=lol number=7]";
            }

            SECTION("twoKeyList[number=4]")
            {
                input = "cd twoKeyList[number=4]";
            }
        }
        REQUIRE_THROWS(parser.parseCommand(input, errorStream));
    }
}
