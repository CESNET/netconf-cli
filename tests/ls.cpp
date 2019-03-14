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

TEST_CASE("ls")
{
    auto schema = std::make_shared<StaticSchema>();
    schema->addModule("example");
    schema->addModule("second");
    schema->addContainer("", "example:a");
    schema->addList("example:a", "example:listInCont", {"number"});
    schema->addContainer("", "second:a");
    schema->addContainer("", "example:b");
    schema->addContainer("example:a", "example:a2");
    schema->addContainer("example:b", "example:b2");
    schema->addContainer("example:a/example:a2", "example:a3");
    schema->addContainer("example:b/example:b2", "example:b3");
    schema->addList("", "example:list", {"number"});
    schema->addContainer("example:list", "example:contInList");
    schema->addList("", "example:twoKeyList", {"number", "name"});
    Parser parser(schema);
    std::string input;
    std::ostringstream errorStream;

    SECTION("valid input")
    {
        ls_ expected;

        SECTION("no arguments")
        {
            SECTION("ls")
                input = "ls";

            SECTION("ls --recursive")
            {
                input = "ls --recursive";
                expected.m_options.push_back(LsOption::Recursive);
            }
        }

        SECTION("with path argument")
        {
            SECTION("ls example:a")
            {
                input = "ls example:a";
                expected.m_path = dataPath_{Scope::Relative, {dataNode_(module_{"example"}, container_{"a"})}};
            }

            SECTION("ls /example:a")
            {
                SECTION("cwd: /") {}
                SECTION("cwd: /example:a") { parser.changeNode(dataPath_{Scope::Relative, {dataNode_(module_{"example"}, container_{"a"})}}); }

                input = "ls /example:a";
                expected.m_path = dataPath_{Scope::Absolute, {dataNode_(module_{"example"}, container_{"a"})}};
            }

            SECTION("ls /")
            {
                SECTION("cwd: /") {}
                SECTION("cwd: /example:a") { parser.changeNode(dataPath_{Scope::Relative, {dataNode_(module_{"example"}, container_{"a"})}}); }
                input = "ls /";
                expected.m_path = dataPath_{Scope::Absolute, {}};
            }

            SECTION("ls example:a/a2")
            {
                input = "ls example:a/a2";
                expected.m_path = dataPath_{Scope::Relative, {dataNode_(module_{"example"}, container_{"a"}),
                                                          dataNode_(container_{"a2"})}};
            }

            SECTION("ls a2")
            {
                parser.changeNode(dataPath_{Scope::Relative, {dataNode_(module_{"example"}, container_{"a"})}});
                input = "ls a2";
                expected.m_path = dataPath_{Scope::Relative, {dataNode_(container_{"a2"})}};
            }

            SECTION("ls /example:a/a2")
            {
                SECTION("cwd: /") {}
                SECTION("cwd: /example:a") { parser.changeNode(dataPath_{Scope::Relative, {dataNode_(module_{"example"}, container_{"a"})}}); }
                input = "ls /example:a/a2";
                expected.m_path = dataPath_{Scope::Absolute, {dataNode_(module_{"example"}, container_{"a"}),
                                                          dataNode_(container_{"a2"})}};
            }

            SECTION("ls example:a/example:a2")
            {
                input = "ls example:a/example:a2";
                expected.m_path = dataPath_{Scope::Relative, {dataNode_(module_{"example"}, container_{"a"}),
                                                          dataNode_(module_{"example"}, container_{"a2"})}};
            }

            SECTION("ls example:a2")
            {
                parser.changeNode(dataPath_{Scope::Relative, {dataNode_(module_{"example"}, container_{"a"})}});
                input = "ls example:a2";
                expected.m_path = dataPath_{Scope::Relative, {dataNode_(module_{"example"}, container_{"a2"})}};
            }

            SECTION("ls /example:a/example:a2")
            {
                SECTION("cwd: /") {}
                SECTION("cwd: /example:a") { parser.changeNode(dataPath_{Scope::Relative, {dataNode_(module_{"example"}, container_{"a"})}}); }

                input = "ls /example:a/example:a2";
                expected.m_path = dataPath_{Scope::Absolute, {dataNode_(module_{"example"}, container_{"a"}),
                                                          dataNode_(module_{"example"}, container_{"a2"})}};
            }

            SECTION("ls --recursive /example:a")
            {
                SECTION("cwd: /") {}
                SECTION("cwd: /example:a") { parser.changeNode(dataPath_{Scope::Relative, {dataNode_(module_{"example"}, container_{"a"})}}); }

                input = "ls --recursive /example:a";
                expected.m_options.push_back(LsOption::Recursive);
                expected.m_path = dataPath_{Scope::Absolute, {dataNode_(module_{"example"}, container_{"a"})}};
            }

            SECTION("ls example:list")
            {
                input = "ls example:list";
                expected.m_path = dataPath_{Scope::Relative, {dataNode_(module_{"example"}, list_{"list"})}};
            }

            SECTION("ls example:a/example:listInCont")
            {
                input = "ls example:a/example:listInCont";
                expected.m_path = dataPath_{Scope::Relative, {dataNode_(module_{"example"}, container_{"a"}),
                                                                dataNode_(module_{"example"}, list_{"listInCont"})}};
            }

            SECTION("ls example:list[number=342]/contInList")
            {
                input = "ls example:list[number=342]/contInList";
                auto keys = std::map<std::string, std::string>{
                    {"number", "342"}};
                expected.m_path = dataPath_{Scope::Relative, {dataNode_(module_{"example"}, listElement_{"list", keys}),
                                                                dataNode_(container_{"contInList"})}};
            }

            SECTION("ls example:list/contInList")
            {
                input = "ls example:list/contInList";
                expected.m_path = schemaPath_{Scope::Relative, {schemaNode_(module_{"example"}, list_{"list"}),
                                                                schemaNode_(container_{"contInList"})}};
            }
        }

        command_ command = parser.parseCommand(input, errorStream);
        REQUIRE(command.type() == typeid(ls_));
        REQUIRE(boost::get<ls_>(command) == expected);
    }

    SECTION("invalid input")
    {
        SECTION("invalid path")
        {
            SECTION("ls example:nonexistent")
                input = "ls example:nonexistent";

            SECTION("ls /example:nonexistent")
                input = "ls /example:nonexistent";

            SECTION("ls /bad:nonexistent")
                input = "ls /bad:nonexistent";

            SECTION("ls example:a/nonexistent")
                input = "ls example:a/nonexistent";

            SECTION("ls /example:a/nonexistent")
                input = "ls /example:a/nonexistent";
        }

        SECTION("whitespace before path")
        {
            SECTION("ls --recursive/")
                input = "ls --recursive/";

            SECTION("ls/")
                input = "ls/";

            SECTION("ls --recursive/example:a")
                input = "ls --recursive/example:a";

            SECTION("ls/example:a")
                input = "ls/example:a";

            SECTION("lssecond:a")
                input = "lssecond:a";
        }

        REQUIRE_THROWS_AS(parser.parseCommand(input, errorStream), InvalidCommandException);
    }
}
