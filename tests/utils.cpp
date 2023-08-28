/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/

#include "trompeloeil_doctest.hpp"
#include <libyang-cpp/Context.hpp>
#include "completion.hpp"
#include "leaf_data_helpers.hpp"
#include "libyang_utils.hpp"
#include "pretty_printers.hpp"
#include "utils.hpp"

using namespace std::literals;

TEST_CASE("utils")
{
    SECTION("filterByPrefix")
    {
        std::set<Completion> set{{"ahoj"}, {"coze"}, {"copak"}, {"aha"}, {"polivka"}};

        REQUIRE((filterByPrefix(set, "a") == std::set<Completion>{{"ahoj"}, {"aha"}}));
        REQUIRE((filterByPrefix(set, "ah") == std::set<Completion>{{"ahoj"}, {"aha"}}));
        REQUIRE((filterByPrefix(set, "aho") == std::set<Completion>{{"ahoj"}}));
        REQUIRE((filterByPrefix(set, "polivka") == std::set<Completion>{{"polivka"}}));
        REQUIRE((filterByPrefix(set, "polivkax") == std::set<Completion>{}));
        REQUIRE((filterByPrefix(set, "co") == std::set<Completion>{{"copak"}, {"coze"}}));
    }

    SECTION("joinPaths")
    {
        std::string prefix, suffix, result;

        SECTION("regular")
        {
            prefix = "/example:a";
            suffix = "leaf";
            result = "/example:a/leaf";
        }

        SECTION("no prefix - absolute path")
        {
            suffix = "/example:a/leaf";
            result = "/example:a/leaf";
        }

        SECTION("no prefix - relative path")
        {
            suffix = "example:a/leaf";
            result = "example:a/leaf";
        }

        SECTION("no suffix")
        {
            prefix = "/example:a/leaf";
            result = "/example:a/leaf";
        }

        SECTION("at root")
        {
            prefix = "/";
            suffix = "example:a";
            result = "/example:a";
        }

        SECTION("trailing slash")
        {
            prefix = "/example:a";
            suffix = "/";
            result = "/example:a/";
        }

        SECTION("prefix ends with slash")
        {
            prefix = "/example:a/";
            suffix = "leaf";
            result = "/example:a/leaf";
        }

        SECTION("suffix starts with slash")
        {
            prefix = "/example:a";
            suffix = "/leaf";
            result = "/example:a/leaf";
        }

        SECTION("slashes all the way to eleven")
        {
            prefix = "/example:a/";
            suffix = "/leaf";
            result = "/example:a/leaf";
        }

        REQUIRE(joinPaths(prefix, suffix) == result);
    }

    SECTION("leafDataTypeToString")
    {
        yang::LeafDataType type;
        std::string expected;
        SECTION("union")
        {
            type = yang::Union{{
                yang::TypeInfo{yang::String{}},
                yang::TypeInfo{createEnum({"foo", "bar"})},
                yang::TypeInfo{yang::Int8{}},
                yang::TypeInfo{yang::Int64{}},
            }};
            expected = "a string, an enum, an 8-bit integer, a 64-bit integer";
        }

        REQUIRE(leafDataTypeToString(type) == expected);
    }
}

const auto schema = R"(
module test-schema {
    namespace "http://example.com/ayyyy";
    prefix AHOJ;

    leaf int8 {
        type int8;
    }
    leaf int16 {
        type int16;
    }
    leaf int32 {
        type int32;
    }
    leaf int64 {
        type int64;
    }
    leaf uint8 {
        type uint8;
    }
    leaf uint16 {
        type uint16;
    }
    leaf uint32 {
        type uint32;
    }
    leaf uint64 {
        type uint64;
    }
    leaf boolean {
        type boolean;
    }
    leaf string {
        type string;
    }
    leaf enum {
        type enumeration {
            enum A;
            enum B;
            enum C;
        }
    }
    identity food;
    identity apple {
        base "food";
    }
    leaf identityRef {
        type identityref {
            base "food";
        }
    }
    leaf binary {
        type binary;
    }
    leaf empty {
        type empty;
    }
    leaf bits {
        type bits {
            bit a;
            bit b;
            bit AHOJ;
        }
    }
    typedef capabilitiesType {
        type bits {
            bit router;
            bit switch;
            bit hub;
        }
    }
    leaf capabilities {
        type capabilitiesType;
    }
    leaf dec64 {
        type decimal64 {
            fraction-digits 5;
        }
    }

    list stuff {
        key "name";
        leaf name {
            type string;
        }
    }

    leaf leafRefPresent {
        type leafref {
            path ../stuff/name;
        }
    }

    container users {
        config false;
        list userList {
            leaf name {
                type string;
            }
        }
    }

    leaf iid-valid {
        type instance-identifier;
    }

    leaf iid-relaxed {
        type instance-identifier {
            require-instance false;
        }
    }
}
)"s;

const auto data = R"(
{
    "test-schema:int8": 8,
    "test-schema:int16": 300,
    "test-schema:int32": -300,
    "test-schema:int64": "-999999999999999",
    "test-schema:uint8": 8,
    "test-schema:uint16": 300,
    "test-schema:uint32": 300,
    "test-schema:uint64": "999999999999999",
    "test-schema:boolean": true,
    "test-schema:string": "AHOJ",
    "test-schema:enum": "A",
    "test-schema:identityRef": "apple",
    "test-schema:binary": "QUhPSgo=",
    "test-schema:empty": [null],
    "test-schema:bits": "a AHOJ",
    "test-schema:capabilities": "switch hub",
    "test-schema:dec64": "43242.43260",
    "test-schema:stuff": [
        {
            "name": "Xaver"
        }
    ],
    "test-schema:leafRefPresent": "Xaver",
    "test-schema:users": {
        "userList": [
            {
                "name": "John"
            },
            {
                "name": "Aneta"
            },
            {
                "name": "Aneta"
            }
        ]
    },
    "test-schema:iid-valid": "/test-schema:stuff[name='Xaver']/name",
    "test-schema:iid-relaxed": "/test-schema:stuff[name='XXX']/name"
}
)"s;


