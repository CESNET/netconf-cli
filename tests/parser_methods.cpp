/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/

#include <experimental/iterator>
#include "trompeloeil_doctest.h"
#include "ast_commands.hpp"
#include "datastoreaccess_mock.hpp"
#include "parser.hpp"
#include "static_schema.hpp"

namespace std {
std::ostream& operator<<(std::ostream& s, const std::set<std::string> set)
{
    s << std::endl
      << "{";
    std::copy(set.begin(), set.end(), std::experimental::make_ostream_joiner(s, ", "));
    s << "}" << std::endl;
    return s;
}
}

TEST_CASE("parser methods")
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
    auto mockDatastore = std::make_shared<MockDatastoreAccess>();

    // DataQuery gets the schema from DatastoreAccess once
    auto expectation = NAMED_REQUIRE_CALL(*mockDatastore, schema())
        .RETURN(schema);
    auto dataQuery = std::make_shared<DataQuery>(*mockDatastore);
    expectation.reset();
    Parser parser(schema, dataQuery);

    SECTION("availableNodes")
    {
        std::set<std::string> expected;
        boost::optional<boost::variant<boost::variant<dataPath_, schemaPath_>, module_>> arg{boost::none};
        SECTION("cwd: /")
        {
            SECTION("arg: <none>")
            {
                expected = {"example:a", "example:b", "example:list", "example:twoKeyList", "second:a"};
            }

            SECTION("arg: example:a")
            {
                arg = dataPath_{Scope::Relative, {{module_{"example"}, container_{"a"}}}};
                expected = {"example:a2", "example:listInCont"};
            }

            SECTION("arg: example:list")
            {
                arg = dataPath_{Scope::Relative, {{module_{"example"}, list_{"list"}}}};
                expected = {"example:contInList"};
            }

            SECTION("arg: /example:a")
            {
                arg = dataPath_{Scope::Absolute, {{module_{"example"}, container_{"a"}}}};
                expected = {"example:a2", "example:listInCont"};
            }

            SECTION("arg: /example:list")
            {
                arg = dataPath_{Scope::Absolute, {{module_{"example"}, list_{"list"}}}};
                expected = {"example:contInList"};
            }
        }
        SECTION("cwd: /example:a")
        {
            parser.changeNode({Scope::Relative, {{module_{"example"}, container_{"a"}}}});

            SECTION("arg: <none>")
            {
                expected = {"example:a2", "example:listInCont"};
            }

            SECTION("arg: example:a2")
            {
                arg = dataPath_{Scope::Relative, {{container_{"a2"}}}};
                expected = {"example:a3"};
            }

            SECTION("arg: example:listInCont")
            {
                arg = dataPath_{Scope::Relative, {{list_{"listInCont"}}}};
            }

            SECTION("arg: /example:a")
            {
                arg = dataPath_{Scope::Absolute, {{module_{"example"}, container_{"a"}}}};
                expected = {"example:a2", "example:listInCont"};
            }

            SECTION("arg: /example:list")
            {
                arg = dataPath_{Scope::Absolute, {{module_{"example"}, list_{"list"}}}};
                expected = {"example:contInList"};
            }
        }
        SECTION("cwd: /example:list")
        {
            parser.changeNode({Scope::Relative, {{module_{"example"}, list_{"list"}}}});

            SECTION("arg: <none>")
            {
                expected = {"example:contInList"};
            }

            SECTION("arg: example:contInList")
            {
                arg = dataPath_{Scope::Relative, {{container_{"contInList"}}}};
            }

            SECTION("arg: /example:a")
            {
                arg = dataPath_{Scope::Absolute, {{module_{"example"}, container_{"a"}}}};
                expected = {"example:a2", "example:listInCont"};
            }

            SECTION("arg: /example:list")
            {
                arg = dataPath_{Scope::Absolute, {{module_{"example"}, list_{"list"}}}};
                expected = {"example:contInList"};
            }
        }

        REQUIRE(expected == parser.availableNodes(arg, Recursion::NonRecursive));
    }
}
