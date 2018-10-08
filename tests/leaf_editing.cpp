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
    schema->addModule("mod");
    schema->addContainer("", "mod:contA");
    schema->addLeaf("", "mod:leafString", yang::LeafDataTypes::String);
    schema->addLeaf("", "mod:leafDecimal", yang::LeafDataTypes::Decimal);
    schema->addLeaf("", "mod:leafBool", yang::LeafDataTypes::Bool);
    schema->addLeaf("", "mod:leafInt", yang::LeafDataTypes::Int);
    schema->addLeaf("", "mod:leafUint", yang::LeafDataTypes::Uint);
    schema->addLeafEnum("", "mod:leafEnum", {"lol", "data", "coze"});
    schema->addLeaf("mod:contA", "mod:leafInCont", yang::LeafDataTypes::String);
    schema->addList("", "mod:list", {"number"});
    schema->addLeaf("mod:list", "mod:leafInList", yang::LeafDataTypes::String);
    Parser parser(schema);
    std::string input;
    std::ostringstream errorStream;

    SECTION("valid input")
    {
        set_ expected;

        SECTION("set leafString some_data")
        {
            input = "set mod:leafString some_data";
            expected.m_path.m_nodes.push_back(dataNode_{module_{"mod"}, leaf_("leafString")});
            expected.m_data = std::string("some_data");
        }

        SECTION("set mod:contA/leafInCont more_data")
        {
            input = "set mod:contA/leafInCont more_data";
            expected.m_path.m_nodes.push_back(dataNode_{module_{"mod"}, container_("contA")});
            expected.m_path.m_nodes.push_back(dataNode_{leaf_("leafInCont")});
            expected.m_data = std::string("more_data");
        }

        SECTION("set list[number=1]/leafInList another_data")
        {
            input = "set mod:list[number=1]/leafInList another_data";
            auto keys = std::map<std::string, std::string>{
                {"number", "1"}};
            expected.m_path.m_nodes.push_back(dataNode_{module_{"mod"}, listElement_("list", keys)});
            expected.m_path.m_nodes.push_back(dataNode_{leaf_("leafInList")});
            expected.m_data = std::string("another_data");
        }

        SECTION("data types")
        {
            SECTION("string")
            {
                input = "set mod:leafString somedata";
                expected.m_path.m_nodes.push_back(dataNode_{module_{"mod"}, leaf_("leafString")});
                expected.m_data = std::string("somedata");
            }

            SECTION("int")
            {
                input = "set mod:leafInt 2";
                expected.m_path.m_nodes.push_back(dataNode_{module_{"mod"}, leaf_("leafInt")});
                expected.m_data = 2;
            }

            SECTION("decimal")
            {
                input = "set mod:leafDecimal 3.14159";
                expected.m_path.m_nodes.push_back(dataNode_{module_{"mod"}, leaf_("leafDecimal")});
                expected.m_data = 3.14159;
            }

            SECTION("enum")
            {
                input = "set mod:leafEnum coze";
                expected.m_path.m_nodes.push_back(dataNode_{module_{"mod"}, leaf_("leafEnum")});
                expected.m_data = enum_("coze");
            }

            SECTION("bool")
            {
                input = "set mod:leafBool true";
                expected.m_path.m_nodes.push_back(dataNode_{module_{"mod"}, leaf_("leafBool")});
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

        REQUIRE_THROWS_AS(parser.parseCommand(input, errorStream), InvalidCommandException&);
    }
}
