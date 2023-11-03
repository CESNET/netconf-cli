/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/

#include "trompeloeil_doctest.hpp"
#include <experimental/iterator>
#include "ast_commands.hpp"
#include "datastoreaccess_mock.hpp"
#include "interpreter.hpp"
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
    IMPLEMENT_CONST_MOCK1(hasInputNodes);
};

TEST_CASE("interpreter tests")
{
    auto schema = std::make_shared<MockSchema>();
    Parser parser(schema);
    auto datastore = std::make_shared<MockDatastoreAccess>();
    auto input_datastore = std::make_shared<MockDatastoreAccess>();
    auto createTemporaryDatastore = [input_datastore]([[maybe_unused]] const std::shared_ptr<DatastoreAccess>& datastore) {
        return input_datastore;
    };
    ProxyDatastore proxyDatastore(datastore, createTemporaryDatastore);
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
        expectations.emplace_back(NAMED_REQUIRE_CALL(*datastore, schema()).RETURN(schema));
        expectations.emplace_back(NAMED_REQUIRE_CALL(*schema, availableNodes(expectedPath, Recursion::NonRecursive)).RETURN(std::set<ModuleNodePair>{}));
        toInterpret.emplace_back(ls);
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

                    inputPath = dataPath_{scope, {dataNode_{module_{"mod"}, leaf_{"myLeaf"}}}};
                }

                SECTION("cwd: /mod:whatever")
                {
                    parser.changeNode(dataPath_{Scope::Relative, {dataNode_{module_{"mod"}, container_{"whatever"}}}});
                    SECTION("absolute")
                    {
                        scope = Scope::Absolute;
                        inputPath = dataPath_{scope, {dataNode_{module_{"mod"}, leaf_{"myLeaf"}}}};
                    }

                    SECTION("relative")
                    {
                        scope = Scope::Relative;
                        inputPath = dataPath_{scope, {dataNode_{nodeup_{}}, dataNode_{module_{"mod"}, leaf_{"myLeaf"}}}};
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

                    inputPath = dataPath_{scope, {dataNode_{module_{"mod"}, listElement_{"myList", {{"name", "AHOJ"s}}}}}};
                }

                SECTION("cwd: /mod:whatever")
                {
                    parser.changeNode(dataPath_{Scope::Relative, {dataNode_{module_{"mod"}, container_{"whatever"}}}});
                    SECTION("absolute")
                    {
                        scope = Scope::Absolute;
                        inputPath = dataPath_{scope, {dataNode_{module_{"mod"}, listElement_{"myList", {{"name", "AHOJ"s}}}}}};
                    }

                    SECTION("relative")
                    {
                        scope = Scope::Relative;
                        inputPath = dataPath_{scope, {dataNode_{nodeup_{}}, dataNode_{module_{"mod"}, listElement_{"myList", {{"name", "AHOJ"s}}}}}};
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
        expectations.emplace_back(NAMED_REQUIRE_CALL(*datastore, getItems(expectedPathArg)).RETURN(treeReturned));
        toInterpret.emplace_back(getCmd);
    }

    SECTION("create/delete")
    {
        using namespace std::string_literals;
        dataPath_ inputPath;

        SECTION("list instance")
        {
            inputPath.m_nodes = {dataNode_{module_{"mod"}, listElement_{"department", {{"name", "engineering"s}}}}};
            expectations.emplace_back(NAMED_REQUIRE_CALL(*datastore, createItem("/mod:department[name='engineering']")));
            expectations.emplace_back(NAMED_REQUIRE_CALL(*datastore, deleteItem("/mod:department[name='engineering']")));
        }

        SECTION("leaflist instance")
        {
            inputPath.m_nodes = {dataNode_{module_{"mod"}, leafListElement_{"addresses", "127.0.0.1"s}}};
            expectations.emplace_back(NAMED_REQUIRE_CALL(*datastore, createItem("/mod:addresses[.='127.0.0.1']")));
            expectations.emplace_back(NAMED_REQUIRE_CALL(*datastore, deleteItem("/mod:addresses[.='127.0.0.1']")));
        }

        SECTION("presence container")
        {
            inputPath.m_nodes = {dataNode_{module_{"mod"}, container_{"pContainer"}}};
            expectations.emplace_back(NAMED_REQUIRE_CALL(*datastore, createItem("/mod:pContainer")));
            expectations.emplace_back(NAMED_REQUIRE_CALL(*datastore, deleteItem("/mod:pContainer")));
        }

        create_ createCmd;
        createCmd.m_path = inputPath;
        delete_ deleteCmd;
        deleteCmd.m_path = inputPath;
        toInterpret.emplace_back(createCmd);
        toInterpret.emplace_back(deleteCmd);
    }

    SECTION("delete a leaf")
    {
        expectations.emplace_back(NAMED_REQUIRE_CALL(*datastore, deleteItem("/mod:someLeaf")));
        delete_ deleteCmd;
        deleteCmd.m_path = {Scope::Absolute, {dataNode_{module_{"mod"}, leaf_{"someLeaf"}}, }};
        toInterpret.emplace_back(deleteCmd);
    }

    SECTION("commit")
    {
        expectations.emplace_back(NAMED_REQUIRE_CALL(*datastore, commitChanges()));
        toInterpret.emplace_back(commit_{});
    }

    SECTION("discard")
    {
        expectations.emplace_back(NAMED_REQUIRE_CALL(*datastore, discardChanges()));
        toInterpret.emplace_back(discard_{});
    }


    SECTION("set")
    {
        dataPath_ inputPath;
        leaf_data_ inputData;

        SECTION("setting identityRef without module") // The parser has to fill in the module
        {
            inputPath.m_nodes = {dataNode_{module_{"mod"}, leaf_{"animal"}}};
            inputData = identityRef_{"Doge"};
            expectations.emplace_back(NAMED_REQUIRE_CALL(*datastore, setLeaf("/mod:animal", identityRef_{"mod", "Doge"})));
        }


        set_ setCmd;
        setCmd.m_path = inputPath;
        setCmd.m_data = inputData;
        toInterpret.emplace_back(setCmd);
    }


    SECTION("copy")
    {
        SECTION("running -> startup")
        {
            expectations.emplace_back(NAMED_REQUIRE_CALL(*datastore, copyConfig(Datastore::Running, Datastore::Startup)));
            toInterpret.emplace_back(copy_{{}, Datastore::Running, Datastore::Startup});
        }

        SECTION("startup -> running")
        {
            expectations.emplace_back(NAMED_REQUIRE_CALL(*datastore, copyConfig(Datastore::Startup, Datastore::Running)));
            toInterpret.emplace_back(copy_{{}, Datastore::Startup, Datastore::Running});
        }
    }

    for (const auto& command : toInterpret) {
        boost::apply_visitor(Interpreter(parser, proxyDatastore), command);
    }
}

TEST_CASE("rpc")
{
    auto schema = std::make_shared<MockSchema>();
    Parser parser(schema);
    auto datastore = std::make_shared<MockDatastoreAccess>();
    auto input_datastore = std::make_shared<MockDatastoreAccess>();
    auto createTemporaryDatastore = [input_datastore]([[maybe_unused]] const std::shared_ptr<DatastoreAccess>& datastore) {
        return input_datastore;
    };
    ProxyDatastore proxyDatastore(datastore, createTemporaryDatastore);

    SECTION("entering/leaving rpc context")
    {
        dataPath_ rpcPath;
        rpcPath.pushFragment({module_{"example"}, rpcNode_{"launch-nukes"}});
        prepare_ prepareCmd;
        prepareCmd.m_path = rpcPath;

        {
            REQUIRE_CALL(*input_datastore, createItem("/example:launch-nukes"));
            boost::apply_visitor(Interpreter(parser, proxyDatastore), command_{prepareCmd});
        }

        REQUIRE(parser.currentPath() == rpcPath);

        dataPath_ inRpcContainer = dataPath_{Scope::Relative, {dataNode_{container_{"payload"}}}};
        dataPath_ inRpcLeaf = dataPath_{Scope::Relative, {dataNode_{leaf_{"description"}}}};
        dataPath_ outRpcPath = dataPath_{Scope::Absolute, {dataNode_{nodeup_{}}, dataNode_{module_("example"), container_{"somewhere-else"}}}};

        std::string inRpcContainerString = "/example:launch-nukes/payload";
        std::string inRpcLeafString = "/example:launch-nukes/description";

        SECTION("valid commands")
        {
            command_ command;
            std::vector<std::unique_ptr<trompeloeil::expectation>> expectations;

            SECTION("create inside context")
            {
                expectations.emplace_back(NAMED_REQUIRE_CALL(*input_datastore, createItem(inRpcContainerString)));
                command = create_{{}, inRpcContainer};
            }

            SECTION("set inside context")
            {
                leaf_data_ leafData = std::string{"Nuke the whales"};
                expectations.emplace_back(NAMED_REQUIRE_CALL(*input_datastore, setLeaf(inRpcLeafString, leafData)));
                command = set_{{}, inRpcLeaf, leafData};
            }

            SECTION("delete inside context")
            {
                expectations.emplace_back(NAMED_REQUIRE_CALL(*input_datastore, deleteItem(inRpcContainerString)));
                command = delete_{{}, inRpcContainer};
            }

            SECTION("cd inside context")
            {
                command = cd_{{}, inRpcContainer};
            }

            SECTION("move inside context")
            {
                expectations.emplace_back(NAMED_REQUIRE_CALL(*datastore, moveItem("description", yang::move::Absolute::Begin)));
                command = move_{{}, inRpcLeaf, yang::move::Absolute::Begin};
            }

            SECTION("cancel")
            {
                command = cancel_{};
            }

            SECTION("exec")
            {
                expectations.emplace_back(NAMED_REQUIRE_CALL(*input_datastore, getItems("/")).RETURN(DatastoreAccess::Tree{}));
                expectations.emplace_back(NAMED_REQUIRE_CALL(*datastore, execute("/example:launch-nukes", DatastoreAccess::Tree{})).RETURN(DatastoreAccess::Tree{}));
                command = exec_{};
            }

            boost::apply_visitor(Interpreter(parser, proxyDatastore), command);

            boost::apply_visitor([&](auto& cmd) {
                if constexpr (std::is_same_v<std::decay_t<decltype(cmd)>, exec_>) {
                    REQUIRE(parser.currentPath() == dataPath_{});
                } else if constexpr (std::is_same_v<std::decay_t<decltype(cmd)>, cancel_>) {
                    REQUIRE(parser.currentPath() == dataPath_{});
                }
            }, command);

        }

        SECTION("invalid commands")
        {
            command_ command;

            SECTION("block commit entirely")
            {
                command = commit_{};
            }

            SECTION("block discard entirely")
            {
                command = discard_{};
            }

            SECTION("block copy entirely")
            {
                command = copy_{{}, Datastore::Running, Datastore::Startup};
            }

            SECTION("block switch entirely")
            {
                command = switch_{{}, DatastoreTarget::Running};
            }

            SECTION("block prepare entirely")
            {
                command = prepare_{{}, outRpcPath};
            }

            SECTION("block create outside RPC context")
            {
                command = create_{{}, outRpcPath};
            }

            SECTION("block set outside RPC context")
            {
                command = set_{{}, outRpcPath, std::string{"Nuke the whales"}};
            }

            SECTION("block delete outside RPC context")
            {
                command = delete_{{}, outRpcPath};
            }

            SECTION("block cd outside RPC context")
            {
                command = cd_{{}, outRpcPath};
            }

            SECTION("block move outside RPC context")
            {
                command = move_{{}, outRpcPath, yang::move::Absolute::Begin};
            }

            REQUIRE_THROWS(boost::apply_visitor(Interpreter(parser, proxyDatastore), command));
        }
    }
}
