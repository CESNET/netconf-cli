/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/

#include "trompeloeil_doctest.hpp"
#include "completion.hpp"
#include "leaf_data_helpers.hpp"
#include "utils.hpp"

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

    SECTION("joinPaths") {
        std::string prefix, suffix, result;

        SECTION("regular") {
            prefix = "/example:a";
            suffix = "leaf";
            result = "/example:a/leaf";
        }

        SECTION("no prefix - absolute path") {
            suffix = "/example:a/leaf";
            result = "/example:a/leaf";
        }

        SECTION("no prefix - relative path") {
            suffix = "example:a/leaf";
            result = "example:a/leaf";
        }

        SECTION("no suffix") {
            prefix = "/example:a/leaf";
            result = "/example:a/leaf";
        }

        SECTION("at root") {
            prefix = "/";
            suffix = "example:a";
            result = "/example:a";
        }

        SECTION("trailing slash") {
            prefix = "/example:a";
            suffix = "/";
            result = "/example:a/";
        }

        SECTION("prefix ends with slash") {
            prefix = "/example:a/";
            suffix = "leaf";
            result = "/example:a/leaf";
        }

        SECTION("suffix starts with slash") {
            prefix = "/example:a";
            suffix = "/leaf";
            result = "/example:a/leaf";
        }

        SECTION("slashes all the way to eleven") {
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
