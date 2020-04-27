/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/

#include <experimental/iterator>
#include "leaf_data_helpers.hpp"
#include "pretty_printers.hpp"
#include "trompeloeil_doctest.hpp"
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
        description "A 32-bit integer leaf.";
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
        default 9001;
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
        default data;
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

    leaf length {
        type int32;
        units "m";
    }

    leaf wavelength {
        type decimal64 {
            fraction-digits 10;
        }
        units "nm";
    }

    typedef seconds {
        type int32;
        units "s";
    }

    leaf duration {
        type seconds;
    }

    leaf another-duration {
        type seconds;
        units "vt";
    }

    leaf activeNumber {
        type leafref {
            path "/_list/number";
        }
    }

    rpc myRpc {}

    leaf numberOrString {
        type union {
            type int32;
            type string;
        }
        description "Can be an int32 or a string.";
    }

    list portSettings {
        key "port";
        leaf port {
            type enumeration {
                enum eth0;
                enum eth1;
                enum eth2;
            }
        }
    }

    feature weirdPortNames;

    list portMapping {
        key "port";
        leaf port {
            type enumeration {
                enum WEIRD {
                    if-feature "weirdPortNames";
                }
                enum utf2;
                enum utf3;
            }
        }
    }

    leaf activeMappedPort {
        type leafref {
            path "../portMapping/port";
        }
    }

    leaf activePort {
        type union {
            type enumeration {
                enum wlan0;
                enum wlan1;
            }
            type leafref {
                path "../portSettings/port";
            }
            type leafref {
                path "../activeMappedPort";
            }
        }
    }

    leaf clockSpeed {
        type int64;
        config false;
    }

    container systemStats {
        config false;
        leaf upTime {
            type uint64;
        }
    }

    leaf obsoleteLeaf {
        type int32;
        status obsolete;
    }

    leaf deprecatedLeaf {
        type int32;
        status deprecated;
    }

    typedef deprecatedType {
        type int32;
        status deprecated;
    }

    leaf obsoleteLeafWithDeprecatedType {
        type deprecatedType;
        status obsolete;
    }

    typedef obsoleteType {
        type int32;
        status obsolete;
    }

    leaf obsoleteLeafWithObsoleteType {
        type deprecatedType;
        status obsolete;
    }

})";

namespace std {
std::ostream& operator<<(std::ostream& s, const std::set<std::string> set)
{
    s << std::endl << "{";
    std::copy(set.begin(), set.end(), std::experimental::make_ostream_joiner(s, ", "));
    s << "}" << std::endl;
    return s;
}
}

