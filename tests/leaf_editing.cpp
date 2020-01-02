/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/

#include "trompeloeil_doctest.h"
#include <boost/core/demangle.hpp>
#include "ast_commands.hpp"
#include "datastoreaccess_mock.hpp"
#include "parser.hpp"
#include "static_schema.hpp"
#include "utils.hpp"

std::ostream& operator<<(std::ostream& s, const set_ cmd)
{
    return s << "Command SET {path: " << pathToAbsoluteSchemaString(cmd.m_path) << ", type " << boost::core::demangle(cmd.m_data.type().name()) << ", data: " << leafDataToString(cmd.m_data) << "}";
}

TEST_CASE("leaf editing")
{
    auto schema = std::make_shared<StaticSchema>();
    schema->addModule("mod");
    schema->addModule("pizza-module");
    schema->addContainer("", "mod:contA");
    schema->addLeaf("", "mod:leafString", yang::LeafDataTypes::String);
    schema->addLeaf("", "mod:leafDecimal", yang::LeafDataTypes::Decimal);
    schema->addLeaf("", "mod:leafBool", yang::LeafDataTypes::Bool);
    schema->addLeaf("", "mod:leafInt8", yang::LeafDataTypes::Int8);
    schema->addLeaf("", "mod:leafInt16", yang::LeafDataTypes::Int16);
    schema->addLeaf("", "mod:leafInt32", yang::LeafDataTypes::Int32);
    schema->addLeaf("", "mod:leafInt64", yang::LeafDataTypes::Int64);
    schema->addLeaf("", "mod:leafUint8", yang::LeafDataTypes::Uint8);
    schema->addLeaf("", "mod:leafUint16", yang::LeafDataTypes::Uint16);
    schema->addLeaf("", "mod:leafUint32", yang::LeafDataTypes::Uint32);
    schema->addLeaf("", "mod:leafUint64", yang::LeafDataTypes::Uint64);
    schema->addLeaf("", "mod:leafBinary", yang::LeafDataTypes::Binary);
    schema->addIdentity(std::nullopt, ModuleValuePair{"mod", "food"});
    schema->addIdentity(std::nullopt, ModuleValuePair{"mod", "vehicle"});
    schema->addIdentity(ModuleValuePair{"mod", "food"}, ModuleValuePair{"mod", "pizza"});
    schema->addIdentity(ModuleValuePair{"mod", "food"}, ModuleValuePair{"mod", "spaghetti"});
    schema->addIdentity(ModuleValuePair{"mod", "pizza"}, ModuleValuePair{"pizza-module", "hawaii"});
    schema->addLeafIdentityRef("", "mod:foodIdentRef", ModuleValuePair{"mod", "food"});
    schema->addLeafIdentityRef("", "mod:pizzaIdentRef", ModuleValuePair{"mod", "pizza"});
    schema->addLeafIdentityRef("mod:contA", "mod:identInCont", ModuleValuePair{"mod", "pizza"});
    schema->addLeafEnum("", "mod:leafEnum", {"lol", "data", "coze"});
    schema->addLeaf("mod:contA", "mod:leafInCont", yang::LeafDataTypes::String);
    schema->addList("", "mod:list", {"number"});
    schema->addLeaf("mod:list", "mod:leafInList", yang::LeafDataTypes::String);
    schema->addLeafRef("", "mod:refToString", "mod:leafString");
    schema->addLeafRef("", "mod:refToInt8", "mod:leafInt8");
    std::shared_ptr<DatastoreAccess> mockDatastore = std::make_shared<MockDatastoreAccess>();
    auto dataQuery = std::make_shared<DataQuery>(*mockDatastore);
    Parser parser(schema, dataQuery);
    std::string input;
    std::ostringstream errorStream;

    SECTION("valid input")
    {
        set_ expected;

        SECTION("set mod:leafString some_data")
        {
            input = "set mod:leafString some_data";
            expected.m_path.m_nodes.push_back(dataNode_{module_{"mod"}, leaf_("leafString")});
            expected.m_data = std::string("some_data");
        }

        SECTION("set mod:contA/leafInCont more_data")
        {
            input = "set mod:contA/leafInCont more_data";
            expected.m_path.m_nodes.push_back(dataNode_{module_{"mod"}, container_("contA")});
            expected.m_path.m_nodes.push_back(dataNode_{leaf_("leafInCont")});
            expected.m_data = std::string("more_data");
        }

        SECTION("set mod:list[number=1]/leafInList another_data")
        {
            input = "set mod:list[number=1]/leafInList another_data";
            auto keys = std::map<std::string, std::string>{
                {"number", "1"}};
            expected.m_path.m_nodes.push_back(dataNode_{module_{"mod"}, listElement_("list", keys)});
            expected.m_path.m_nodes.push_back(dataNode_{leaf_("leafInList")});
            expected.m_data = std::string("another_data");
        }

        SECTION("data types")
        {
            SECTION("string")
            {
                input = "set mod:leafString somedata";
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
                    input = "set mod:refToString blabal";
                    expected.m_path.m_nodes.push_back(dataNode_{module_{"mod"}, leaf_("refToString")});
                    expected.m_data = std::string("blabal");
                }

                SECTION("refToInt8")
                {
                    input = "set mod:refToInt8 42";
                    expected.m_path.m_nodes.push_back(dataNode_{module_{"mod"}, leaf_("refToInt8")});
                    expected.m_data = int8_t{42};
                }
            }
        }

        command_ command = parser.parseCommand(input, errorStream);
        REQUIRE(command.type() == typeid(set_));
        REQUIRE(boost::get<set_>(command) == expected);
    }

    SECTION("invalid input")
    {
        SECTION("missing space between a command and its arguments")
        {
            SECTION("setleaf some_data")
            {
                input = "setleaf some_data";
            }
        }

        SECTION("missing space between arguments")
        {
            SECTION("set leaflol")
            {
                input = "set leaflol";
            }
        }

        SECTION("non-leaf identifiers")
        {
            SECTION("set nonexistent blabla")
            {
                input = "set nonexistent blabla";
            }

            SECTION("set contA abde")
            {
                input = "set contA abde";
            }
        }

        SECTION("wrong types")
        {
            SECTION("set leafBool blabla")
            {
                input = "set leafBool blabla";
            }
            SECTION("set leafUint8 blabla")
            {
                input = "set leafUint8 blabla";
            }
            SECTION("set leafUint8 -5")
            {
                input = "set leafUint8 -5";
            }
            SECTION("set leafInt8 blabla")
            {
                input = "set leafInt8 blabla";
            }
            SECTION("set leafInt8 130")
            {
                input = "set leafInt8 130";
            }
            SECTION("set leafUint16 blabla")
            {
                input = "set leafUint16 blabla";
            }
            SECTION("set leafInt16 blabla")
            {
                input = "set leafInt16 blabla";
            }
            SECTION("set leafUint32 blabla")
            {
                input = "set leafUint32 blabla";
            }
            SECTION("set leafInt32 blabla")
            {
                input = "set leafInt32 blabla";
            }
            SECTION("set leafUint64 blabla")
            {
                input = "set leafUint64 blabla";
            }
            SECTION("set leafInt64 blabla")
            {
                input = "set leafInt64 blabla";
            }
            SECTION("set leafEnum blabla")
            {
                input = "set leafEnum blabla";
            }
            SECTION("set mod:refToInt8 blabla")
            {
                input = "set mod:refToInt8 blabla";
            }
        }

        SECTION("wrong base64 strings")
        {
            SECTION("invalid character")
                input = "set leafBinary dbahj-";
            SECTION("equal sign in the middle")
                input = "set leafBinary db=ahj";
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
    }
}
