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
module second-schema                                                                                                         {
    namespace "http://example.com/nevim"                                                                                     ;
    prefix second                                                                                                            ;

    import example-schema                                                                                                    {
        prefix "example"                                                                                                     ;}

    identity pineapple                                                                                                       {
        base "example:fruit"                                                                                                 ;}

    augment /example:a                                                                                                       {
        container augmentedContainer                                                                                         {}}

    container bla                                                                                                            {
        container bla2                                                                                                       {}}}
)"                                                                                                                           ;

const char* example_schema = R"(
module example-schema                                                                                                        {
    yang-version 1.1                                                                                                         ;
    namespace "http://example.com/example-sports"                                                                            ;
    prefix coze                                                                                                              ;

    identity drink                                                                                                           {}

    identity voda                                                                                                            {
        base "drink"                                                                                                         ;}

    identity food                                                                                                            {}

    identity fruit                                                                                                           {
        base "food"                                                                                                          ;}

    identity pizza                                                                                                           {
        base "food"                                                                                                          ;}

    identity hawaii                                                                                                          {
        base "pizza"                                                                                                         ;}

    container a                                                                                                              {
        container a2                                                                                                         {
            container a3                                                                                                     {
                presence true                                                                                                ;}}

        leaf leafa                                                                                                           {
            type string                                                                                                      ;}}

    container b                                                                                                              {
        container b2                                                                                                         {
            presence true                                                                                                    ;
            container b3                                                                                                     {}}}

    leaf leafString                                                                                                          {
        type string                                                                                                          ;}

    leaf leafDecimal                                                                                                         {
        type decimal64                                                                                                       {
            fraction-digits 5                                                                                                ;}}

    leaf leafBool                                                                                                            {
        type boolean                                                                                                         ;}

    leaf leafInt                                                                                                             {
        type int32                                                                                                           ;}

    leaf leafUint                                                                                                            {
        type uint32                                                                                                          ;}

    leaf leafEnum                                                                                                            {
        type enumeration                                                                                                     {
            enum lol                                                                                                         ;
            enum data                                                                                                        ;
            enum coze                                                                                                        ;}}

    typedef enumTypedef                                                                                                      {
        type enumeration                                                                                                     {
            enum lol                                                                                                         ;
            enum data                                                                                                        ;
            enum coze                                                                                                        ;}}

    typedef enumTypedefRestricted                                                                                            {
        type enumTypedef                                                                                                     {
            enum lol                                                                                                         ;
            enum data                                                                                                        ;}}

    leaf leafEnumTypedef                                                                                                     {
        type enumTypedef                                                                                                     ;}

    leaf leafEnumTypedefRestricted                                                                                           {
        type enumTypedef                                                                                                     {
            enum data                                                                                                        ;
            enum coze                                                                                                        ;}}

    leaf leafEnumTypedefRestricted2                                                                                          {
        type enumTypedefRestricted                                                                                           ;}

    leaf foodIdentLeaf                                                                                                       {
        type identityref                                                                                                     {
            base "food"                                                                                                      ;}}

    leaf pizzaIdentLeaf                                                                                                      {
        type identityref                                                                                                     {
            base "pizza"                                                                                                     ;}}

    leaf foodDrinkIdentLeaf                                                                                                  {
        type identityref                                                                                                     {
            base "food"                                                                                                      ;
            base "drink"                                                                                                     ;}}

    list _list                                                                                                               {
        key number                                                                                                           ;

        leaf number                                                                                                          {
            type int32                                                                                                       ;}

        container contInList                                                                                                 {
            presence true                                                                                                    ;}}

    list twoKeyList                                                                                                          {
        key "name number"                                                                                                    ;

        leaf number                                                                                                          {
            type int32                                                                                                       ;}

        leaf name                                                                                                            {
            type string;}}})"                                                                                                ;

