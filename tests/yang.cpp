/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/

#include "trompeloeil_doctest.h"
#include "yang_schema.hpp"

const char* second_schema = R"(
module second-schema {
    namespace "http://example.com/nevim";
    prefix second;

    import example-schema {
        prefix "example";
    }

    identity pineapple {
        base "example:fruit";
    }

    augment /example:a {
        container augmentedContainer {
        }
    }

    container bla {
        container bla2 {
        }
    }
}
)";

const char* example_schema = R"(
module example-schema {
    yang-version 1.1;
    namespace "http://example.com/example-sports";
    prefix coze;

    identity drink {
    }

    identity voda {
        base "drink";
    }

    identity food {
    }

    identity fruit {
        base "food";
    }

    identity pizza {
        base "food";
    }

    identity hawaii {
        base "pizza";
    }

    container a {
        container a2 {
            container a3 {
                presence true;
            }
        }

        leaf leafa {
            type string;
        }
    }

    container b {
        container b2 {
            presence true;
            container b3 {
            }
        }
    }

    leaf leafString {
        type string;
    }

    leaf leafDecimal {
        type decimal64 {
            fraction-digits 5;
        }
    }

    leaf leafBool {
        type boolean;
    }

    leaf leafInt8 {
        type int8;
    }

    leaf leafUint8 {
        type uint8;
    }

    leaf leafInt16 {
        type int16;
    }

    leaf leafUint16 {
        type uint16;
    }

    leaf leafInt32 {
        type int32;
    }

    leaf leafUint32 {
        type uint32;
    }

    leaf leafInt64 {
        type int64;
    }

    leaf leafUint64 {
        type uint64;
    }

    leaf leafEnum {
        type enumeration {
            enum lol;
            enum data;
            enum coze;
        }
    }

    typedef enumTypedef {
        type enumeration {
            enum lol;
            enum data;
            enum coze;
        }
    }

    typedef enumTypedefRestricted {
        type enumTypedef {
            enum lol;
            enum data;
        }
    }

    leaf leafEnumTypedef {
        type enumTypedef;
    }

    leaf leafEnumTypedefRestricted {
        type enumTypedef {
            enum data;
            enum coze;
        }
    }

    leaf leafEnumTypedefRestricted2 {
        type enumTypedefRestricted;
    }

    leaf foodIdentLeaf {
        type identityref {
            base "food";
        }
    }

    leaf pizzaIdentLeaf {
        type identityref {
            base "pizza";
        }
    }

    leaf foodDrinkIdentLeaf {
        type identityref {
            base "food";
            base "drink";
        }
    }

    list _list {
        key number;

        leaf number {
            type int32;
        }

        container contInList {
            presence true;
        }
    }

    list twoKeyList {
        key "name number";

        leaf number {
            type int32;
        }

        leaf name {
            type string;
        }
    }

    grouping arithmeticFlags {
        leaf carry {
            type boolean;
        }
        leaf zero {
            type boolean;
        }
    }

    grouping flags {
        leaf direction {
            type boolean;
        }
        leaf interrupt {
            type boolean;
        }

        uses arithmeticFlags;
    }

    uses flags;

    choice interface {
        case caseLoopback {
            container loopback {
                leaf ip {
                    type string;
                }
            }
        }

        case caseEthernet {
            container ethernet {
                leaf ip {
                    type string;
                }
            }
        }
    }

    feature bigPizzas;

    leaf pizzaSize {
        type enumeration {
            enum large {
                if-feature "bigPizzas";
            }
            enum medium;
            enum small;
        }
    }

})";

