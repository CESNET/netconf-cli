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
#include "schema.hpp"

TEST_CASE("leaf editing")
{
    Schema schema;
    schema.addContainer("", "contA");
    schema.addLeaf("", "leaf");
    schema.addLeaf("contA", "leafInCont");
    schema.addList("", "list", {"number"});
    schema.addLeaf("list", "leafInList");
    Parser parser(schema);
    std::string input;
    std::ostringstream errorStream;

    SECTION("valid input")
    {
        set_ expected;

        SECTION("set leaf some_data")
        {
            input = "set leaf some_data";
            expected.m_path.m_nodes.push_back(leaf_("leaf"));
            expected.m_data = "some_data";
        }

        SECTION("set contA/leafInCont more_data")
        {
            input = "set contA/leafInCont more_data";
            expected.m_path.m_nodes.push_back(container_("contA"));
            expected.m_path.m_nodes.push_back(leaf_("leafInCont"));
            expected.m_data = "more_data";
        }

        command_ command = parser.parseCommand(input, errorStream);
        REQUIRE(command.type() == typeid(set_));
        REQUIRE(boost::get<set_>(command) == expected);
    }

    SECTION("invalid input")
    {
        SECTION("missing space between a command and its arguments")
        {
            SECTION("setleaf some_data")
            {
                input = "setleaf some_data";
            }
        }

        SECTION("missing space between arguments")
        {
            SECTION("set leaflol")
            {
                input = "set leaflol";
            }
        }

        SECTION("non-leaf identifiers")
        {
            SECTION("set nonexistent blabla")
            {
                input = "set nonexistent blabla";
            }

            SECTION("set contA abde")
            {
                input = "set contA abde";
            }
        }

        REQUIRE_THROWS(parser.parseCommand(input, errorStream));
    }
}
