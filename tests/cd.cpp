/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/

#include "CTree.hpp"
#include "ast.hpp"
#include "CParser.hpp"
#include "trompeloeil_catch.h"

TEST_CASE("cd")
{
    CTree tree;
    tree.addNode("", "a");
    tree.addNode("", "b");

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

        cd_ command = parser.parseCommand(input);
        REQUIRE(command == expected);
    }
}