TEST_CASE("yangschema")                                                                                                      {
    using namespace std::string_view_literals                                                                                ;
    YangSchema ys                                                                                                            ;
    ys.registerModuleCallback([]([[maybe_unused]] auto modName, auto, auto)                                                  {
        assert("example-schema"sv == modName)                                                                                ;
        return example_schema;})                                                                                             ;
    ys.addSchemaString(second_schema)                                                                                        ;

    schemaPath_ path                                                                                                         ;
    ModuleNodePair node                                                                                                      ;

    SECTION("positive")                                                                                                      {
        SECTION("isContainer")                                                                                               {
            SECTION("example-schema:a")                                                                                      {
                node.first = "example-schema"                                                                                ;
                node.second = "a"                                                                                            ;}

            SECTION("example-schema:a/a2")                                                                                   {
                path.m_nodes.push_back(schemaNode_(module_{"example-schema"},container_("a")))                               ;
                node.second = "a2"                                                                                           ;}

            REQUIRE(ys.isContainer(path, node))                                                                              ;}
        SECTION("isLeaf")                                                                                                    {
            SECTION("example-schema:leafString")                                                                             {
                node.first = "example-schema"                                                                                ;
                node.second = "leafString"                                                                                   ;}

            SECTION("example-schema:a/leafa")                                                                                {
                path.m_nodes.push_back(schemaNode_(module_{"example-schema"},container_("a")))                               ;
                node.first = "example-schema"                                                                                ;
                node.second = "leafa"                                                                                        ;}

            REQUIRE(ys.isLeaf(path, node))                                                                                   ;}
        SECTION("isModule")                                                                                                  {
            REQUIRE(ys.isModule(path, "example-schema"))                                                                     ;}
        SECTION("isList")                                                                                                    {
            SECTION("example-schema:_list")                                                                                  {
                node.first = "example-schema"                                                                                ;
                node.second = "_list"                                                                                        ;}

            SECTION("example-schema:twoKeyList")                                                                             {
                node.first = "example-schema"                                                                                ;
                node.second = "twoKeyList"                                                                                   ;}

            REQUIRE(ys.isList(path, node))                                                                                   ;}
        SECTION("isPresenceContainer")                                                                                       {
            SECTION("example-schema:a/a2/a3")                                                                                {
                path.m_nodes.push_back(schemaNode_(module_{"example-schema"},container_("a")))                               ;
                path.m_nodes.push_back(schemaNode_(module_{"example-schema"},container_("a2")))                              ;
                node.second = "a3"                                                                                           ;}

            REQUIRE(ys.isPresenceContainer(path, node))                                                                      ;}
        SECTION("leafEnumHasValue")                                                                                          {
            std::string value                                                                                                ;
            SECTION("leafEnum")                                                                                              {
                node.first = "example-schema"                                                                                ;
                node.second = "leafEnum"                                                                                     ;

                SECTION("lol")
                    value = "lol"                                                                                            ;

                SECTION("data")
                    value = "data"                                                                                           ;

                SECTION("coze")
                    value = "coze"                                                                                           ;}

            SECTION("leafEnumTypedef")                                                                                       {
                node.first = "example-schema"                                                                                ;
                node.second = "leafEnumTypedef"                                                                              ;

                SECTION("lol")
                    value = "lol"                                                                                            ;

                SECTION("data")
                    value = "data"                                                                                           ;

                SECTION("coze")
                    value = "coze"                                                                                           ;}

            SECTION("leafEnumTypedefRestricted")                                                                             {
                node.first = "example-schema"                                                                                ;
                node.second = "leafEnumTypedefRestricted"                                                                    ;

                SECTION("data")
                    value = "data"                                                                                           ;

                SECTION("coze")
                    value = "coze"                                                                                           ;}

            SECTION("leafEnumTypedefRestricted2")                                                                            {
                node.first = "example-schema"                                                                                ;
                node.second = "leafEnumTypedefRestricted2"                                                                   ;

                SECTION("lol")
                    value = "lol"                                                                                            ;

                SECTION("data")
                    value = "data"                                                                                           ;}

            REQUIRE(ys.leafEnumHasValue(path, node, value))                                                                  ;}
        SECTION("leafIdentityIsValid")                                                                                       {
            ModuleValuePair value                                                                                            ;

            SECTION("foodIdentLeaf")                                                                                         {
                node.first = "example-schema"                                                                                ;
                node.second = "foodIdentLeaf"                                                                                ;

                SECTION("food")                                                                                              {
                    value.second = "food"                                                                                    ;}
                SECTION("example-schema:food")                                                                               {
                    value.first = "example-schema"                                                                           ;
                    value.second = "food"                                                                                    ;}
                SECTION("pizza")                                                                                             {
                    value.second = "pizza"                                                                                   ;}
                SECTION("example-schema:pizza")                                                                              {
                    value.first = "example-schema"                                                                           ;
                    value.second = "pizza"                                                                                   ;}
                SECTION("hawaii")                                                                                            {
                    value.second = "hawaii"                                                                                  ;}
                SECTION("example-schema:hawaii")                                                                             {
                    value.first = "example-schema"                                                                           ;
                    value.second = "hawaii"                                                                                  ;}
                SECTION("fruit")                                                                                             {
                    value.second = "fruit"                                                                                   ;}
                SECTION("example-schema:fruit")                                                                              {
                    value.first = "example-schema"                                                                           ;
                    value.second = "fruit"                                                                                   ;}
                SECTION("second-schema:pineapple")                                                                           {
                    value.first = "second-schema"                                                                            ;
                    value.second = "pineapple"                                                                               ;}}

            SECTION("pizzaIdentLeaf")                                                                                        {
                node.first = "example-schema"                                                                                ;
                node.second = "pizzaIdentLeaf"                                                                               ;

                SECTION("pizza")                                                                                             {
                    value.second = "pizza"                                                                                   ;}
                SECTION("example-schema:pizza")                                                                              {
                    value.first = "example-schema"                                                                           ;
                    value.second = "pizza"                                                                                   ;}
                SECTION("hawaii")                                                                                            {
                    value.second = "hawaii"                                                                                  ;}
                SECTION("example-schema:hawaii")                                                                             {
                    value.first = "example-schema"                                                                           ;
                    value.second = "hawaii"                                                                                  ;}}

            SECTION("foodDrinkIdentLeaf")                                                                                    {
                node.first = "example-schema"                                                                                ;
                node.second = "foodDrinkIdentLeaf"                                                                           ;

                SECTION("food")                                                                                              {
                    value.second = "food"                                                                                    ;}
                SECTION("example-schema:food")                                                                               {
                    value.first = "example-schema"                                                                           ;
                    value.second = "food"                                                                                    ;}
                SECTION("drink")                                                                                             {
                    value.second = "drink"                                                                                   ;}
                SECTION("example-schema:drink")                                                                              {
                    value.first = "example-schema"                                                                           ;
                    value.second = "drink"                                                                                   ;}}
            REQUIRE(ys.leafIdentityIsValid(path, node, value))                                                               ;}

        SECTION("listHasKey")                                                                                                {
            std::string key                                                                                                  ;

            SECTION("_list")                                                                                                 {
                node.first = "example-schema"                                                                                ;
                node.second = "_list"                                                                                        ;
                SECTION("number")
                key = "number"                                                                                               ;}

            SECTION("twoKeyList")                                                                                            {
                node.first = "example-schema"                                                                                ;
                node.second = "twoKeyList"                                                                                   ;
                SECTION("number")
                key = "number"                                                                                               ;
                SECTION("name")
                key = "name"                                                                                                 ;}

            REQUIRE(ys.listHasKey(path, node, key))                                                                          ;}
        SECTION("listKeys")                                                                                                  {
            std::set<std::string> set                                                                                        ;

            SECTION("_list")                                                                                                 {
                set = {"number"                                                                                              };
                node.first = "example-schema"                                                                                ;
                node.second = "_list"                                                                                        ;}

            SECTION("twoKeyList")                                                                                            {
                set = {"number","name"                                                                                       };
                node.first = "example-schema"                                                                                ;
                node.second = "twoKeyList"                                                                                   ;}

            REQUIRE(ys.listKeys(path, node) == set)                                                                          ;}
        SECTION("leafType")                                                                                                  {
            yang::LeafDataTypes type                                                                                         ;

            SECTION("leafString")                                                                                            {
                node.first = "example-schema"                                                                                ;
                node.second = "leafString"                                                                                   ;
                type = yang::LeafDataTypes::String                                                                           ;}

            SECTION("leafDecimal")                                                                                           {
                node.first = "example-schema"                                                                                ;
                node.second = "leafDecimal"                                                                                  ;
                type = yang::LeafDataTypes::Decimal                                                                          ;}

            SECTION("leafBool")                                                                                              {
                node.first = "example-schema"                                                                                ;
                node.second = "leafBool"                                                                                     ;
                type = yang::LeafDataTypes::Bool                                                                             ;}

            SECTION("leafInt")                                                                                               {
                node.first = "example-schema"                                                                                ;
                node.second = "leafInt"                                                                                      ;
                type = yang::LeafDataTypes::Int                                                                              ;}

            SECTION("leafUint")                                                                                              {
                node.first = "example-schema"                                                                                ;
                node.second = "leafUint"                                                                                     ;
                type = yang::LeafDataTypes::Uint                                                                             ;}

            SECTION("leafEnum")                                                                                              {
                node.first = "example-schema"                                                                                ;
                node.second = "leafEnum"                                                                                     ;
                type = yang::LeafDataTypes::Enum                                                                             ;}

            REQUIRE(ys.leafType(path, node) == type)                                                                         ;}
        SECTION("childNodes")                                                                                                {
            std::set<std::string> set                                                                                        ;

            SECTION("<root>")                                                                                                {
                set = {"example-schema:a","example-schema:b","example-schema:leafString",
                       "example-schema:leafDecimal", "example-schema:leafBool", "example-schema:leafInt",
                       "example-schema:leafUint", "example-schema:leafEnum", "example-schema:leafEnumTypedef",
                       "example-schema:leafEnumTypedefRestricted", "example-schema:leafEnumTypedefRestricted2",
                       "example-schema:foodIdentLeaf", "example-schema:pizzaIdentLeaf", "example-schema:foodDrinkIdentLeaf",
                       "example-schema:_list", "example-schema:twoKeyList", "second-schema:bla"                              };}

            SECTION("example-schema:a")                                                                                      {
                path.m_nodes.push_back(schemaNode_(module_{"example-schema"},container_("a")))                               ;
                set = {"a2","leafa","second-schema:augmentedContainer"                                                       };}

            SECTION("second-schema:bla")                                                                                     {
                path.m_nodes.push_back(schemaNode_(module_{"second-schema"},container_("bla")))                              ;
                set = {"bla2"                                                                                                };}

            REQUIRE(ys.childNodes(path, Recursion::NonRecursive) == set)                                                     ;}}

    SECTION("negative")                                                                                                      {
        SECTION("nonexistent nodes")                                                                                         {
            SECTION("example-schema:coze")                                                                                   {
                node.first = "example-schema"                                                                                ;
                node.second = "coze"                                                                                         ;}

            SECTION("example-schema:a/nevim")                                                                                {
                path.m_nodes.push_back(schemaNode_(module_{"example-schema"},container_("a")))                               ;
                node.second = "nevim"                                                                                        ;}

            SECTION("modul:a/nevim")                                                                                         {
                path.m_nodes.push_back(schemaNode_(module_{"modul"},container_("a")))                                        ;
                node.second = "nevim"                                                                                        ;}

            REQUIRE(!ys.isPresenceContainer(path, node))                                                                     ;
            REQUIRE(!ys.isList(path, node))                                                                                  ;
            REQUIRE(!ys.isLeaf(path, node))                                                                                  ;
            REQUIRE(!ys.isContainer(path, node))                                                                             ;}

        SECTION("\"is\" methods return false for existing nodes for different nodetypes")                                    {
            SECTION("example-schema:a")                                                                                      {
                node.first = "example-schema"                                                                                ;
                node.second = "a"                                                                                            ;}

            SECTION("example-schema:a/a2")                                                                                   {
                path.m_nodes.push_back(schemaNode_(module_{"example-schema"},container_("a")))                               ;
                node.second = "a2"                                                                                           ;}

            REQUIRE(!ys.isPresenceContainer(path, node))                                                                     ;
            REQUIRE(!ys.isList(path, node))                                                                                  ;
            REQUIRE(!ys.isLeaf(path, node))                                                                                  ;}

        SECTION("nodetype-specific methods called with different nodetypes")                                                 {
            path.m_nodes.push_back(schemaNode_(module_{"example-schema"},container_("a")))                                   ;
            node.second = "a2"                                                                                               ;

            REQUIRE(!ys.leafEnumHasValue(path, node, "haha"))                                                                ;
            REQUIRE(!ys.listHasKey(path, node, "chacha"))                                                                    ;}

        SECTION("nonexistent module")                                                                                        {
            REQUIRE(!ys.isModule(path, "notAModule"))                                                                        ;}

        SECTION("leafIdentityIsValid")                                                                                       {
            ModuleValuePair value                                                                                            ;
            SECTION("pizzaIdentLeaf")                                                                                        {
                node.first = "example-schema"                                                                                ;
                node.second = "pizzaIdentLeaf"                                                                               ;

                SECTION("wrong base ident")                                                                                  {
                    SECTION("food")                                                                                          {
                        value.second = "food"                                                                                ;}
                    SECTION("fruit")                                                                                         {
                        value.second = "fruit"                                                                               ;}}
                SECTION("non-existent identity")                                                                             {
                    value.second = "nonexistent"                                                                             ;}
                SECTION("weird module")                                                                                      {
                    value.first = "ahahaha"                                                                                  ;
                    value.second = "pizza"                                                                                   ;}}
            SECTION("different module identity, but withotu prefix")                                                         {
                node.first = "example-schema"                                                                                ;
                node.second = "foodIdentLeaf"                                                                                ;
                value.second = "pineapple"                                                                                   ;}
            REQUIRE_FALSE(ys.leafIdentityIsValid(path, node, value))                                                         ;}}}
