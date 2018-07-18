/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/

#include "trompeloeil_catch.h"
#include "yang_schema.hpp"

const char* schema = R"(
module example-schema {
    namespace "http://example.com/example-sports";
    prefix coze;

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

    leaf leafInt {
        type int32;
    }

    leaf leafUint {
        type uint32;
    }

    leaf leafEnum {
        type enumeration {
            enum lol;
            enum data;
            enum coze;
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
})";

TEST_CASE("yangschema")
{
    YangSchema ys;
    ys.addSchemaString(schema);
    path_ path;
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
                path.m_nodes.push_back(node_(module_{"example-schema"}, container_("a")));
                node.second = "a2";
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
                path.m_nodes.push_back(node_(module_{"example-schema"}, container_("a")));
                node.first = "example-schema";
                node.second = "leafa";
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
                path.m_nodes.push_back(node_(module_{"example-schema"}, container_("a")));
                path.m_nodes.push_back(node_(module_{"example-schema"}, container_("a2")));
                node.second = "a3";
            }

            REQUIRE(ys.isPresenceContainer(path, node));
        }
        SECTION("leafEnumHasValue")
        {
            node.first = "example-schema";
            node.second = "leafEnum";
            std::string value;

            SECTION("lol")
            value = "lol";

            SECTION("data")
            value = "data";

            SECTION("coze")
            value = "coze";

            REQUIRE(ys.leafEnumHasValue(path, node, value));
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

            SECTION("leafInt")
            {
                node.first = "example-schema";
                node.second = "leafInt";
                type = yang::LeafDataTypes::Int;
            }

            SECTION("leafUint")
            {
                node.first = "example-schema";
                node.second = "leafUint";
                type = yang::LeafDataTypes::Uint;
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
                       "example-schema:leafDecimal", "example-schema:leafBool", "example-schema:leafInt",
                       "example-schema:leafUint", "example-schema:leafEnum", "example-schema:_list", "example-schema:twoKeyList"};
            }

            SECTION("a")
            {
                path.m_nodes.push_back(node_(module_{"example-schema"}, container_("a")));
                set = {"example-schema:a2", "example-schema:leafa"};
            }

            REQUIRE(ys.childNodes(path) == set);
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
                path.m_nodes.push_back(node_(module_{"example-schema"}, container_("a")));
                node.second = "nevim";
            }

            SECTION("modul:a/nevim")
            {
                path.m_nodes.push_back(node_(module_{"modul"}, container_("a")));
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
                path.m_nodes.push_back(node_(module_{"example-schema"}, container_("a")));
                node.second = "a2";
            }

            REQUIRE(!ys.isPresenceContainer(path, node));
            REQUIRE(!ys.isList(path, node));
            REQUIRE(!ys.isLeaf(path, node));
        }

        SECTION("nodetype-specific methods called with different nodetypes")
        {
            path.m_nodes.push_back(node_(module_{"example-schema"}, container_("a")));
            node.second = "a2";

            REQUIRE(!ys.leafEnumHasValue(path, node, "haha"));
            REQUIRE(!ys.listHasKey(path, node, "chacha"));
        }

        SECTION("nonexistent module")
        {
            REQUIRE(!ys.isModule(path, "notAModule"));
        }
    }
}