TEST_CASE("yangschema")
{
    using namespace std::string_view_literals;
    YangSchema ys;
    ys.registerModuleCallback([]([[maybe_unused]] auto modName, auto, auto) {
        assert("example-schema"sv == modName);
        return example_schema;
    });
    ys.addSchemaString(second_schema);

    schemaPath_ path;
    ModuleNodePair node;

    SECTION("positive")
    {
        SECTION("isContainer")
        {
            SECTION("example-schema:a")
            {
                node.first = "example-schema";
                node.second = "a";
            }

            SECTION("example-schema:a/a2")
            {
                path.m_nodes.push_back(schemaNode_(module_{"example-schema"}, container_("a")));
                node.second = "a2";
            }

            SECTION("example-schema:ethernet")
            {
                node.first = "example-schema";
                node.second = "ethernet";
            }

            SECTION("example-schema:loopback")
            {
                node.first = "example-schema";
                node.second = "loopback";
            }

            REQUIRE(ys.isContainer(path, node));
        }
        SECTION("isLeaf")
        {
            SECTION("example-schema:leafString")
            {
                node.first = "example-schema";
                node.second = "leafString";
            }

            SECTION("example-schema:a/leafa")
            {
                path.m_nodes.push_back(schemaNode_(module_{"example-schema"}, container_("a")));
                node.first = "example-schema";
                node.second = "leafa";
            }

            SECTION("example-schema:carry")
            {
                node.first = "example-schema";
                node.second = "carry";
            }

            SECTION("example-schema:zero")
            {
                node.first = "example-schema";
                node.second = "zero";
            }

            SECTION("example-schema:direction")
            {
                node.first = "example-schema";
                node.second = "direction";
            }

            SECTION("example-schema:interrupt")
            {
                node.first = "example-schema";
                node.second = "interrupt";
            }

            REQUIRE(ys.isLeaf(path, node));
        }
        SECTION("isModule")
        {
            REQUIRE(ys.isModule(path, "example-schema"));
        }
        SECTION("isList")
        {
            SECTION("example-schema:_list")
            {
                node.first = "example-schema";
                node.second = "_list";
            }

            SECTION("example-schema:twoKeyList")
            {
                node.first = "example-schema";
                node.second = "twoKeyList";
            }

            REQUIRE(ys.isList(path, node));
        }
        SECTION("isPresenceContainer")
        {
            SECTION("example-schema:a/a2/a3")
            {
                path.m_nodes.push_back(schemaNode_(module_{"example-schema"}, container_("a")));
                path.m_nodes.push_back(schemaNode_(module_{"example-schema"}, container_("a2")));
                node.second = "a3";
            }

            REQUIRE(ys.isPresenceContainer(path, node));
        }
        SECTION("leafEnumHasValue")
        {
            std::string value;
            SECTION("leafEnum")
            {
                node.first = "example-schema";
                node.second = "leafEnum";

                SECTION("lol")
                    value = "lol";

                SECTION("data")
                    value = "data";

                SECTION("coze")
                    value = "coze";
            }

            SECTION("leafEnumTypedef")
            {
                node.first = "example-schema";
                node.second = "leafEnumTypedef";

                SECTION("lol")
                    value = "lol";

                SECTION("data")
                    value = "data";

                SECTION("coze")
                    value = "coze";
            }

            SECTION("leafEnumTypedefRestricted")
            {
                node.first = "example-schema";
                node.second = "leafEnumTypedefRestricted";

                SECTION("data")
                    value = "data";

                SECTION("coze")
                    value = "coze";
            }

            SECTION("leafEnumTypedefRestricted2")
            {
                node.first = "example-schema";
                node.second = "leafEnumTypedefRestricted2";

                SECTION("lol")
                    value = "lol";

                SECTION("data")
                    value = "data";
            }

            SECTION("pizzaSize")
            {
                node.first = "example-schema";
                node.second = "pizzaSize";

                SECTION("small")
                {
                    value = "small";

                }
                SECTION("medium")
                {
                    value = "medium";
                }

                SECTION("large")
                {
                    ys.enableFeature("example-schema", "bigPizzas");
                    value = "large";
                }
            }

            REQUIRE(ys.leafEnumHasValue(path, node, value));
        }
        SECTION("leafIdentityIsValid")
        {
            ModuleValuePair value;

            SECTION("foodIdentLeaf")
            {
                node.first = "example-schema";
                node.second = "foodIdentLeaf";

                SECTION("food")
                {
                    value.second = "food";
                }
                SECTION("example-schema:food")
                {
                    value.first = "example-schema";
                    value.second = "food";
                }
                SECTION("pizza")
                {
                    value.second = "pizza";
                }
                SECTION("example-schema:pizza")
                {
                    value.first = "example-schema";
                    value.second = "pizza";
                }
                SECTION("hawaii")
                {
                    value.second = "hawaii";
                }
                SECTION("example-schema:hawaii")
                {
                    value.first = "example-schema";
                    value.second = "hawaii";
                }
                SECTION("fruit")
                {
                    value.second = "fruit";
                }
                SECTION("example-schema:fruit")
                {
                    value.first = "example-schema";
                    value.second = "fruit";
                }
                SECTION("second-schema:pineapple")
                {
                    value.first = "second-schema";
                    value.second = "pineapple";
                }
            }

            SECTION("pizzaIdentLeaf")
            {
                node.first = "example-schema";
                node.second = "pizzaIdentLeaf";

                SECTION("pizza")
                {
                    value.second = "pizza";
                }
                SECTION("example-schema:pizza")
                {
                    value.first = "example-schema";
                    value.second = "pizza";
                }
                SECTION("hawaii")
                {
                    value.second = "hawaii";
                }
                SECTION("example-schema:hawaii")
                {
                    value.first = "example-schema";
                    value.second = "hawaii";
                }
            }

            SECTION("foodDrinkIdentLeaf")
            {
                node.first = "example-schema";
                node.second = "foodDrinkIdentLeaf";

                SECTION("food")
                {
                    value.second = "food";
                }
                SECTION("example-schema:food")
                {
                    value.first = "example-schema";
                    value.second = "food";
                }
                SECTION("drink")
                {
                    value.second = "drink";
                }
                SECTION("example-schema:drink")
                {
                    value.first = "example-schema";
                    value.second = "drink";
                }
            }
            REQUIRE(ys.leafIdentityIsValid(path, node, value));
        }

        SECTION("listHasKey")
        {
            std::string key;

            SECTION("_list")
            {
                node.first = "example-schema";
                node.second = "_list";
                SECTION("number")
                key = "number";
            }

            SECTION("twoKeyList")
            {
                node.first = "example-schema";
                node.second = "twoKeyList";
                SECTION("number")
                key = "number";
                SECTION("name")
                key = "name";
            }

            REQUIRE(ys.listHasKey(path, node, key));
        }
        SECTION("listKeys")
        {
            std::set<std::string> set;

            SECTION("_list")
            {
                set = {"number"};
                node.first = "example-schema";
                node.second = "_list";
            }

            SECTION("twoKeyList")
            {
                set = {"number", "name"};
                node.first = "example-schema";
                node.second = "twoKeyList";
            }

            REQUIRE(ys.listKeys(path, node) == set);
        }
        SECTION("leafType")
        {
            yang::LeafDataTypes type;

            SECTION("leafString")
            {
                node.first = "example-schema";
                node.second = "leafString";
                type = yang::LeafDataTypes::String;
            }

            SECTION("leafDecimal")
            {
                node.first = "example-schema";
                node.second = "leafDecimal";
                type = yang::LeafDataTypes::Decimal;
            }

            SECTION("leafBool")
            {
                node.first = "example-schema";
                node.second = "leafBool";
                type = yang::LeafDataTypes::Bool;
            }

            SECTION("leafInt8")
            {
                node.first = "example-schema";
                node.second = "leafInt8";
                type = yang::LeafDataTypes::Int8;
            }

            SECTION("leafUint8")
            {
                node.first = "example-schema";
                node.second = "leafUint8";
                type = yang::LeafDataTypes::Uint8;
            }

            SECTION("leafInt15")
            {
                node.first = "example-schema";
                node.second = "leafInt16";
                type = yang::LeafDataTypes::Int16;
            }

            SECTION("leafUint16")
            {
                node.first = "example-schema";
                node.second = "leafUint16";
                type = yang::LeafDataTypes::Uint16;
            }

            SECTION("leafInt32")
            {
                node.first = "example-schema";
                node.second = "leafInt32";
                type = yang::LeafDataTypes::Int32;
            }

            SECTION("leafUint32")
            {
                node.first = "example-schema";
                node.second = "leafUint32";
                type = yang::LeafDataTypes::Uint32;
            }

            SECTION("leafInt64")
            {
                node.first = "example-schema";
                node.second = "leafInt64";
                type = yang::LeafDataTypes::Int64;
            }

            SECTION("leafUint64")
            {
                node.first = "example-schema";
                node.second = "leafUint64";
                type = yang::LeafDataTypes::Uint64;
            }

            SECTION("leafEnum")
            {
                node.first = "example-schema";
                node.second = "leafEnum";
                type = yang::LeafDataTypes::Enum;
            }

            REQUIRE(ys.leafType(path, node) == type);
        }
        SECTION("childNodes")
        {
            std::set<std::string> set;

            SECTION("<root>")
            {
                set = {"example-schema:a", "example-schema:b", "example-schema:leafString",
                       "example-schema:leafDecimal", "example-schema:leafBool",
                       "example-schema:leafInt8", "example-schema:leafUint8",
                       "example-schema:leafInt16", "example-schema:leafUint16",
                       "example-schema:leafInt32", "example-schema:leafUint32",
                       "example-schema:leafInt64", "example-schema:leafUint64",
                       "example-schema:leafEnum", "example-schema:leafEnumTypedef",
                       "example-schema:leafEnumTypedefRestricted", "example-schema:leafEnumTypedefRestricted2",
                       "example-schema:foodIdentLeaf", "example-schema:pizzaIdentLeaf", "example-schema:foodDrinkIdentLeaf",
                       "example-schema:_list", "example-schema:twoKeyList", "second-schema:bla",
                       "example-schema:carry", "example-schema:zero", "example-schema:direction",
                       "example-schema:interrupt",
                       "example-schema:ethernet", "example-schema:loopback",
                       "example-schema:pizzaSize"};
            }

            SECTION("example-schema:a")
            {
                path.m_nodes.push_back(schemaNode_(module_{"example-schema"}, container_("a")));
                set = {"a2", "leafa", "second-schema:augmentedContainer"};
            }

            SECTION("second-schema:bla")
            {
                path.m_nodes.push_back(schemaNode_(module_{"second-schema"}, container_("bla")));
                set = {"bla2"};
            }

            REQUIRE(ys.childNodes(path, Recursion::NonRecursive) == set);
        }
    }

    SECTION("negative")
    {
        SECTION("nonexistent nodes")
        {
            SECTION("example-schema:coze")
            {
                node.first = "example-schema";
                node.second = "coze";
            }

            SECTION("example-schema:a/nevim")
            {
                path.m_nodes.push_back(schemaNode_(module_{"example-schema"}, container_("a")));
                node.second = "nevim";
            }

            SECTION("modul:a/nevim")
            {
                path.m_nodes.push_back(schemaNode_(module_{"modul"}, container_("a")));
                node.second = "nevim";
            }

            REQUIRE(!ys.isPresenceContainer(path, node));
            REQUIRE(!ys.isList(path, node));
            REQUIRE(!ys.isLeaf(path, node));
            REQUIRE(!ys.isContainer(path, node));
        }

        SECTION("\"is\" methods return false for existing nodes for different nodetypes")
        {
            SECTION("example-schema:a")
            {
                node.first = "example-schema";
                node.second = "a";
            }

            SECTION("example-schema:a/a2")
            {
                path.m_nodes.push_back(schemaNode_(module_{"example-schema"}, container_("a")));
                node.second = "a2";
            }

            REQUIRE(!ys.isPresenceContainer(path, node));
            REQUIRE(!ys.isList(path, node));
            REQUIRE(!ys.isLeaf(path, node));
        }

        SECTION("nodetype-specific methods called with different nodetypes")
        {
            path.m_nodes.push_back(schemaNode_(module_{"example-schema"}, container_("a")));
            node.second = "a2";

            REQUIRE(!ys.leafEnumHasValue(path, node, "haha"));
            REQUIRE(!ys.listHasKey(path, node, "chacha"));
        }

        SECTION("nonexistent module")
        {
            REQUIRE(!ys.isModule(path, "notAModule"));
        }

        SECTION("leafIdentityIsValid")
        {
            ModuleValuePair value;
            SECTION("pizzaIdentLeaf")
            {
                node.first = "example-schema";
                node.second = "pizzaIdentLeaf";

                SECTION("wrong base ident")
                {
                    SECTION("food")
                    {
                        value.second = "food";
                    }
                    SECTION("fruit")
                    {
                        value.second = "fruit";
                    }
                }
                SECTION("non-existent identity")
                {
                    value.second = "nonexistent";
                }
                SECTION("weird module")
                {
                    value.first = "ahahaha";
                    value.second = "pizza";
                }
            }
            SECTION("different module identity, but withotu prefix")
            {
                node.first = "example-schema";
                node.second = "foodIdentLeaf";
                value.second = "pineapple";
            }
            REQUIRE_FALSE(ys.leafIdentityIsValid(path, node, value));
        }

        SECTION("grouping is not a node")
        {
            SECTION("example-schema:arithmeticFlags")
            {
                node.first = "example-schema";
                node.second = "arithmeticFlags";
            }

            SECTION("example-schema:flags")
            {
                node.first = "example-schema";
                node.second = "startAndStop";
            }

            REQUIRE(!ys.isPresenceContainer(path, node));
            REQUIRE(!ys.isList(path, node));
            REQUIRE(!ys.isLeaf(path, node));
            REQUIRE(!ys.isContainer(path, node));
        }

        SECTION("choice is not a node")
        {
            SECTION("example-schema:interface")
            {
                node.first = "example-schema";
                node.second = "interface";
            }

            REQUIRE(!ys.isPresenceContainer(path, node));
            REQUIRE(!ys.isList(path, node));
            REQUIRE(!ys.isLeaf(path, node));
            REQUIRE(!ys.isContainer(path, node));
        }

        SECTION("case is not a node")
        {
            SECTION("example-schema:caseLoopback")
            {
                node.first = "example-schema";
                node.second = "caseLoopback";
            }

            SECTION("example-schema:caseEthernet")
            {
                node.first = "example-schema";
                node.second = "caseEthernet";
            }

            REQUIRE(!ys.isPresenceContainer(path, node));
            REQUIRE(!ys.isList(path, node));
            REQUIRE(!ys.isLeaf(path, node));
            REQUIRE(!ys.isContainer(path, node));
        }

        SECTION("enum is disabled by if-feature if feature is not enabled")
        {
            node.first = "example-schema";
            node.second = "pizzaSize";

            std::string value = "large";

            REQUIRE(!ys.leafEnumHasValue(path, node, value));
        }
    }
}
