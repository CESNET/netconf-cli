/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/

#include "trompeloeil_doctest.hpp"
#include "ast_commands.hpp"
#include "leaf_data_helpers.hpp"
#include "parser.hpp"
#include "pretty_printers.hpp"
#include "static_schema.hpp"

TEST_CASE("cd")
{
    auto schema = std::make_shared<StaticSchema>();
    schema->addModule("example");
    schema->addModule("second");
    schema->addContainer("/", "example:a");
    schema->addContainer("/", "second:a");
    schema->addContainer("/", "example:b");
    schema->addContainer("/example:a", "example:a2");
    schema->addContainer("/example:b", "example:b2");
    schema->addContainer("/example:a/example:a2", "example:a3");
    schema->addContainer("/example:b/example:b2", "example:b3");
    schema->addList("/", "example:list", {"number"});
    schema->addLeaf("/example:list", "example:number", yang::Int32{});
    schema->addContainer("/example:list", "example:contInList");
    schema->addList("/", "example:twoKeyList", {"number", "name"});
    schema->addLeaf("/example:twoKeyList", "example:number", yang::Int32{});
    schema->addLeaf("/example:twoKeyList", "example:name", yang::String{});
    schema->addRpc("/", "example:launch-nukes");
    schema->addList("/", "example:ports", {"name"});
    schema->addLeaf("/example:ports", "example:name", createEnum({"A", "B", "C"}));
    schema->addLeaf("/", "example:myLeaf", yang::Int32{});
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
                SECTION("trailing slash")
                {
                    input = "cd example:a/";
                    expected.m_path.m_trailingSlash = TrailingSlash::Present;
                }
                SECTION("no trailing slash")
                {
                    input = "cd example:a";
                }
                expected.m_path.m_nodes.emplace_back(module_{"example"}, container_("a"));
            }

            SECTION("second:a")
            {
                SECTION("trailing slash")
                {
                    input = "cd second:a/";
                    expected.m_path.m_trailingSlash = TrailingSlash::Present;
                }
                SECTION("no trailing slash")
                {
                    input = "cd second:a";
                }
                expected.m_path.m_nodes.emplace_back(module_{"second"}, container_("a"));
            }

            SECTION("example:b")
            {
                input = "cd example:b";
                expected.m_path.m_nodes.emplace_back(module_{"example"}, container_("b"));
            }

            SECTION("example:a/a2")
            {
                input = "cd example:a/a2";
                expected.m_path.m_nodes.emplace_back(module_{"example"}, container_("a"));
                expected.m_path.m_nodes.emplace_back(container_("a2"));
            }

            SECTION("example:a/example:a2")
            {
                input = "cd example:a/example:a2";
                expected.m_path.m_nodes.emplace_back(module_{"example"}, container_("a"));
                expected.m_path.m_nodes.emplace_back(module_{"example"}, container_("a2"));
            }

            SECTION("example:b/b2")
            {
                input = "cd example:b/b2";
                expected.m_path.m_nodes.emplace_back(module_{"example"}, container_("b"));
                expected.m_path.m_nodes.emplace_back(container_("b2"));
            }
        }

        SECTION("list elements")
        {
            SECTION("example:list[number=1]")
            {
                input = "cd example:list[number=1]";
                auto keys = ListInstance{
                    {"number", int32_t{1}}};
                expected.m_path.m_nodes.emplace_back(module_{"example"}, listElement_("list", keys));
            }

            SECTION("example:list[number=1]/contInList")
            {
                input = "cd example:list[number=1]/contInList";
                auto keys = ListInstance{
                    {"number", int32_t{1}}};
                expected.m_path.m_nodes.emplace_back(module_{"example"}, listElement_("list", keys));
                expected.m_path.m_nodes.emplace_back(container_("contInList"));
            }

            SECTION("example:twoKeyList[number=4][name='abcd']")
            {
                input = "cd example:twoKeyList[number=4][name='abcd']";
                auto keys = ListInstance{
                    {"number", int32_t{4}},
                    {"name", std::string{"abcd"}}};
                expected.m_path.m_nodes.emplace_back(module_{"example"}, listElement_("twoKeyList", keys));
            }

            SECTION("enum key type")
            {
                input = "cd example:ports[name=A]";
                auto keys = ListInstance{
                    {"name", enum_{"A"}}};
                expected.m_path.m_nodes.emplace_back(module_{"example"}, listElement_("ports", keys));
            }
        }

        SECTION("whitespace handling")
        {
            SECTION("  cd   example:a     ")
            {
                input = "  cd   example:a     ";
                expected.m_path.m_nodes.emplace_back(module_{"example"}, container_("a"));
            }
        }

        SECTION("moving up")
        {
            SECTION("moving up when already in root")
            {
                input = "cd ..";
                expected.m_path.m_nodes.emplace_back(nodeup_());
            }

            SECTION("moving up TWICE when already in root")
            {
                input = "cd ../..";
                expected.m_path.m_nodes.emplace_back(nodeup_());
                expected.m_path.m_nodes.emplace_back(nodeup_());
            }

            SECTION("example:a/..")
            {
                input = "cd example:a/..";
                expected.m_path.m_nodes.emplace_back(module_{"example"}, container_("a"));
                expected.m_path.m_nodes.emplace_back(nodeup_());
            }

            SECTION("example:a/../example:a")
            {
                input = "cd example:a/../example:a";
                expected.m_path.m_nodes.emplace_back(module_{"example"}, container_("a"));
                expected.m_path.m_nodes.emplace_back(nodeup_());
                expected.m_path.m_nodes.emplace_back(module_{"example"}, container_("a"));
            }

            SECTION("example:a/../example:a/a2")
            {
                input = "cd example:a/../example:a/a2";
                expected.m_path.m_nodes.emplace_back(module_{"example"}, container_("a"));
                expected.m_path.m_nodes.emplace_back(nodeup_());
                expected.m_path.m_nodes.emplace_back(module_{"example"}, container_("a"));
                expected.m_path.m_nodes.emplace_back(container_("a2"));
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
            SECTION("cdexample:a")
            {
                input = "cdexample:a";
            }
        }

        SECTION("whitespace between module and nodename")
        {
            SECTION("cd example: a")
            {
                input = "cd example: a";
            }

            SECTION("cd example : a")
            {
                input = "cd example : a";
            }

            SECTION("cd example :a")
            {
                input = "cd example :a";
            }
        }

        SECTION("entering modules")
        {
            SECTION("cd example")
            {
                input = "cd example";
            }

            SECTION("cd example:")
            {
                input = "cd example:";
            }
        }

        SECTION("garbage arguments handling")
        {
            SECTION("cd example:a garbage")
            {
                input = "cd example:a garbage";
            }
            SECTION("cd example:a/a2 garbage")
            {
                input = "cd example:a/a2 garbage";
            }
        }

        SECTION("invalid node identifiers")
        {
            SECTION("example:nonexistent")
            {
                input = "cd example:nonexistent";
            }

            SECTION("example:nonexistent/lol")
            {
                input = "cd example:nonexistent/lol";
            }
        }

        SECTION("invalid module identifiers")
        {
            SECTION("elpmaxe:nonexistent")
            {
                input = "cd elpmaxe:nonexistent";
            }

            SECTION("elpmaxe:nonexistent/example:lol")
            {
                input = "cd elpmaxe:nonexistent/example:lol";
            }
        }

        SECTION("no top-level module")
        {
            SECTION("cd a")
            {
                input = "cd a";
            }

            SECTION("cd example:a/../a")
            {
                input = "cd example:a/../a";
            }
        }

        SECTION("invalid list key identifiers")
        {
            SECTION("example:list")
            {
                input = "cd example:list";
            }

            SECTION("example:list[]")
            {
                input = "cd example:list[]";
            }

            SECTION("example:twoKeyList[invalidKey='4']")
            {
                input = "cd example:twoKeyList[invalidKey='4']";
            }

            SECTION("example:twoKeyList[number=4][number=5]")
            {
                input = "cd example:twoKeyList[number=4][number=5]";
            }

            SECTION("example:twoKeyList[number=4][name='lol'][number=7]")
            {
                input = "cd example:twoKeyList[number=4][name='lol'][number=7]";
            }

            SECTION("example:twoKeyList[number=4]")
            {
                input = "cd example:twoKeyList[number=4]";
            }

            SECTION("strings must be quoted")
            {
                input = "cd example:twoKeyList[number=4][name=abcd]";
            }
        }

        SECTION("no space between list prefix and suffix")
        {
            input = "cd example:list  [number=10]";
        }

        SECTION("cd into rpc")
        {
            input = "cd example:launch-nukes";
        }

        SECTION("cd into a leaf")
        {
            input = "cd example:myLeaf";
        }

        REQUIRE_THROWS_AS(parser.parseCommand(input, errorStream), InvalidCommandException);
    }
}
