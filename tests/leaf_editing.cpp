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
#include "static_schema.hpp"
#include "utils.hpp"

std::ostream& operator<<(std::ostream& s, const set_ cmd)
{
    return s << "Command SET {path: " << pathToSchemaString(cmd.m_path, Prefixes::Always) << ", type " << boost::core::demangle(cmd.m_data.type().name()) << ", data: " << leafDataToString(cmd.m_data) << "}";
}

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
    schema->addIdentity(std::nullopt, ModuleValuePair{"mod", "food"});
    schema->addIdentity(std::nullopt, ModuleValuePair{"mod", "vehicle"});
    schema->addIdentity(ModuleValuePair{"mod", "food"}, ModuleValuePair{"mod", "pizza"});
    schema->addIdentity(ModuleValuePair{"mod", "food"}, ModuleValuePair{"mod", "spaghetti"});
    schema->addIdentity(ModuleValuePair{"mod", "pizza"}, ModuleValuePair{"pizza-module", "hawaii"});
    schema->addLeaf("/", "mod:foodIdentRef", yang::IdentityRef{schema->validIdentities("mod", "food")});
    schema->addLeaf("/", "mod:pizzaIdentRef", yang::IdentityRef{schema->validIdentities("mod", "pizza")});
    schema->addLeaf("/mod:contA", "mod:identInCont", yang::IdentityRef{schema->validIdentities("mod", "pizza")});
    schema->addLeaf("/", "mod:leafEnum", createEnum({"lol", "data", "coze"}));
    schema->addLeaf("/mod:contA", "mod:leafInCont", yang::String{});
    schema->addList("/", "mod:list", {"number"});
    schema->addLeaf("/mod:list", "mod:number", yang::Int32{});
    schema->addLeaf("/mod:list", "mod:leafInList", yang::String{});
    schema->addLeaf("/", "mod:refToString", yang::LeafRef{"/mod:leafString", std::make_unique<yang::LeafDataType>(schema->leafType("/mod:leafString"))});
    schema->addLeaf("/", "mod:refToInt8", yang::LeafRef{"/mod:leafInt8", std::make_unique<yang::LeafDataType>(schema->leafType("/mod:leafInt8"))});
    schema->addLeaf("/", "mod:refToLeafInCont", yang::LeafRef{"/mod:contA/identInCont", std::make_unique<yang::LeafDataType>(schema->leafType("/mod:contA/mod:identInCont"))});
    Parser parser(schema);
    std::string input;
    std::ostringstream errorStream;

    SECTION("valid input")
    {
        set_ expected;

        SECTION("set mod:leafString \"some_data\"")
        {
            input = "set mod:leafString \'some_data\'";
            expected.m_path.m_nodes.push_back(dataNode_{module_{"mod"}, leaf_("leafString")});
            expected.m_data = std::string("some_data");
        }

        SECTION("set mod:contA/leafInCont 'more_data'")
        {
            input = "set mod:contA/leafInCont 'more_data'";
            expected.m_path.m_nodes.push_back(dataNode_{module_{"mod"}, container_("contA")});
            expected.m_path.m_nodes.push_back(dataNode_{leaf_("leafInCont")});
            expected.m_data = std::string("more_data");
        }

        SECTION("set mod:contA/leafInCont \"data with' a quote\"")
        {
            input = "set mod:contA/leafInCont \"data with' a quote\"";
            expected.m_path.m_nodes.push_back(dataNode_{module_{"mod"}, container_("contA")});
            expected.m_path.m_nodes.push_back(dataNode_{leaf_("leafInCont")});
            expected.m_data = std::string("data with' a quote");
        }

        SECTION("set mod:contA/leafInCont 'data with\" a quote'")
        {
            input = "set mod:contA/leafInCont 'data with\" a quote'";
            expected.m_path.m_nodes.push_back(dataNode_{module_{"mod"}, container_("contA")});
            expected.m_path.m_nodes.push_back(dataNode_{leaf_("leafInCont")});
            expected.m_data = std::string("data with\" a quote");
        }

        SECTION("set mod:contA/leafInCont   'more   d\tata'") // spaces in string
        {
            input = "set mod:contA/leafInCont 'more   d\tata'";
            expected.m_path.m_nodes.push_back(dataNode_{module_{"mod"}, container_("contA")});
            expected.m_path.m_nodes.push_back(dataNode_{leaf_("leafInCont")});
            expected.m_data = std::string("more   d\tata");
        }

        SECTION("set mod:list[number=1]/leafInList \"another_data\"")
        {
            input = "set mod:list[number=1]/leafInList \"another_data\"";
            auto keys = std::map<std::string, leaf_data_>{
                {"number", int32_t{1}}};
            expected.m_path.m_nodes.push_back(dataNode_{module_{"mod"}, listElement_("list", keys)});
            expected.m_path.m_nodes.push_back(dataNode_{leaf_("leafInList")});
            expected.m_data = std::string("another_data");
        }

        SECTION("data types")
        {
            SECTION("string")
            {
                input = "set mod:leafString \"somedata\"";
                expected.m_path.m_nodes.push_back(dataNode_{module_{"mod"}, leaf_("leafString")});
                expected.m_data = std::string("somedata");
            }

            SECTION("int8")
            {
                input = "set mod:leafInt8 2";
                expected.m_path.m_nodes.push_back(dataNode_{module_{"mod"}, leaf_("leafInt8")});
                expected.m_data = int8_t{2};
            }

            SECTION("negative int8")
            {
                input = "set mod:leafInt8 -10";
                expected.m_path.m_nodes.push_back(dataNode_{module_{"mod"}, leaf_("leafInt8")});
                expected.m_data = int8_t{-10};
            }

            SECTION("uint8")
            {
                input = "set mod:leafUint8 2";
                expected.m_path.m_nodes.push_back(dataNode_{module_{"mod"}, leaf_("leafUint8")});
                expected.m_data = uint8_t{2};
            }

            SECTION("int16")
            {
                input = "set mod:leafInt16 30000";
                expected.m_path.m_nodes.push_back(dataNode_{module_{"mod"}, leaf_("leafInt16")});
                expected.m_data = int16_t{30'000};
            }

            SECTION("uint16")
            {
                input = "set mod:leafUint16 30000";
                expected.m_path.m_nodes.push_back(dataNode_{module_{"mod"}, leaf_("leafUint16")});
                expected.m_data = uint16_t{30'000};
            }

            SECTION("int32")
            {
                input = "set mod:leafInt32 30000";
                expected.m_path.m_nodes.push_back(dataNode_{module_{"mod"}, leaf_("leafInt32")});
                expected.m_data = int32_t{30'000};
            }

            SECTION("uint32")
            {
                input = "set mod:leafUint32 30000";
                expected.m_path.m_nodes.push_back(dataNode_{module_{"mod"}, leaf_("leafUint32")});
                expected.m_data = uint32_t{30'000};
            }

            SECTION("int32")
            {
                input = "set mod:leafInt32 30000";
                expected.m_path.m_nodes.push_back(dataNode_{module_{"mod"}, leaf_("leafInt32")});
                expected.m_data = int32_t{30'000};
            }

            SECTION("uint64")
            {
                input = "set mod:leafUint64 30000";
                expected.m_path.m_nodes.push_back(dataNode_{module_{"mod"}, leaf_("leafUint64")});
                expected.m_data = uint64_t{30'000};
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
                expected.m_path.m_nodes.push_back(dataNode_{module_{"mod"}, leaf_("leafBinary")});
            }

            SECTION("identityRef")
            {
                SECTION("foodIdentRef")
                {
                    input = "set mod:foodIdentRef ";
                    expected.m_path.m_nodes.push_back(dataNode_{module_{"mod"}, leaf_("foodIdentRef")});

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
                    expected.m_path.m_nodes.push_back(dataNode_{module_{"mod"}, leaf_("pizzaIdentRef")});
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
                    expected.m_path.m_nodes.push_back(dataNode_{module_{"mod"}, container_("contA")});
                    expected.m_path.m_nodes.push_back(dataNode_(leaf_("identInCont")));
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
                    expected.m_path.m_nodes.push_back(dataNode_{module_{"mod"}, leaf_("refToString")});
                    expected.m_data = std::string("blabal");
                }

                SECTION("refToInt8")
                {
                    input = "set mod:refToInt8 42";
                    expected.m_path.m_nodes.push_back(dataNode_{module_{"mod"}, leaf_("refToInt8")});
                    expected.m_data = int8_t{42};
                }

                SECTION("refToLeafInCont")
                {
                    input = "set mod:refToLeafInCont pizza";
                    expected.m_path.m_nodes.push_back(dataNode_{module_{"mod"}, leaf_("refToLeafInCont")});
                    expected.m_data = identityRef_{"pizza"};
                }
            }
        }

        command_ command = parser.parseCommand(input, errorStream);
        REQUIRE(command.type() == typeid(set_));
        REQUIRE(boost::get<set_>(command) == expected);
    }

    SECTION("invalid input")
    {
        std::string expectedError;
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
                input = "set mod:leafBinary dbahj-";
            SECTION("equal sign in the middle")
                input = "set mod:leafBinary db=ahj";
            SECTION("enclosing in quotes")
                input = "set mod:leafBinary 'dbahj'";
        }

        SECTION("non-existing identity")
        {
            input = "set mod:foodIdentRef identityBLABLA";
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

        REQUIRE_THROWS_AS(parser.parseCommand(input, errorStream), InvalidCommandException);
        REQUIRE(errorStream.str().find(expectedError) != std::string::npos);
    }
}
