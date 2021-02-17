
/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/

#include "trompeloeil_doctest.hpp"
#include "parser.hpp"
#include "pretty_printers.hpp"
#include "static_schema.hpp"

TEST_CASE("rpc")
{
    auto schema = std::make_shared<StaticSchema>();
    schema->addModule("example");
    schema->addRpc("/", "example:fire");
    schema->addRpc("/", "example:shutdown");
    schema->addLeaf("/example:shutdown/input", "example:interface", yang::String{});
    schema->addList("/", "example:port", {"name"});
    schema->addLeaf("/example:port", "example:name", yang::String{});
    schema->addAction("/example:port", "example:shutdown");
    Parser parser(schema);
    std::string input;
    std::ostringstream errorStream;
    SECTION("with prepare")
    {
        prepare_ prepExpected;
        prepExpected.m_path.m_scope = Scope::Relative;
        SECTION("rpc")
        {
            input = "prepare example:fire";
            prepExpected.m_path.m_nodes.emplace_back(module_{"example"}, rpcNode_{"fire"});
        }

        SECTION("action")
        {
            input = "prepare example:port[name='eth0']/shutdown";
            prepExpected.m_path.m_nodes.emplace_back(module_{"example"}, listElement_{"port", {{"name", std::string{"eth0"}}}});
            prepExpected.m_path.m_nodes.emplace_back(actionNode_{"shutdown"});
        }

        command_ command = parser.parseCommand(input, errorStream);
        auto lol = boost::get<prepare_>(command);
        REQUIRE(boost::get<prepare_>(command) == prepExpected);
    }

    SECTION("direct exec")
    {
        exec_ execExpected;
        execExpected.m_path = dataPath_{};
        execExpected.m_path->m_scope = Scope::Relative;
        SECTION("the rpc has no arguments")
        {
            input = "exec example:fire";
            execExpected.m_path->m_nodes.emplace_back(module_{"example"}, rpcNode_{"fire"});
            command_ command = parser.parseCommand(input, errorStream);
            auto lol = boost::get<exec_>(command);
            REQUIRE(boost::get<exec_>(command) == execExpected);
        }

        SECTION("the rpc has arguments")
        {
            input = "exec example:shutdown";
            REQUIRE_THROWS_AS(parser.parseCommand(input, errorStream), InvalidCommandException);
        }
    }

    SECTION("exec with no arguments")
    {
        exec_ execExpected;

        SECTION("without space")
        {
            input = "exec ";
        }

        SECTION("with space")
        {
            input = "exec ";
        }
        command_ command = parser.parseCommand(input, errorStream);
        auto lol = boost::get<exec_>(command);
        REQUIRE(boost::get<exec_>(command) == execExpected);
    }
}