TEST_CASE("libyang_utils")
{
    libyang::Context ctx;
    ctx.parseModule(schema, libyang::SchemaFormat::YANG);
    auto dataNode = ctx.parseData(data, libyang::DataFormat::JSON, std::nullopt, libyang::ValidationOptions::Present);

    SECTION("leafValueFromNode")
    {
        std::string path;
        leaf_data_ expectedLeafData;

        SECTION("test-schema:int8")
        {
            path = "test-schema:int8";
            expectedLeafData = int8_t{8};
        }
        SECTION("test-schema:int16")
        {
            path = "test-schema:int16";
            expectedLeafData = int16_t{300};
        }
        SECTION("test-schema:int32")
        {
            path = "test-schema:int32";
            expectedLeafData = int32_t{-300};
        }
        SECTION("test-schema:int64")
        {
            path = "test-schema:int64";
            expectedLeafData = int64_t{-999999999999999};
        }
        SECTION("test-schema:uint8")
        {
            path = "test-schema:uint8";
            expectedLeafData = uint8_t{8};
        }
        SECTION("test-schema:uint16")
        {
            path = "test-schema:uint16";
            expectedLeafData = uint16_t{300};
        }
        SECTION("test-schema:uint32")
        {
            path = "test-schema:uint32";
            expectedLeafData = uint32_t{300};
        }
        SECTION("test-schema:uint64")
        {
            path = "test-schema:uint64";
            expectedLeafData = uint64_t{999999999999999};
        }
        SECTION("test-schema:boolean")
        {
            path = "test-schema:boolean";
            expectedLeafData = true;
        }
        SECTION("test-schema:string")
        {
            path = "test-schema:string";
            expectedLeafData = std::string{"AHOJ"};
        }
        SECTION("test-schema:enum")
        {
            path = "test-schema:enum";
            expectedLeafData = enum_{"A"};
        }
        SECTION("test-schema:identityRef")
        {
            path = "test-schema:identityRef";
            expectedLeafData = identityRef_{"test-schema", "apple"};
        }
        SECTION("test-schema:binary")
        {
            path = "test-schema:binary";
            expectedLeafData = binary_{"QUhPSgo="};
        }
        SECTION("test-schema:empty")
        {
            path = "test-schema:empty";
            expectedLeafData = empty_{};
        }
        SECTION("test-schema:bits")
        {
            path = "test-schema:bits";
            expectedLeafData = bits_{{"a", "AHOJ"}};
        }
        SECTION("test-schema:dec64")
        {
            path = "test-schema:dec64";
            expectedLeafData = 43242.43260;
        }
        SECTION("test-schema:leafRefPresent")
        {
            path = "test-schema:leafRefPresent";
            expectedLeafData = std::string{"Xaver"};
        }

        auto leaf = dataNode->findPath("/" + path);
        REQUIRE(leafValueFromNode(leaf->asTerm()) == expectedLeafData);
    }

    SECTION("lyNodesToTree")
    {
        DatastoreAccess::Tree expected{
            {"/test-schema:int8", int8_t{8}},
            {"/test-schema:int16", int16_t{300}},
            {"/test-schema:int32", int32_t{-300}},
            {"/test-schema:int64", int64_t{-999999999999999}},
            {"/test-schema:uint8", uint8_t{8}},
            {"/test-schema:uint16", uint16_t{300}},
            {"/test-schema:uint32", uint32_t{300}},
            {"/test-schema:uint64", uint64_t{999999999999999}},
            {"/test-schema:boolean", true},
            {"/test-schema:string", std::string{"AHOJ"}},
            {"/test-schema:enum", enum_{"A"}},
            {"/test-schema:identityRef", identityRef_{"test-schema", "apple"}},
            {"/test-schema:binary", binary_{"QUhPSgo="}},
            {"/test-schema:empty", empty_{}},
            {"/test-schema:bits", bits_{{"a", "AHOJ"}}},
            {"/test-schema:capabilities", bits_{{"switch", "hub"}}},
            {"/test-schema:dec64", 43242.432600},
            {"/test-schema:stuff[name='Xaver']", special_{SpecialValue::List}},
            {"/test-schema:stuff[name='Xaver']/name", std::string{"Xaver"}},
            {"/test-schema:leafRefPresent", std::string{"Xaver"}},
            {"/test-schema:users/userList[1]", special_{SpecialValue::List}},
            {"/test-schema:users/userList[1]/name", std::string{"John"}},
            {"/test-schema:users/userList[2]", special_{SpecialValue::List}},
            {"/test-schema:users/userList[2]/name", std::string{"Aneta"}},
            {"/test-schema:users/userList[3]", special_{SpecialValue::List}},
            {"/test-schema:users/userList[3]/name", std::string{"Aneta"}},
            {"/test-schema:iid-valid", instanceIdentifier_{"/test-schema:stuff[name='Xaver']/name"}},
            {"/test-schema:iid-relaxed", instanceIdentifier_{"/test-schema:stuff[name='XXX']/name"}},
        };

        DatastoreAccess::Tree tree;
        lyNodesToTree(tree, dataNode->siblings());
        REQUIRE(tree == expected);
    }
}
