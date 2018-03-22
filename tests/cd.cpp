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

    CParser parser(tree);
    cd_ expected;

    std::string input;

    SECTION("basic cd parsing")
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

        cd_ command = parser.parseCommand(input);
        REQUIRE(command == expected);
    }

    SECTION("InvalidNodeException")
    {
        SECTION("x")
        {
            input = "cd x";
            REQUIRE_THROWS_AS(parser.parseCommand(input), InvalidNodeException);
        }

        SECTION("a/x")
        {
            input = "cd a/x";
            REQUIRE_THROWS_AS(parser.parseCommand(input), InvalidNodeException);
        }
    }
}
