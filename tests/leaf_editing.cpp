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

TEST_CASE("leaf editing")
{
    auto schema = std::make_shared<StaticSchema>();
    schema->addContainer("", "contA");
    schema->addLeaf("", "leafString", yang::LeafDataTypes::String);
    schema->addLeaf("", "leafDecimal", yang::LeafDataTypes::Decimal);
    schema->addLeaf("", "leafBool", yang::LeafDataTypes::Bool);
    schema->addLeaf("", "leafInt", yang::LeafDataTypes::Int);
    schema->addLeaf("", "leafUint", yang::LeafDataTypes::Uint);
    schema->addLeafEnum("", "leafEnum", {"lol", "data", "coze"});
    schema->addLeaf("contA", "leafInCont", yang::LeafDataTypes::String);
    schema->addList("", "list", {"number"});
    schema->addLeaf("list", "leafInList", yang::LeafDataTypes::String);
    Parser parser(schema);
    std::string input;
    std::ostringstream errorStream;

    SECTION("valid input")
    {
        set_ expected;

        SECTION("set leafString some_data")
        {
            input = "set leafString some_data";
            expected.m_path.m_nodes.push_back(leaf_("leafString"));
            expected.m_data = std::string("some_data");
        }

        SECTION("set contA/leafInCont more_data")
        {
            input = "set contA/leafInCont more_data";
            expected.m_path.m_nodes.push_back(container_("contA"));
            expected.m_path.m_nodes.push_back(leaf_("leafInCont"));
            expected.m_data = std::string("more_data");
        }

        SECTION("set list[number=1]/leafInList another_data")
        {
            input = "set list[number=1]/leafInList another_data";
            auto keys = std::map<std::string, std::string>{
                {"number", "1"}};
            expected.m_path.m_nodes.push_back(listElement_("list", keys));
            expected.m_path.m_nodes.push_back(leaf_("leafInList"));
            expected.m_data = std::string("another_data");
        }

        SECTION("data types")
        {
            SECTION("string")
            {
                input = "set leafString somedata";
                expected.m_path.m_nodes.push_back(leaf_("leafString"));
                expected.m_data = std::string("somedata");
            }

            SECTION("int")
            {
                input = "set leafInt 2";
                expected.m_path.m_nodes.push_back(leaf_("leafInt"));
                expected.m_data = 2;
            }

            SECTION("decimal")
            {
                input = "set leafDecimal 3.14159";
                expected.m_path.m_nodes.push_back(leaf_("leafDecimal"));
                expected.m_data = 3.14159;
            }

            SECTION("enum")
            {
                input = "set leafEnum coze";
                expected.m_path.m_nodes.push_back(leaf_("leafEnum"));
                expected.m_data = enum_("coze");
            }

            SECTION("bool")
            {
                input = "set leafBool true";
                expected.m_path.m_nodes.push_back(leaf_("leafBool"));
                expected.m_data = true;
            }
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

        SECTION("wrong types")
        {
            SECTION("set leafBool blabla")
            {
                input = "set leafBool blabla";
            }
            SECTION("set leafUint blabla")
            {
                input = "set leafUint blabla";
            }
            SECTION("set leafInt blabla")
            {
                input = "set leafInt blabla";
            }
            SECTION("set leafEnum blabla")
            {
                input = "set leafEnum blabla";
            }
        }

        REQUIRE_THROWS(parser.parseCommand(input, errorStream));
    }
}
