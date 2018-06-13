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
    schema->addContainer("", "a");
    schema->addContainer("", "b");
    schema->addContainer("a", "a2");
    schema->addContainer("b", "b2");
    schema->addContainer("a/a2", "a3");
    schema->addContainer("b/b2", "b3");
    schema->addList("", "list", {"number"});
    schema->addContainer("list", "contInList");
    schema->addList("", "twoKeyList", {"number", "name"});
    Parser parser(schema);
    std::string input;
    std::ostringstream errorStream;

    SECTION("valid input")
    {
        cd_ expected;

        SECTION("container")
        {
            SECTION("a")
            {
                input = "cd a";
                expected.m_path.m_nodes.push_back(container_("a"));
            }

            SECTION("b")
            {
                input = "cd b";
                expected.m_path.m_nodes.push_back(container_("b"));
            }

            SECTION("a/a2")
            {
                input = "cd a/a2";
                expected.m_path.m_nodes.push_back(container_("a"));
                expected.m_path.m_nodes.push_back(container_("a2"));
            }

            SECTION("b/b2")
            {
                input = "cd b/b2";
                expected.m_path.m_nodes.push_back(container_("b"));
                expected.m_path.m_nodes.push_back(container_("b2"));
            }
        }

        SECTION("list elements")
        {
            SECTION("list[number=1]")
            {
                input = "cd list[number=1]";
                auto keys = std::map<std::string, std::string>{
                    {"number", "1"}};
                expected.m_path.m_nodes.push_back(listElement_("list", keys));
            }

            SECTION("list[number=1]/contInList")
            {
                input = "cd list[number=1]/contInList";
                auto keys = std::map<std::string, std::string>{
                    {"number", "1"}};
                expected.m_path.m_nodes.push_back(listElement_("list", keys));
                expected.m_path.m_nodes.push_back(container_("contInList"));
            }

            SECTION("twoKeyList[number=4 name=abcd]")
            {
                input = "cd twoKeyList[number=4 name=abcd]";
                auto keys = std::map<std::string, std::string>{
                    {"number", "4"},
                    {"name", "abcd"}};
                expected.m_path.m_nodes.push_back(listElement_("twoKeyList", keys));
            }
        }

        SECTION("whitespace handling")
        {
            SECTION("  cd   a     ")
            {
                input = "  cd   a     ";
                expected.m_path.m_nodes.push_back(container_("a"));
            }
        }

        SECTION("moving up")
        {
            SECTION("a/..")
            {
                input = "cd a/..";
                expected.m_path.m_nodes.push_back(container_("a"));
                expected.m_path.m_nodes.push_back(nodeup_());
            }

            SECTION("a/../a")
            {
                input = "cd a/../a";
                expected.m_path.m_nodes.push_back(container_("a"));
                expected.m_path.m_nodes.push_back(nodeup_());
                expected.m_path.m_nodes.push_back(container_("a"));
            }

            SECTION("a/../a/a2")
            {
                input = "cd a/../a/a2";
                expected.m_path.m_nodes.push_back(container_("a"));
                expected.m_path.m_nodes.push_back(nodeup_());
                expected.m_path.m_nodes.push_back(container_("a"));
                expected.m_path.m_nodes.push_back(container_("a2"));
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
