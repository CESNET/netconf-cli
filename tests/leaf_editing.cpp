/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/

#include "trompeloeil_doctest.hpp"
#include <boost/core/demangle.hpp>
#include "ast_commands.hpp"
#include "leaf_data_helpers.hpp"
#include "parser.hpp"
#include "pretty_printers.hpp"
#include "static_schema.hpp"
#include "utils.hpp"

TEST_CASE("leaf editing")
{
    auto schema = std::make_shared<StaticSchema>();
    schema->addModule("mod");
    schema->addModule("pizza-module");
    schema->addContainer("/", "mod:contA");
    schema->addLeaf("/", "mod:leafString", yang::String{});
    schema->addLeaf("/", "mod:leafDecimal", yang::Decimal{});
    schema->addLeaf("/", "mod:leafBool", yang::Bool{});
    schema->addLeaf("/", "mod:leafInt8", yang::Int8{});
    schema->addLeaf("/", "mod:leafInt16", yang::Int16{});
    schema->addLeaf("/", "mod:leafInt32", yang::Int32{});
    schema->addLeaf("/", "mod:leafInt64", yang::Int64{});
    schema->addLeaf("/", "mod:leafUint8", yang::Uint8{});
    schema->addLeaf("/", "mod:leafUint16", yang::Uint16{});
    schema->addLeaf("/", "mod:leafUint32", yang::Uint32{});
    schema->addLeaf("/", "mod:leafUint64", yang::Uint64{});
    schema->addLeaf("/", "mod:leafBinary", yang::Binary{});
    schema->addIdentity(std::nullopt, identityRef_{"mod", "food"});
    schema->addIdentity(std::nullopt, identityRef_{"mod", "vehicle"});
    schema->addIdentity(identityRef_{"mod", "food"}, identityRef_{"mod", "pizza"});
    schema->addIdentity(identityRef_{"mod", "food"}, identityRef_{"mod", "spaghetti"});
    schema->addIdentity(identityRef_{"mod", "pizza"}, identityRef_{"pizza-module", "hawaii"});
    schema->addLeaf("/", "mod:foodIdentRef", yang::IdentityRef{schema->validIdentities("mod", "food")});
    schema->addLeaf("/", "mod:pizzaIdentRef", yang::IdentityRef{schema->validIdentities("mod", "pizza")});
    schema->addLeaf("/mod:contA", "mod:identInCont", yang::IdentityRef{schema->validIdentities("mod", "pizza")});
    schema->addLeaf("/", "mod:leafEnum", createEnum({"lol", "data", "coze"}));
    schema->addLeaf("/mod:contA", "mod:leafInCont", yang::String{});
    schema->addList("/", "mod:list", {"number"});
    schema->addLeaf("/mod:list", "mod:number", yang::Int32{});
    schema->addLeaf("/mod:list", "mod:leafInList", yang::String{});
    schema->addLeaf("/", "mod:refToString", yang::LeafRef{"/mod:leafString", std::make_unique<yang::TypeInfo>(schema->leafType("/mod:leafString"))});
    schema->addLeaf("/", "mod:refToInt8", yang::LeafRef{"/mod:leafInt8", std::make_unique<yang::TypeInfo>(schema->leafType("/mod:leafInt8"))});
    schema->addLeaf("/", "mod:refToLeafInCont", yang::LeafRef{"/mod:contA/identInCont", std::make_unique<yang::TypeInfo>(schema->leafType("/mod:contA/mod:identInCont"))});
    schema->addLeaf("/", "mod:intOrString", yang::Union{{yang::TypeInfo{yang::Int32{}}, yang::TypeInfo{yang::String{}}}});
    schema->addLeaf("/", "mod:twoInts", yang::Union{{yang::TypeInfo{yang::Uint8{}}, yang::TypeInfo{yang::Int16{}}}});
    schema->addLeaf("/", "mod:unionStringEnumLeafref", yang::Union{{
            yang::LeafDataType{yang::String{}},
        yang::LeafDataType{createEnum({"foo", "bar"})},
        yang::LeafDataType{yang::LeafRef{"/mod:leafEnum", std::make_unique<yang::TypeInfo>(schema->leafType("/mod:leafEnum"))}}
    }});

    schema->addList("/", "mod:portSettings", {"port"});
    schema->addLeaf("/mod:portSettings", "mod:port", createEnum({"eth0", "eth1", "eth2"}));
    schema->addList("/", "mod:portMapping", {"port"});
    schema->addLeaf("/mod:portMapping", "mod:port", createEnum({"utf1", "utf2", "utf3"}));
    schema->addLeaf("/", "mod:activeMappedPort", yang::LeafRef{"/mod:portMapping/mod:port", std::make_unique<yang::TypeInfo>(schema->leafType("/mod:portMapping/mod:port"))});
    schema->addLeaf("/", "mod:activePort", yang::Union{{
        yang::TypeInfo{createEnum({"wlan0", "wlan1"})},
        yang::TypeInfo{yang::LeafRef{"/mod:portSettings/mod:port", std::make_unique<yang::TypeInfo>(schema->leafType("/mod:portSettings/mod:port"))}},
        yang::TypeInfo{yang::LeafRef{"/mod:activeMappedPort", std::make_unique<yang::TypeInfo>(schema->leafType("/mod:activeMappedPort"))}},
        yang::TypeInfo{yang::Empty{}},
    }});
    schema->addLeaf("/", "mod:dummy", yang::Empty{});
    schema->addLeaf("/", "mod:readonly", yang::Int32{}, yang::AccessType::ReadOnly);

    schema->addLeaf("/", "mod:flags", yang::Bits{{"carry", "sign"}});
    schema->addLeaf("/", "mod:iid", yang::InstanceIdentifier{});
    schema->addLeaf("/mod:contA", "mod:x", yang::String{});
    schema->addLeaf("/mod:contA", "mod:iid", yang::InstanceIdentifier{});

    Parser parser(schema);
    std::string input;
    std::ostringstream errorStream;

    SECTION("valid input")
    {
        set_ expected;
        dataPath_ cwd;

        SECTION("set mod:leafString \"some_data\"")
        {
            input = "set mod:leafString \'some_data\'";
            expected.m_path.m_nodes.emplace_back(module_{"mod"}, leaf_("leafString"));
            expected.m_data = std::string("some_data");
        }

        SECTION("cd mod:contA; set leafInCont 'x'")
        {
            input = "set leafInCont 'x'";
            cwd.m_nodes.emplace_back(module_{"mod"}, container_{"contA"});
            expected.m_path.m_nodes.emplace_back(leaf_("leafInCont"));
            expected.m_data = std::string("x");
        }

        SECTION("set mod:contA/leafInCont 'more_data'")
        {
            input = "set mod:contA/leafInCont 'more_data'";
            expected.m_path.m_nodes.emplace_back(module_{"mod"}, container_("contA"));
            expected.m_path.m_nodes.emplace_back(leaf_("leafInCont"));
            expected.m_data = std::string("more_data");
        }

        SECTION("set mod:contA/leafInCont \"data with' a quote\"")
        {
            input = "set mod:contA/leafInCont \"data with' a quote\"";
            expected.m_path.m_nodes.emplace_back(module_{"mod"}, container_("contA"));
            expected.m_path.m_nodes.emplace_back(leaf_("leafInCont"));
            expected.m_data = std::string("data with' a quote");
        }

        SECTION("set mod:contA/leafInCont 'data with\" a quote'")
        {
            input = "set mod:contA/leafInCont 'data with\" a quote'";
            expected.m_path.m_nodes.emplace_back(module_{"mod"}, container_("contA"));
            expected.m_path.m_nodes.emplace_back(leaf_("leafInCont"));
            expected.m_data = std::string("data with\" a quote");
        }

        SECTION("set mod:contA/leafInCont   'more   d\tata'") // spaces in string
        {
            input = "set mod:contA/leafInCont 'more   d\tata'";
            expected.m_path.m_nodes.emplace_back(module_{"mod"}, container_("contA"));
            expected.m_path.m_nodes.emplace_back(leaf_("leafInCont"));
            expected.m_data = std::string("more   d\tata");
        }

        SECTION("set mod:list[number=1]/leafInList \"another_data\"")
        {
            input = "set mod:list[number=1]/leafInList \"another_data\"";
            auto keys = ListInstance{
                {"number", int32_t{1}}};
            expected.m_path.m_nodes.emplace_back(module_{"mod"}, listElement_("list", keys));
            expected.m_path.m_nodes.emplace_back(leaf_("leafInList"));
            expected.m_data = std::string("another_data");
        }

        SECTION("data types")
        {
            SECTION("string")
            {
                input = "set mod:leafString \"somedata\"";
                expected.m_path.m_nodes.emplace_back(module_{"mod"}, leaf_("leafString"));
                expected.m_data = std::string("somedata");
            }

            SECTION("more spaces before value")
            {
                input = "set mod:leafString   \"somedata\"";
                expected.m_path.m_nodes.emplace_back(module_{"mod"}, leaf_("leafString"));
                expected.m_data = std::string("somedata");
            }

            SECTION("int8")
            {
                input = "set mod:leafInt8 2";
                expected.m_path.m_nodes.emplace_back(module_{"mod"}, leaf_("leafInt8"));
                expected.m_data = int8_t{2};
            }

            SECTION("negative int8")
            {
                input = "set mod:leafInt8 -10";
                expected.m_path.m_nodes.emplace_back(module_{"mod"}, leaf_("leafInt8"));
                expected.m_data = int8_t{-10};
            }

            SECTION("uint8")
            {
                input = "set mod:leafUint8 2";
                expected.m_path.m_nodes.emplace_back(module_{"mod"}, leaf_("leafUint8"));
                expected.m_data = uint8_t{2};
            }

            SECTION("int16")
            {
                input = "set mod:leafInt16 30000";
                expected.m_path.m_nodes.emplace_back(module_{"mod"}, leaf_("leafInt16"));
                expected.m_data = int16_t{30'000};
            }

            SECTION("uint16")
            {
                input = "set mod:leafUint16 30000";
                expected.m_path.m_nodes.emplace_back(module_{"mod"}, leaf_("leafUint16"));
                expected.m_data = uint16_t{30'000};
            }

            SECTION("int32")
            {
                input = "set mod:leafInt32 30000";
                expected.m_path.m_nodes.emplace_back(module_{"mod"}, leaf_("leafInt32"));
                expected.m_data = int32_t{30'000};
            }

            SECTION("uint32")
            {
                input = "set mod:leafUint32 30000";
                expected.m_path.m_nodes.emplace_back(module_{"mod"}, leaf_("leafUint32"));
                expected.m_data = uint32_t{30'000};
            }

            SECTION("int32")
            {
                input = "set mod:leafInt32 30000";
                expected.m_path.m_nodes.emplace_back(module_{"mod"}, leaf_("leafInt32"));
                expected.m_data = int32_t{30'000};
            }

            SECTION("uint64")
            {
                input = "set mod:leafUint64 30000";
                expected.m_path.m_nodes.emplace_back(module_{"mod"}, leaf_("leafUint64"));
                expected.m_data = uint64_t{30'000};
            }

            SECTION("decimal")
            {
                input = "set mod:leafDecimal 3.14159";
                expected.m_path.m_nodes.emplace_back(module_{"mod"}, leaf_("leafDecimal"));
                expected.m_data = 3.14159;
            }

            SECTION("enum")
            {
                input = "set mod:leafEnum coze";
                expected.m_path.m_nodes.emplace_back(module_{"mod"}, leaf_("leafEnum"));
                expected.m_data = enum_("coze");
            }

            SECTION("bool")
            {
                input = "set mod:leafBool true";
                expected.m_path.m_nodes.emplace_back(module_{"mod"}, leaf_("leafBool"));
                expected.m_data = true;
            }

            SECTION("union")
            {
                SECTION("int")
                {
                    expected.m_path.m_nodes.emplace_back(module_{"mod"}, leaf_("intOrString"));
                    input = "set mod:intOrString 90";
                    expected.m_data = int32_t{90};
                }
                SECTION("string")
                {
                    expected.m_path.m_nodes.emplace_back(module_{"mod"}, leaf_("intOrString"));
                    input = "set mod:intOrString \"test\"";
                    expected.m_data = std::string{"test"};
                }

                SECTION("union with two integral types")
                {
                    expected.m_path.m_nodes.emplace_back(module_{"mod"}, leaf_("twoInts"));
                    SECTION("uint8")
                    {
                        input = "set mod:twoInts 100";
                        expected.m_data = uint8_t{100};
                    }
                    SECTION("int16")
                    {
                        input = "set mod:twoInts 6666";
                        expected.m_data = int16_t{6666};
                    }
                }

                SECTION("union with enum and leafref to enum")
                {
                    expected.m_path.m_nodes.emplace_back(module_{"mod"}, leaf_("unionStringEnumLeafref"));
                    SECTION("string")
                    {
                        input = "set mod:unionStringEnumLeafref \"AHOJ\"";
                        expected.m_data = std::string{"AHOJ"};
                    }
                    SECTION("enum")
                    {
                        input = "set mod:unionStringEnumLeafref bar";
                        expected.m_data = enum_("bar");
                    }
                    SECTION("enum leafref")
                    {
                        input = "set mod:unionStringEnumLeafref coze";
                        expected.m_data = enum_("coze");
                    }
                }

                SECTION("activePort")
                {
                    expected.m_path.m_nodes.emplace_back(module_{"mod"}, leaf_("activePort"));
                    input = "set mod:activePort ";
                    SECTION("1. anonymous enum")
                    {
                        SECTION("wlan0")
                        {
                            input += "wlan0";
                            expected.m_data = enum_("wlan0");
                        }
                        SECTION("wlan1")
                        {
                            input += "wlan1";
                            expected.m_data = enum_("wlan1");
                        }
                    }
                    SECTION("2. leafref to enum")
                    {
                        SECTION("eth0")
                        {
                            input += "eth0";
                            expected.m_data = enum_("eth0");
                        }
                        SECTION("eth1")
                        {
                            input += "eth1";
                            expected.m_data = enum_("eth1");
                        }
                        SECTION("eth2")
                        {
                            input += "eth2";
                            expected.m_data = enum_("eth2");
                        }
                    }
                    SECTION("3. leafref to leafref")
                    {
                        SECTION("utf1")
                        {
                            input += "utf1";
                            expected.m_data = enum_("utf1");
                        }
                        SECTION("utf2")
                        {
                            input += "utf2";
                            expected.m_data = enum_("utf2");
                        }
                        SECTION("utf3")
                        {
                            input += "utf3";
                            expected.m_data = enum_("utf3");
                        }
                    }
                    SECTION("4. empty")
                    {
                        expected.m_data = empty_{};
                    }
                }
            }

            SECTION("binary")
            {
                SECTION("zero ending '='")
                {
                    input = "set mod:leafBinary This/IsABase64EncodedSomething++/342431++";
                    expected.m_data = binary_{"This/IsABase64EncodedSomething++/342431++"};
                }

                SECTION("one ending '='")
                {
                    input = "set mod:leafBinary This/IsABase64EncodedSomething++/342431++=";
                    expected.m_data = binary_{"This/IsABase64EncodedSomething++/342431++="};
                }

                SECTION("two ending '='")
                {
                    input = "set mod:leafBinary This/IsABase64EncodedSomething++/342431++==";
                    expected.m_data = binary_{"This/IsABase64EncodedSomething++/342431++=="};
                }
                expected.m_path.m_nodes.emplace_back(module_{"mod"}, leaf_("leafBinary"));
            }

            SECTION("identityRef")
            {
                SECTION("foodIdentRef")
                {
                    input = "set mod:foodIdentRef ";
                    expected.m_path.m_nodes.emplace_back(module_{"mod"}, leaf_("foodIdentRef"));

                    SECTION("food")
                    {
                        input += "food";
                        expected.m_data = identityRef_("food");
                    }
                    SECTION("mod:food")
                    {
                        input += "mod:food";
                        expected.m_data = identityRef_("mod", "food");
                    }
                    SECTION("pizza")
                    {
                        input += "pizza";
                        expected.m_data = identityRef_("pizza");
                    }
                    SECTION("mod:pizza")
                    {
                        input += "mod:pizza";
                        expected.m_data = identityRef_("mod", "pizza");
                    }
                    SECTION("pizza-module:hawaii")
                    {
                        input += "pizza-module:hawaii";
                        expected.m_data = identityRef_("pizza-module", "hawaii");
                    }
                }
                SECTION("pizzaIdentRef")
                {
                    input = "set mod:pizzaIdentRef ";
                    expected.m_path.m_nodes.emplace_back(module_{"mod"}, leaf_("pizzaIdentRef"));
                    SECTION("pizza")
                    {
                        input += "pizza";
                        expected.m_data = identityRef_("pizza");
                    }
                    SECTION("mod:pizza")
                    {
                        input += "mod:pizza";
                        expected.m_data = identityRef_("mod", "pizza");
                    }
                    SECTION("pizza-module:hawaii")
                    {
                        input += "pizza-module:hawaii";
                        expected.m_data = identityRef_("pizza-module", "hawaii");
                    }
                }
                SECTION("mod:contA/identInCont")
                {
                    input = "set mod:contA/identInCont ";
                    expected.m_path.m_nodes.emplace_back(module_{"mod"}, container_("contA"));
                    expected.m_path.m_nodes.emplace_back(leaf_("identInCont"));
                    SECTION("pizza")
                    {
                        input += "pizza";
                        expected.m_data = identityRef_("pizza");
                    }
                    SECTION("mod:pizza")
                    {
                        input += "mod:pizza";
                        expected.m_data = identityRef_("mod", "pizza");
                    }
                    SECTION("pizza-module:hawaii")
                    {
                        input += "pizza-module:hawaii";
                        expected.m_data = identityRef_("pizza-module", "hawaii");
                    }
                }
            }
            SECTION("leafRef")
            {
                SECTION("refToString")
                {
                    input = "set mod:refToString \"blabal\"";
                    expected.m_path.m_nodes.emplace_back(module_{"mod"}, leaf_("refToString"));
                    expected.m_data = std::string("blabal");
                }

                SECTION("refToInt8")
                {
                    input = "set mod:refToInt8 42";
                    expected.m_path.m_nodes.emplace_back(module_{"mod"}, leaf_("refToInt8"));
                    expected.m_data = int8_t{42};
                }

                SECTION("refToLeafInCont")
                {
                    input = "set mod:refToLeafInCont pizza";
                    expected.m_path.m_nodes.emplace_back(module_{"mod"}, leaf_("refToLeafInCont"));
                    expected.m_data = identityRef_{"pizza"};
                }
            }
            SECTION("empty")
            {
                input = "set mod:dummy ";
                expected.m_path.m_nodes.emplace_back(module_{"mod"}, leaf_("dummy"));
                expected.m_data = empty_{};
            }

            SECTION("bits")
            {
                input = "set mod:flags ";
                decltype(bits_::m_bits) bits;
                SECTION("<nothing>")
                {
                    bits = {};
                }
                SECTION("carry")
                {
                    input += "carry";
                    bits = {"carry"};
                }
                SECTION("sign")
                {
                    input += "sign";
                    bits = {"sign"};
                }
                SECTION("carry sign")
                {
                    input += "carry sign";
                    bits = {"carry", "sign"};
                }
                SECTION("sign carry")
                {
                    input += "sign carry";
                    bits = {"sign", "carry"};
                }
                expected.m_path.m_nodes.emplace_back(module_{"mod"}, leaf_("flags"));
                expected.m_data = bits_{bits};
            }
        }

        SECTION("instance-identifier") {
            SECTION("toplevel")
            {
                input = "set mod:iid /mod:leafUint32";
                expected.m_path.m_nodes.emplace_back(module_{"mod"}, leaf_{"iid"});
                expected.m_data = instanceIdentifier_{"/mod:leafUint32"};
            }

            SECTION("deep to toplevel")
            {
                input = "set mod:contA/iid /mod:leafUint32";
                expected.m_path.m_nodes.emplace_back(module_{"mod"}, container_{"contA"});
                expected.m_path.m_nodes.emplace_back(leaf_{"iid"});
                expected.m_data = instanceIdentifier_{"/mod:leafUint32"};
            }

            SECTION("deep to deep unprefixed")
            {
                input = "set mod:contA/iid /mod:contA/x";
                expected.m_path.m_nodes.emplace_back(module_{"mod"}, container_{"contA"});
                expected.m_path.m_nodes.emplace_back(leaf_{"iid"});
                expected.m_data = instanceIdentifier_{"/mod:contA/x"};
            }

            SECTION("deep to deep mod-prefixed")
            {
                input = "set mod:contA/iid /mod:contA/mod:x";
                expected.m_path.m_nodes.emplace_back(module_{"mod"}, container_{"contA"});
                expected.m_path.m_nodes.emplace_back(leaf_{"iid"});
                expected.m_data = instanceIdentifier_{"/mod:contA/mod:x"};
            }

            SECTION("absolute when nested")
            {
                cwd.m_nodes.emplace_back(module_{"mod"}, container_{"contA"});
                input = "set iid /mod:contA/x";
                expected.m_path.m_nodes.emplace_back(leaf_{"iid"});
                expected.m_data = instanceIdentifier_{"/mod:contA/x"};
            }
        }

        parser.changeNode(cwd);
        command_ command = parser.parseCommand(input, errorStream);
        REQUIRE(command.type() == typeid(set_));
        REQUIRE(boost::get<set_>(command) == expected);
    }

    SECTION("invalid input")
    {
        std::string expectedError;
        dataPath_ cwd;

        SECTION("missing space between a command and its arguments")
        {
            SECTION("setmod:leafString some_data")
            {
                input = "setmod:leafString 'some_data'";
            }
        }

        SECTION("missing space between arguments")
        {
            SECTION("set mod:leafString'lol'")
            {
                input = "set mod:leafString'lol'";
            }
        }

        SECTION("non-leaf identifiers")
        {
            SECTION("set mod:nonexistent 'blabla'")
            {
                input = "set mod:nonexistent 'blabla'";
            }

            SECTION("set mod:contA 'abde'")
            {
                input = "set mod:contA 'abde'";
            }
        }

        SECTION("wrong types")
        {
            expectedError = "leaf data type mismatch";
            SECTION("set mod:leafBool 'blabla'")
            {
                input = "set mod:leafBool 'blabla'";
            }
            SECTION("set mod:leafUint8 'blabla'")
            {
                input = "set mod:leafUint8 'blabla'";
            }
            SECTION("set mod:leafUint8 -5")
            {
                input = "set mod:leafUint8 -5";
            }
            SECTION("set mod:leafInt8 'blabla'")
            {
                input = "set mod:leafInt8 'blabla'";
            }
            SECTION("set mod:leafInt8 130")
            {
                input = "set mod:leafInt8 130";
            }
            SECTION("set mod:leafUint16 'blabla'")
            {
                input = "set mod:leafUint16 'blabla'";
            }
            SECTION("set mod:leafInt16 'blabla'")
            {
                input = "set mod:leafInt16 'blabla'";
            }
            SECTION("set mod:leafUint32 'blabla'")
            {
                input = "set mod:leafUint32 'blabla'";
            }
            SECTION("set mod:leafInt32 'blabla'")
            {
                input = "set mod:leafInt32 'blabla'";
            }
            SECTION("set mod:leafUint64 'blabla'")
            {
                input = "set mod:leafUint64 'blabla'";
            }
            SECTION("set mod:leafInt64 'blabla'")
            {
                input = "set mod:leafInt64 'blabla'";
            }
            SECTION("set mod:leafEnum 'blabla'")
            {
                input = "set mod:leafEnum 'blabla'";
            }
            SECTION("set mod:refToInt8 'blabla'")
            {
                input = "set mod:refToInt8 'blabla'";
            }
        }

        SECTION("wrong base64 strings")
        {
            SECTION("invalid character")
            {
                input = "set mod:leafBinary dbahj-";
            }
            SECTION("equal sign in the middle")
            {
                input = "set mod:leafBinary db=ahj";
            }
            SECTION("enclosing in quotes")
            {
                input = "set mod:leafBinary 'dbahj'";
            }
        }

        SECTION("non-existing identity")
        {
            input = "set mod:foodIdentRef identityBLABLA";
        }

        SECTION("identity with non-existing module")
        {
            expectedError = "Invalid module name";
            input = "set mod:foodIdentRef xd:haha";
        }

        SECTION("setting identities with wrong bases")
        {
            SECTION("set mod:foodIdentRef mod:vehicle")
            {
                input = "set mod:foodIdentRef mod:vehicle";
            }
            SECTION("set mod:pizzaIdentRef mod:food")
            {
                input = "set mod:pizzaIdentRef mod:food";
            }
        }
        SECTION("setting different module identities without prefix")
        {
            input = "set mod:pizzaIdentRef hawaii";
        }
        SECTION("identity prefix without name")
        {
            input = "set mod:contA/identInCont pizza-module:";
        }

        SECTION("set a union path to a wrong type")
        {
            input = "set mod:intOrString true";
        }

        SECTION("no space for empty data")
        {
            input = "set mod:dummy";
        }

        SECTION("empty path")
        {
            input = "set ";
        }

        SECTION("setting readonly data")
        {
            input = "set mod:readonly 123";
        }

        SECTION("nonexistent bits")
        {
            input = "set mod:flags daw";
        }

        SECTION("same bit more than once")
        {
            input = "set mod:flags carry carry";
        }

        SECTION("instance-identifier non-existing node")
        {
            input = "set mod:iid /mod:404";
        }

        SECTION("instance-identifier non-existing node without leading slash")
        {
            input = "set mod:iid mod:404";
        }

        SECTION("instance-identifier node without leading slash")
        {
            input = "set mod:iid mod:leafUint32";
        }

        SECTION("instance-identifier to pseudo-relative unprefixed")
        {
            cwd.m_nodes.emplace_back(module_{"mod"}, container_{"contA"});
            input = "set iid x";
        }

        SECTION("instance-identifier to pseudo-relative prefixed")
        {
            cwd.m_nodes.emplace_back(module_{"mod"}, container_{"contA"});
            input = "set iid mod:x";
        }

        SECTION("instance-identifier without leading slash when nested")
        {
            cwd.m_nodes.emplace_back(module_{"mod"}, container_{"contA"});
            input = "set iid mod:contA/x";
        }

        parser.changeNode(cwd);
        REQUIRE_THROWS_AS(parser.parseCommand(input, errorStream), InvalidCommandException);
        REQUIRE(errorStream.str().find(expectedError) != std::string::npos);
    }

    SECTION("deleting a leaf")
    {
        delete_ expected;
        input = "delete mod:leafString";
        expected.m_path.m_nodes.emplace_back(module_{"mod"}, leaf_("leafString"));

        command_ command = parser.parseCommand(input, errorStream);
        REQUIRE(command.type() == typeid(delete_));
        REQUIRE(boost::get<delete_>(command) == expected);
    }
}
