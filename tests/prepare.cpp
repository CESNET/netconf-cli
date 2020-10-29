
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

TEST_CASE("prepare command")
{
    auto schema = std::make_shared<StaticSchema>();
    schema->addModule("example");
    schema->addRpc("/", "example:fire");
    schema->addList("/", "example:port", {"name"});
    schema->addLeaf("/example:port", "example:name", yang::String{});
    schema->addAction("/example:port", "example:shutdown");
    Parser parser(schema);
    std::string input;
    std::ostringstream errorStream;
    prepare_ expected;
    expected.m_path.m_scope = Scope::Relative;
    SECTION("rpc")
    {
        input = "prepare example:fire";
        expected.m_path.m_nodes.push_back({module_{"example"}, rpcNode_{"fire"}});
    }

    SECTION("action")
    {
        input = "prepare example:port[name='eth0']/shutdown";
        expected.m_path.m_nodes.push_back({module_{"example"}, listElement_{"port", {{"name", std::string{"eth0"}}}}});
        expected.m_path.m_nodes.push_back({actionNode_{"shutdown"}});
    }

    command_ command = parser.parseCommand(input, errorStream);
    REQUIRE(command.type() == typeid(prepare_));
    auto lol = boost::get<prepare_>(command);
    REQUIRE(boost::get<prepare_>(command) == expected);
}