TEST_CASE("yangschema")
{
    using namespace std::string_view_literals;
    YangSchema ys;
    ys.registerModuleCallback([]([[maybe_unused]] auto modName, auto, auto, auto) {
        assert("example-schema"sv == modName);
        return example_schema;
    });
    ys.addSchemaString(second_schema);

    schemaPath_ path{Scope::Absolute, {}};
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
            REQUIRE(ys.isModule("example-schema"));
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
            yang::LeafDataType type;

            SECTION("leafString")
            {
                node.first = "example-schema";
                node.second = "leafString";
                type = yang::String{};
            }

            SECTION("leafDecimal")
            {
                node.first = "example-schema";
                node.second = "leafDecimal";
                type = yang::Decimal{};
            }

            SECTION("leafBool")
            {
                node.first = "example-schema";
                node.second = "leafBool";
                type = yang::Bool{};
            }

            SECTION("leafInt8")
            {
                node.first = "example-schema";
                node.second = "leafInt8";
                type = yang::Int8{};
            }

            SECTION("leafUint8")
            {
                node.first = "example-schema";
                node.second = "leafUint8";
                type = yang::Uint8{};
            }

            SECTION("leafInt16")
            {
                node.first = "example-schema";
                node.second = "leafInt16";
                type = yang::Int16{};
            }

            SECTION("leafUint16")
            {
                node.first = "example-schema";
                node.second = "leafUint16";
                type = yang::Uint16{};
            }

            SECTION("leafInt32")
            {
                node.first = "example-schema";
                node.second = "leafInt32";
                type = yang::Int32{};
            }

            SECTION("leafUint32")
            {
                node.first = "example-schema";
                node.second = "leafUint32";
                type = yang::Uint32{};
            }

            SECTION("leafInt64")
            {
                node.first = "example-schema";
                node.second = "leafInt64";
                type = yang::Int64{};
            }

            SECTION("leafUint64")
            {
                node.first = "example-schema";
                node.second = "leafUint64";
                type = yang::Uint64{};
            }

            SECTION("leafEnum")
            {
                node.first = "example-schema";
                node.second = "leafEnum";
                type = createEnum({"lol", "data", "coze"});
            }

            SECTION("leafEnumTypedef")
            {
                node.first = "example-schema";
                node.second = "leafEnumTypedef";
                type = createEnum({"lol", "data", "coze"});
            }

            SECTION("leafEnumTypedefRestricted")
            {
                node.first = "example-schema";
                node.second = "leafEnumTypedefRestricted";
                type = createEnum({"data", "coze"});
            }

            SECTION("leafEnumTypedefRestricted2")
            {
                node.first = "example-schema";
                node.second = "leafEnumTypedefRestricted2";
                type = createEnum({"lol", "data"});
            }

            SECTION("pizzaSize")
            {
                node.first = "example-schema";
                node.second = "pizzaSize";

                SECTION("bigPizzas disabled")
                {
                    type = createEnum({"small", "medium"});
                }
                SECTION("bigPizzas enabled")
                {
                    ys.enableFeature("example-schema", "bigPizzas");
                    type = createEnum({"small", "medium", "large"});
                }
            }

            SECTION("foodIdentLeaf")
            {
                node.first = "example-schema";
                node.second = "foodIdentLeaf";
                type = yang::IdentityRef{{{"second-schema", "pineapple"},
                                          {"example-schema", "food"},
                                          {"example-schema", "pizza"},
                                          {"example-schema", "hawaii"},
                                          {"example-schema", "fruit"}}};
            }

            SECTION("pizzaIdentLeaf")
            {
                node.first = "example-schema";
                node.second = "pizzaIdentLeaf";

                type = yang::IdentityRef{{
                    {"example-schema", "pizza"},
                    {"example-schema", "hawaii"},
                }};
            }

            SECTION("foodDrinkIdentLeaf")
            {
                node.first = "example-schema";
                node.second = "foodDrinkIdentLeaf";

                type = yang::IdentityRef{{
                    {"example-schema", "food"},
                    {"example-schema", "drink"},
                    {"example-schema", "fruit"},
                    {"example-schema", "hawaii"},
                    {"example-schema", "pizza"},
                    {"example-schema", "voda"},
                    {"second-schema", "pineapple"},
                }};
            }

            SECTION("activeNumber")
            {
                node.first = "example-schema";
                node.second = "activeNumber";
                type.emplace<yang::LeafRef>(
                    "/example-schema:_list/number",
                    std::make_unique<yang::TypeInfo>(ys.leafType("/example-schema:_list/number"))
                );
            }

            SECTION("activePort")
            {
                node.first = "example-schema";
                node.second = "activePort";

                yang::Enum enums = [&ys]() {
                    SECTION("weird ports disabled")
                    {
                        return createEnum({"utf2", "utf3"});
                    }
                    SECTION("weird ports enabled")
                    {
                        ys.enableFeature("example-schema", "weirdPortNames");
                        return createEnum({"WEIRD", "utf2", "utf3"});
                    }
                    __builtin_unreachable();
                }();

                type = yang::Union{{
                    yang::TypeInfo{createEnum({"wlan0", "wlan1"})},
                    yang::TypeInfo{yang::LeafRef{
                        "/example-schema:portSettings/port",
                        std::make_unique<yang::TypeInfo>(createEnum({"eth0", "eth1", "eth2"}))
                    }},
                    yang::TypeInfo{yang::LeafRef{
                        "/example-schema:activeMappedPort",
                        std::make_unique<yang::TypeInfo>(yang::LeafRef{
                                "/example-schema:portMapping/port",
                                std::make_unique<yang::TypeInfo>(enums)
                        })
                    }},
                }};
            }

            REQUIRE(ys.leafType(path, node) == type);
        }
        SECTION("childNodes")
        {
            std::set<ModuleNodePair> set;

            using namespace std::string_literals;
            SECTION("<root>")
            {
                set = {{"example-schema"s, "a"}, {"example-schema"s, "b"}, {"example-schema"s, "leafString"},
                       {"example-schema"s, "leafDecimal"}, {"example-schema"s, "leafBool"},
                       {"example-schema"s, "leafInt8"}, {"example-schema"s, "leafUint8"},
                       {"example-schema"s, "leafInt16"}, {"example-schema"s, "leafUint16"},
                       {"example-schema"s, "leafInt32"}, {"example-schema"s, "leafUint32"},
                       {"example-schema"s, "leafInt64"}, {"example-schema"s, "leafUint64"},
                       {"example-schema"s, "leafEnum"}, {"example-schema"s, "leafEnumTypedef"},
                       {"example-schema"s, "leafEnumTypedefRestricted"}, {"example-schema"s, "leafEnumTypedefRestricted2"},
                       {"example-schema"s, "foodIdentLeaf"}, {"example-schema"s, "pizzaIdentLeaf"}, {"example-schema"s, "foodDrinkIdentLeaf"},
                       {"example-schema"s, "_list"}, {"example-schema"s, "twoKeyList"}, {"second-schema"s, "bla"},
                       {"example-schema"s, "carry"}, {"example-schema"s, "zero"}, {"example-schema"s, "direction"},
                       {"example-schema"s, "interrupt"},
                       {"example-schema"s, "ethernet"}, {"example-schema"s, "loopback"},
                       {"example-schema"s, "pizzaSize"},
                       {"example-schema"s, "length"}, {"example-schema"s, "wavelength"},
                       {"example-schema"s, "duration"}, {"example-schema"s, "another-duration"},
                       {"example-schema"s, "activeNumber"},
                       {"example-schema"s, "numberOrString"},
                       {"example-schema"s, "portSettings"},
                       {"example-schema"s, "portMapping"},
                       {"example-schema"s, "activeMappedPort"},
                       {"example-schema"s, "activePort"},
                       {"example-schema"s, "clockSpeed"},
                       {"example-schema"s, "deprecatedLeaf"},
                       {"example-schema"s, "obsoleteLeaf"},
                       {"example-schema"s, "obsoleteLeafWithDeprecatedType"},
                       {"example-schema"s, "obsoleteLeafWithObsoleteType"},
                       {"example-schema"s, "systemStats"}};
            }

            SECTION("example-schema:a")
            {
                path.m_nodes.push_back(schemaNode_(module_{"example-schema"}, container_("a")));
                set = {
                    {boost::none, "a2"},
                    {boost::none, "leafa"},
                    {"second-schema"s, "augmentedContainer"}
                };
            }

            SECTION("second-schema:bla")
            {
                path.m_nodes.push_back(schemaNode_(module_{"second-schema"}, container_("bla")));
                set = {{boost::none, "bla2"}};
            }

            SECTION("example-schema:ethernet")
            {
                path.m_nodes.push_back(schemaNode_(module_{"example-schema"}, container_("ethernet")));
                set = {{boost::none, "ip"}};
            }

            REQUIRE(ys.childNodes(path, Recursion::NonRecursive) == set);
        }
        SECTION("nodeType")
        {
            yang::NodeTypes expected;
            SECTION("leafInt32")
            {
                path.m_nodes.push_back(schemaNode_(module_{"example-schema"}, leaf_("leafInt32")));
                expected = yang::NodeTypes::Leaf;
            }

            SECTION("a")
            {
                path.m_nodes.push_back(schemaNode_(module_{"example-schema"}, container_("a")));
                expected = yang::NodeTypes::Container;
            }

            SECTION("a/a2/a3")
            {
                path.m_nodes.push_back(schemaNode_(module_{"example-schema"}, container_("a")));
                path.m_nodes.push_back(schemaNode_(container_("a2")));
                path.m_nodes.push_back(schemaNode_(container_("a3")));
                expected = yang::NodeTypes::PresenceContainer;
            }

            SECTION("_list")
            {
                path.m_nodes.push_back(schemaNode_(module_{"example-schema"}, list_("_list")));
                expected = yang::NodeTypes::List;
            }

            REQUIRE(ys.nodeType(pathToSchemaString(path, Prefixes::WhenNeeded)) == expected);
        }

        SECTION("description")
        {
            std::optional<std::string> expected;
            SECTION("leafInt32")
            {
                path.m_nodes.push_back(schemaNode_(module_{"example-schema"}, leaf_("leafInt32")));
                expected = "A 32-bit integer leaf.";
            }

            SECTION("leafString")
            {
                path.m_nodes.push_back(schemaNode_(module_{"example-schema"}, leaf_("leafString")));
            }

            SECTION("numberOrString")
            {
                path.m_nodes.push_back(schemaNode_(module_{"example-schema"}, leaf_("numberOrString")));
                expected = "Can be an int32 or a string.";
            }

            REQUIRE(ys.description(pathToSchemaString(path, Prefixes::WhenNeeded)) == expected);
        }

        SECTION("status")
        {
            REQUIRE(ys.status("/example-schema:leafUint64") == yang::Status::Current);
            REQUIRE(ys.status("/example-schema:obsoleteLeaf") == yang::Status::Obsolete);
            REQUIRE(ys.status("/example-schema:deprecatedLeaf") == yang::Status::Deprecated);
            REQUIRE(ys.status("/example-schema:obsoleteLeafWithDeprecatedType") == yang::Status::Obsolete);
            REQUIRE(ys.status("/example-schema:obsoleteLeafWithObsoleteType") == yang::Status::Obsolete);
        }

        SECTION("units")
        {
            yang::LeafDataType expectedType;
            std::optional<std::string> expectedUnits;
            SECTION("length")
            {
                path.m_nodes.push_back(schemaNode_(module_{"example-schema"}, leaf_("length")));
                expectedType.emplace<yang::Int32>();
                expectedUnits = "m";
            }

            SECTION("wavelength")
            {
                path.m_nodes.push_back(schemaNode_(module_{"example-schema"}, leaf_("wavelength")));
                expectedType.emplace<yang::Decimal>();
                expectedUnits = "nm";
            }

            SECTION("leafInt32")
            {
                path.m_nodes.push_back(schemaNode_(module_{"example-schema"}, leaf_("leafInt32")));
                expectedType.emplace<yang::Int32>();
            }

            SECTION("duration")
            {
                path.m_nodes.push_back(schemaNode_(module_{"example-schema"}, leaf_("duration")));
                expectedType.emplace<yang::Int32>();
                expectedUnits = "s";
            }

            SECTION("another-duration")
            {
                path.m_nodes.push_back(schemaNode_(module_{"example-schema"}, leaf_("another-duration")));
                expectedType.emplace<yang::Int32>();
                expectedUnits = "vt";
            }
            REQUIRE(ys.leafType(pathToSchemaString(path, Prefixes::WhenNeeded)) == yang::TypeInfo{expectedType, expectedUnits});
        }

        SECTION("nodeType")
        {
            yang::NodeTypes expected;
            SECTION("leafInt32")
            {
                path.m_nodes.push_back(schemaNode_(module_{"example-schema"}, leaf_("leafInt32")));
                expected = yang::NodeTypes::Leaf;
            }

            SECTION("a")
            {
                path.m_nodes.push_back(schemaNode_(module_{"example-schema"}, container_("a")));
                expected = yang::NodeTypes::Container;
            }

            SECTION("a/a2/a3")
            {
                path.m_nodes.push_back(schemaNode_(module_{"example-schema"}, container_("a")));
                path.m_nodes.push_back(schemaNode_(container_("a2")));
                path.m_nodes.push_back(schemaNode_(container_("a3")));
                expected = yang::NodeTypes::PresenceContainer;
            }

            SECTION("_list")
            {
                path.m_nodes.push_back(schemaNode_(module_{"example-schema"}, list_("_list")));
                expected = yang::NodeTypes::List;
            }

            REQUIRE(ys.nodeType(pathToSchemaString(path, Prefixes::WhenNeeded)) == expected);
        }

        SECTION("leafrefPath")
        {
            REQUIRE(ys.leafrefPath("/example-schema:activeNumber") == "/example-schema:_list/number");
        }

        SECTION("isConfig")
        {
            REQUIRE(ys.isConfig("/example-schema:leafInt32"));
            REQUIRE_FALSE(ys.isConfig("/example-schema:clockSpeed"));
            REQUIRE_FALSE(ys.isConfig("/example-schema:systemStats"));
            REQUIRE_FALSE(ys.isConfig("/example-schema:systemStats/upTime"));
        }

        SECTION("defaultValue")
        {
            REQUIRE(ys.defaultValue("/example-schema:leafUint64") == "9001");
            REQUIRE(ys.defaultValue("/example-schema:leafEnumTypedefRestricted") == "data");
            REQUIRE(ys.defaultValue("/example-schema:leafInt32") == std::nullopt);
        }

        SECTION("moduleNodes")
        {
            std::string module;
            std::set<std::string> expectedNonRecursive;
            std::set<std::string> expectedRecursive;
            SECTION("example-schema")
            {
                module = "example-schema";
                expectedNonRecursive = {
                    "example-schema:_list",
                    "example-schema:a",
                    "example-schema:activeMappedPort",
                    "example-schema:activeNumber",
                    "example-schema:activePort",
                    "example-schema:another-duration",
                    "example-schema:b",
                    "example-schema:carry",
                    "example-schema:clockSpeed",
                    "example-schema:deprecatedLeaf",
                    "example-schema:direction",
                    "example-schema:duration",
                    "example-schema:ethernet",
                    "example-schema:foodDrinkIdentLeaf",
                    "example-schema:foodIdentLeaf",
                    "example-schema:interrupt",
                    "example-schema:leafBool",
                    "example-schema:leafDecimal",
                    "example-schema:leafEnum",
                    "example-schema:leafEnumTypedef",
                    "example-schema:leafEnumTypedefRestricted",
                    "example-schema:leafEnumTypedefRestricted2",
                    "example-schema:leafInt16",
                    "example-schema:leafInt32",
                    "example-schema:leafInt64",
                    "example-schema:leafInt8",
                    "example-schema:leafString",
                    "example-schema:leafUint16",
                    "example-schema:leafUint32",
                    "example-schema:leafUint64",
                    "example-schema:leafUint8",
                    "example-schema:length",
                    "example-schema:loopback",
                    "example-schema:myRpc",
                    "example-schema:numberOrString",
                    "example-schema:obsoleteLeaf",
                    "example-schema:obsoleteLeafWithDeprecatedType",
                    "example-schema:obsoleteLeafWithObsoleteType",
                    "example-schema:pizzaIdentLeaf",
                    "example-schema:pizzaSize",
                    "example-schema:portMapping",
                    "example-schema:portSettings",
                    "example-schema:systemStats",
                    "example-schema:twoKeyList",
                    "example-schema:wavelength",
                    "example-schema:zero"
                };
                expectedRecursive = {
                    "/example-schema:_list",
                    "/example-schema:_list/contInList",
                    "/example-schema:_list/number",
                    "/example-schema:a",
                    "/example-schema:a/a2",
                    "/example-schema:a/a2/a3",
                    "/example-schema:a/leafa",
                    "/example-schema:a/second-schema:augmentedContainer",
                    "/example-schema:activeMappedPort",
                    "/example-schema:activeNumber",
                    "/example-schema:activePort",
                    "/example-schema:another-duration",
                    "/example-schema:b",
                    "/example-schema:b/b2",
                    "/example-schema:b/b2/b3",
                    "/example-schema:carry",
                    "/example-schema:clockSpeed",
                    "/example-schema:deprecatedLeaf",
                    "/example-schema:direction",
                    "/example-schema:duration",
                    "/example-schema:foodDrinkIdentLeaf",
                    "/example-schema:foodIdentLeaf",
                    "/example-schema:interface/caseEthernet/ethernet",
                    "/example-schema:interface/caseEthernet/ethernet/ip",
                    "/example-schema:interface/caseLoopback/loopback",
                    "/example-schema:interface/caseLoopback/loopback/ip",
                    "/example-schema:interrupt",
                    "/example-schema:leafBool",
                    "/example-schema:leafDecimal",
                    "/example-schema:leafEnum",
                    "/example-schema:leafEnumTypedef",
                    "/example-schema:leafEnumTypedefRestricted",
                    "/example-schema:leafEnumTypedefRestricted2",
                    "/example-schema:leafInt16",
                    "/example-schema:leafInt32",
                    "/example-schema:leafInt64",
                    "/example-schema:leafInt8",
                    "/example-schema:leafString",
                    "/example-schema:leafUint16",
                    "/example-schema:leafUint32",
                    "/example-schema:leafUint64",
                    "/example-schema:leafUint8",
                    "/example-schema:length",
                    "/example-schema:myRpc",
                    "/example-schema:myRpc/input",
                    "/example-schema:myRpc/output",
                    "/example-schema:numberOrString",
                    "/example-schema:obsoleteLeaf",
                    "/example-schema:obsoleteLeafWithDeprecatedType",
                    "/example-schema:obsoleteLeafWithObsoleteType",
                    "/example-schema:pizzaIdentLeaf",
                    "/example-schema:pizzaSize",
                    "/example-schema:portMapping",
                    "/example-schema:portMapping/port",
                    "/example-schema:portSettings",
                    "/example-schema:portSettings/port",
                    "/example-schema:systemStats",
                    "/example-schema:systemStats/upTime",
                    "/example-schema:twoKeyList",
                    "/example-schema:twoKeyList/name",
                    "/example-schema:twoKeyList/number",
                    "/example-schema:wavelength",
                    "/example-schema:zero"
                };
            }

            SECTION("second-schema")
            {
                module = "second-schema";
                expectedNonRecursive = {
                    "second-schema:bla"
                };
                expectedRecursive = {
                    "/second-schema:bla", "/second-schema:bla/bla2"
                };
            }

            REQUIRE(ys.moduleNodes(module_{module}, Recursion::NonRecursive) == expectedNonRecursive);
            REQUIRE(ys.moduleNodes(module_{module}, Recursion::Recursive) == expectedRecursive);
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

            REQUIRE(!ys.listHasKey(path, node, "chacha"));
        }

        SECTION("nonexistent module")
        {
            REQUIRE(!ys.isModule("notAModule"));
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
    }
}
