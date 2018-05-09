/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/

#include "trompeloeil_catch.h"
#include "CParser.hpp"
#include "CTree.hpp"
#include "ast.hpp"

TEST_CASE("cd")
{
    CTree tree;
    tree.addContainer("", "a");
    tree.addContainer("", "b");
    tree.addContainer("a", "a2");
    tree.addContainer("b", "b2");
    tree.addContainer("a/a2", "a3");
    tree.addContainer("b/b2", "b3");
    tree.addList("", "list", {"number"});
    tree.addContainer("list", "contInList");
    tree.addList("", "twoKeyList", {"number", "name"});
    CParser parser(tree);
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
                        {"number", "1"}
                };
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
        cd_ command = parser.parseCommand(input, errorStream);
        REQUIRE(command == expected);
    }
    SECTION("invalid input")
    {
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
