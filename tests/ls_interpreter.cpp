/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/

#include <experimental/iterator>
#include "trompeloeil_doctest.hpp"
#include "ast_commands.hpp"
#include "interpreter.hpp"
#include "datastoreaccess_mock.hpp"
#include "parser.hpp"
#include "pretty_printers.hpp"
#include "static_schema.hpp"

class MockSchema : public trompeloeil::mock_interface<Schema> {
public:
    IMPLEMENT_CONST_MOCK1(defaultValue);
    IMPLEMENT_CONST_MOCK1(description);
    IMPLEMENT_CONST_MOCK2(availableNodes);
    IMPLEMENT_CONST_MOCK1(isConfig);
    MAKE_CONST_MOCK1(leafType, yang::TypeInfo(const std::string&), override);
    MAKE_CONST_MOCK2(leafType, yang::TypeInfo(const schemaPath_&, const ModuleNodePair&), override);
    IMPLEMENT_CONST_MOCK1(leafTypeName);
    IMPLEMENT_CONST_MOCK1(isModule);
    IMPLEMENT_CONST_MOCK1(leafrefPath);
    IMPLEMENT_CONST_MOCK3(listHasKey);
    IMPLEMENT_CONST_MOCK1(leafIsKey);
    IMPLEMENT_CONST_MOCK2(listKeys);
    MAKE_CONST_MOCK1(nodeType, yang::NodeTypes(const std::string&), override);
    MAKE_CONST_MOCK2(nodeType, yang::NodeTypes(const schemaPath_&, const ModuleNodePair&), override);
    IMPLEMENT_CONST_MOCK1(status);
};

TEST_CASE("ls interpreter")
{
    auto schema = std::make_shared<MockSchema>();
    Parser parser(schema);

    schemaPath_ expectedPath;
    boost::optional<boost::variant<dataPath_, schemaPath_, module_>> lsArg{boost::none};
    SECTION("cwd: /")
    {
        SECTION("arg: <none>")
        {
            expectedPath = schemaPath_{};
        }

        SECTION("arg: example:a")
        {
            lsArg = dataPath_{Scope::Relative, {{module_{"example"}, container_{"a"}}}};
            expectedPath = schemaPath_{Scope::Absolute, {{module_{"example"}, container_{"a"}}}};
        }

        SECTION("arg: example:list")
        {
            lsArg = dataPath_{Scope::Relative, {{module_{"example"}, list_{"list"}}}};
            expectedPath = schemaPath_{Scope::Absolute, {{module_{"example"}, list_{"list"}}}};
        }

        SECTION("arg: /example:a")
        {
            lsArg = dataPath_{Scope::Absolute, {{module_{"example"}, container_{"a"}}}};
            expectedPath = schemaPath_{Scope::Absolute, {{module_{"example"}, container_{"a"}}}};
        }

        SECTION("arg: /example:list")
        {
            lsArg = dataPath_{Scope::Absolute, {{module_{"example"}, list_{"list"}}}};
            expectedPath = schemaPath_{Scope::Absolute, {{module_{"example"}, list_{"list"}}}};
        }
    }

    SECTION("cwd: /example:a")
    {
        parser.changeNode({Scope::Relative, {{module_{"example"}, container_{"a"}}}});

        SECTION("arg: <none>")
        {
            expectedPath = schemaPath_{Scope::Absolute, {{module_{"example"}, container_{"a"}}}};
        }

        SECTION("arg: example:a2")
        {
            lsArg = dataPath_{Scope::Relative, {{container_{"a2"}}}};
            expectedPath = schemaPath_{Scope::Absolute, {{module_{"example"}, container_{"a"}}, {container_{"a2"}}}};
        }

        SECTION("arg: example:listInCont")
        {
            lsArg = dataPath_{Scope::Relative, {{list_{"listInCont"}}}};
            expectedPath = schemaPath_{Scope::Absolute, {{module_{"example"}, container_{"a"}}, {list_{"listInCont"}}}};
        }

        SECTION("arg: /example:a")
        {
            lsArg = dataPath_{Scope::Absolute, {{module_{"example"}, container_{"a"}}}};
            expectedPath = schemaPath_{Scope::Absolute, {{module_{"example"}, container_{"a"}}}};
        }

        SECTION("arg: /example:list")
        {
            lsArg = dataPath_{Scope::Absolute, {{module_{"example"}, list_{"list"}}}};
            expectedPath = schemaPath_{Scope::Absolute, {{module_{"example"}, list_{"list"}}}};
        }
    }
    SECTION("cwd: /example:list")
    {
        parser.changeNode({Scope::Relative, {{module_{"example"}, list_{"list"}}}});

        SECTION("arg: <none>")
        {
            expectedPath = schemaPath_{Scope::Absolute, {{module_{"example"}, list_{"list"}}}};
        }

        SECTION("arg: example:contInList")
        {
            lsArg = dataPath_{Scope::Relative, {{container_{"contInList"}}}};
            expectedPath = schemaPath_{Scope::Absolute, {{module_{"example"}, list_{"list"}}, {container_{"contInList"}}}};
        }

        SECTION("arg: /example:a")
        {
            lsArg = dataPath_{Scope::Absolute, {{module_{"example"}, container_{"a"}}}};
            expectedPath = schemaPath_{Scope::Absolute, {{module_{"example"}, container_{"a"}}}};
        }

        SECTION("arg: /example:list")
        {
            lsArg = dataPath_{Scope::Absolute, {{module_{"example"}, list_{"list"}}}};
            expectedPath = schemaPath_{Scope::Absolute, {{module_{"example"}, list_{"list"}}}};
        }
    }
    MockDatastoreAccess datastore;
    REQUIRE_CALL(datastore, schema()).RETURN(schema);
    ls_ ls;
    ls.m_path = lsArg;
    REQUIRE_CALL(*schema, availableNodes(expectedPath, Recursion::NonRecursive)).RETURN(std::set<ModuleNodePair>{});
    Interpreter(parser, datastore)(ls);
}
