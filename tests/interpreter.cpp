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
    IMPLEMENT_CONST_MOCK2(listHasKey);
    IMPLEMENT_CONST_MOCK1(leafIsKey);
    IMPLEMENT_CONST_MOCK1(listKeys);
    MAKE_CONST_MOCK1(nodeType, yang::NodeTypes(const std::string&), override);
    MAKE_CONST_MOCK2(nodeType, yang::NodeTypes(const schemaPath_&, const ModuleNodePair&), override);
    IMPLEMENT_CONST_MOCK1(status);
};

TEST_CASE("interpreter tests")
{
    auto schema = std::make_shared<MockSchema>();
    Parser parser(schema);
    MockDatastoreAccess datastore;
    std::vector<std::unique_ptr<trompeloeil::expectation>> expectations;

    std::vector<command_> toInterpret;

    SECTION("ls")
    {
        boost::variant<dataPath_, schemaPath_, module_> expectedPath;
        boost::optional<boost::variant<dataPath_, schemaPath_, module_>> lsArg;
        SECTION("cwd: /")
        {
            SECTION("arg: <none>")
            {
                expectedPath = dataPath_{};
            }

            SECTION("arg: ..")
            {
                lsArg = dataPath_{Scope::Relative, {dataNode_{nodeup_{}}}};
                expectedPath = dataPath_{};
            }

            SECTION("arg: /..")
            {
                lsArg = dataPath_{Scope::Absolute, {dataNode_{nodeup_{}}}};
                expectedPath = dataPath_{Scope::Absolute, {}};
            }

            SECTION("arg: /example:a/../example:a")
            {
                lsArg = dataPath_{Scope::Absolute, {{module_{"example"}, container_{"a"}},
                                                    {nodeup_{}},
                                                    {module_{"example"}, container_{"a"}}}};
                expectedPath = dataPath_{Scope::Absolute, {{module_{"example"}, container_{"a"}}}};
            }

            SECTION("arg: example:a")
            {
                lsArg = dataPath_{Scope::Relative, {{module_{"example"}, container_{"a"}}}};
                expectedPath = dataPath_{Scope::Absolute, {{module_{"example"}, container_{"a"}}}};
            }

            SECTION("arg: example:list")
            {
                lsArg = dataPath_{Scope::Relative, {{module_{"example"}, list_{"list"}}}};
                expectedPath = dataPath_{Scope::Absolute, {{module_{"example"}, list_{"list"}}}};
            }

            SECTION("arg: /example:a")
            {
                lsArg = dataPath_{Scope::Absolute, {{module_{"example"}, container_{"a"}}}};
                expectedPath = dataPath_{Scope::Absolute, {{module_{"example"}, container_{"a"}}}};
            }

            SECTION("arg: /example:list")
            {
                lsArg = dataPath_{Scope::Absolute, {{module_{"example"}, list_{"list"}}}};
                expectedPath = dataPath_{Scope::Absolute, {{module_{"example"}, list_{"list"}}}};
            }

            SECTION("arg example:*")
            {
                lsArg = module_{"example"};
                expectedPath = module_{"example"};
            }
        }

        SECTION("cwd: /example:a")
        {
            parser.changeNode({Scope::Relative, {{module_{"example"}, container_{"a"}}}});

            SECTION("arg: <none>")
            {
                expectedPath = dataPath_{Scope::Absolute, {{module_{"example"}, container_{"a"}}}};
            }

            SECTION("arg: example:a2")
            {
                lsArg = dataPath_{Scope::Relative, {{container_{"a2"}}}};
                expectedPath = dataPath_{Scope::Absolute, {{module_{"example"}, container_{"a"}}, {container_{"a2"}}}};
            }

            SECTION("arg: example:listInCont")
            {
                lsArg = dataPath_{Scope::Relative, {{list_{"listInCont"}}}};
                expectedPath = dataPath_{Scope::Absolute, {{module_{"example"}, container_{"a"}}, {list_{"listInCont"}}}};
            }

            SECTION("arg: /example:a")
            {
                lsArg = dataPath_{Scope::Absolute, {{module_{"example"}, container_{"a"}}}};
                expectedPath = dataPath_{Scope::Absolute, {{module_{"example"}, container_{"a"}}}};
            }

            SECTION("arg: /example:list")
            {
                lsArg = dataPath_{Scope::Absolute, {{module_{"example"}, list_{"list"}}}};
                expectedPath = dataPath_{Scope::Absolute, {{module_{"example"}, list_{"list"}}}};
            }
        }
        SECTION("cwd: /example:list")
        {
            parser.changeNode({Scope::Relative, {{module_{"example"}, list_{"list"}}}});

            SECTION("arg: <none>")
            {
                expectedPath = dataPath_{Scope::Absolute, {{module_{"example"}, list_{"list"}}}};
            }

            SECTION("arg: example:contInList")
            {
                lsArg = schemaPath_{Scope::Relative, {{container_{"contInList"}}}};
                expectedPath = schemaPath_{Scope::Absolute, {{module_{"example"}, list_{"list"}}, {container_{"contInList"}}}};
            }

            SECTION("arg: /example:a")
            {
                lsArg = dataPath_{Scope::Absolute, {{module_{"example"}, container_{"a"}}}};
                expectedPath = dataPath_{Scope::Absolute, {{module_{"example"}, container_{"a"}}}};
            }

            SECTION("arg: /example:list")
            {
                lsArg = dataPath_{Scope::Absolute, {{module_{"example"}, list_{"list"}}}};
                expectedPath = dataPath_{Scope::Absolute, {{module_{"example"}, list_{"list"}}}};
            }

            SECTION("arg example:*")
            {
                lsArg = module_{"example"};
                expectedPath = module_{"example"};
            }
        }
        ls_ ls;
        ls.m_path = lsArg;
        expectations.push_back(NAMED_REQUIRE_CALL(datastore, schema()).RETURN(schema));
        expectations.push_back(NAMED_REQUIRE_CALL(*schema, availableNodes(expectedPath, Recursion::NonRecursive)).RETURN(std::set<ModuleNodePair>{}));
        toInterpret.push_back(ls);
    }

    SECTION("get")
    {
        using namespace std::string_literals;
        DatastoreAccess::Tree treeReturned;
        decltype(get_::m_path) inputPath;
        std::string expectedPathArg;

        SECTION("paths")
        {
            SECTION("/")
            {
                expectedPathArg = "/";
            }

            SECTION("module")
            {
                inputPath = module_{"mod"};
                expectedPathArg = "/mod:*";
            }

            SECTION("path to a leaf")
            {
                expectedPathArg = "/mod:myLeaf";
                Scope scope;
                SECTION("cwd: /")
                {
                    SECTION("absolute")
                    {
                        scope = Scope::Absolute;
                    }

                    SECTION("relative")
                    {
                        scope = Scope::Relative;
                    }

                    inputPath = dataPath_{scope, {dataNode_{{"mod"}, leaf_{"myLeaf"}}}};
                }

                SECTION("cwd: /mod:whatever")
                {
                    parser.changeNode(dataPath_{Scope::Relative, {dataNode_{{"mod"}, container_{"whatever"}}}});
                    SECTION("absolute")
                    {
                        scope = Scope::Absolute;
                        inputPath = dataPath_{scope, {dataNode_{{"mod"}, leaf_{"myLeaf"}}}};
                    }

                    SECTION("relative")
                    {
                        scope = Scope::Relative;
                        inputPath = dataPath_{scope, {dataNode_{nodeup_{}}, dataNode_{{"mod"}, leaf_{"myLeaf"}}}};
                    }

                }
            }

            SECTION("path to a list")
            {
                expectedPathArg = "/mod:myList[name='AHOJ']";
                Scope scope;
                SECTION("cwd: /")
                {
                    SECTION("absolute")
                    {
                        scope = Scope::Absolute;
                    }

                    SECTION("relative")
                    {
                        scope = Scope::Relative;
                    }

                    inputPath = dataPath_{scope, {dataNode_{{"mod"}, listElement_{"myList", {{"name", "AHOJ"s}}}}}};
                }

                SECTION("cwd: /mod:whatever")
                {
                    parser.changeNode(dataPath_{Scope::Relative, {dataNode_{{"mod"}, container_{"whatever"}}}});
                    SECTION("absolute")
                    {
                        scope = Scope::Absolute;
                        inputPath = dataPath_{scope, {dataNode_{{"mod"}, listElement_{"myList", {{"name", "AHOJ"s}}}}}};
                    }

                    SECTION("relative")
                    {
                        scope = Scope::Relative;
                        inputPath = dataPath_{scope, {dataNode_{nodeup_{}}, dataNode_{{"mod"}, listElement_{"myList", {{"name", "AHOJ"s}}}}}};
                    }
                }
            }
        }

        SECTION("trees")
        {
            expectedPathArg = "/";
            SECTION("no leaflists")
            {
                treeReturned = {
                    {"/mod:AHOJ", 30},
                    {"/mod:CAU", std::string{"AYYY"}},
                    {"/mod:CUS", bool{true}}
                };
            }

            SECTION("leaflist at the beginning of a tree")
            {
                treeReturned = {
                    {"/mod:addresses", special_{SpecialValue::LeafList}},
                    {"/mod:addresses[.='0.0.0.0']", std::string{"0.0.0.0"}},
                    {"/mod:addresses[.='127.0.0.1']", std::string{"127.0.0.1"}},
                    {"/mod:addresses[.='192.168.0.1']", std::string{"192.168.0.1"}},
                    {"/mod:AHOJ", 30},
                    {"/mod:CAU", std::string{"AYYY"}},
                };
            }

            SECTION("leaflist in the middle of a tree")
            {
                treeReturned = {
                    {"/mod:AHOJ", 30},
                    {"/mod:addresses", special_{SpecialValue::LeafList}},
                    {"/mod:addresses[.='0.0.0.0']", std::string{"0.0.0.0"}},
                    {"/mod:addresses[.='127.0.0.1']", std::string{"127.0.0.1"}},
                    {"/mod:addresses[.='192.168.0.1']", std::string{"192.168.0.1"}},
                    {"/mod:CAU", std::string{"AYYY"}},
                };
            }

            SECTION("leaflist at the end of a tree")
            {
                treeReturned = {
                    {"/mod:AHOJ", 30},
                    {"/mod:CAU", std::string{"AYYY"}},
                    {"/mod:addresses", special_{SpecialValue::LeafList}},
                    {"/mod:addresses[.='0.0.0.0']", std::string{"0.0.0.0"}},
                    {"/mod:addresses[.='127.0.0.1']", std::string{"127.0.0.1"}},
                    {"/mod:addresses[.='192.168.0.1']", std::string{"192.168.0.1"}},
                };
            }
        }

        get_ getCmd;
        getCmd.m_path = inputPath;
        expectations.push_back(NAMED_REQUIRE_CALL(datastore, getItems(expectedPathArg)).RETURN(treeReturned));
        toInterpret.push_back(getCmd);
    }

    SECTION("create/delete")
    {
        using namespace std::string_literals;
        dataPath_ inputPath;

        SECTION("list instance")
        {
            inputPath.m_nodes = {dataNode_{{"mod"}, listElement_{"department", {{"name", "engineering"s}}}}};
            expectations.push_back(NAMED_REQUIRE_CALL(datastore, createListInstance("/mod:department[name='engineering']")));
            expectations.push_back(NAMED_REQUIRE_CALL(datastore, deleteListInstance("/mod:department[name='engineering']")));
        }

        SECTION("leaflist instance")
        {
            inputPath.m_nodes = {dataNode_{{"mod"}, leafListElement_{"addresses", "127.0.0.1"s}}};
            expectations.push_back(NAMED_REQUIRE_CALL(datastore, createLeafListInstance("/mod:addresses[.='127.0.0.1']")));
            expectations.push_back(NAMED_REQUIRE_CALL(datastore, deleteLeafListInstance("/mod:addresses[.='127.0.0.1']")));
        }

        SECTION("presence container")
        {
            inputPath.m_nodes = {dataNode_{{"mod"}, container_{"pContainer"}}};
            expectations.push_back(NAMED_REQUIRE_CALL(datastore, createPresenceContainer("/mod:pContainer")));
            expectations.push_back(NAMED_REQUIRE_CALL(datastore, deletePresenceContainer("/mod:pContainer")));
        }

        create_ createCmd;
        createCmd.m_path = inputPath;
        delete_ deleteCmd;
        deleteCmd.m_path = inputPath;
        toInterpret.push_back(createCmd);
        toInterpret.push_back(deleteCmd);
    }

    SECTION("commit")
    {
        expectations.push_back(NAMED_REQUIRE_CALL(datastore, commitChanges()));
        toInterpret.push_back(commit_{});
    }

    SECTION("discard")
    {
        expectations.push_back(NAMED_REQUIRE_CALL(datastore, discardChanges()));
        toInterpret.push_back(discard_{});
    }


    SECTION("set")
    {
        dataPath_ inputPath;
        leaf_data_ inputData;

        SECTION("setting identityRef without module") // The parser has to fill in the module
        {
            inputPath.m_nodes = {dataNode_{{"mod"}, leaf_{"animal"}}};
            inputData = identityRef_{"Doge"};
            expectations.push_back(NAMED_REQUIRE_CALL(datastore, setLeaf("/mod:animal", identityRef_{"mod", "Doge"})));
        }


        set_ setCmd;
        setCmd.m_path = inputPath;
        setCmd.m_data = inputData;
        toInterpret.push_back(setCmd);
    }


    SECTION("copy")
    {
        SECTION("running -> startup")
        {
            expectations.push_back(NAMED_REQUIRE_CALL(datastore, copyConfig(Datastore::Running, Datastore::Startup)));
            toInterpret.push_back(copy_{{}, Datastore::Running, Datastore::Startup});
        }

        SECTION("startup -> running")
        {
            expectations.push_back(NAMED_REQUIRE_CALL(datastore, copyConfig(Datastore::Startup, Datastore::Running)));
            toInterpret.push_back(copy_{{}, Datastore::Startup, Datastore::Running});
        }
    }

    for (const auto& command : toInterpret) {
        boost::apply_visitor(Interpreter(parser, datastore), command);
    }
}
