/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/

#include "trompeloeil_doctest.h"

#ifdef sysrepo_BACKEND
#include "sysrepo_access.hpp"
#elif defined(netconf_BACKEND)
#include "netconf_access.hpp"
#include "netopeer_vars.hpp"
#else
#error "Unknown backend"
#endif
#include "sysrepo_subscription.hpp"
#include "utils.hpp"

class MockRecorder : public Recorder {
public:
    MAKE_MOCK3(write, void(const std::string&, const std::string&, const std::string&), override);
};

namespace std {
std::ostream& operator<<(std::ostream& s, const std::map<std::string, leaf_data_> map)
{
    s << std::endl
      << "{";
    for (const auto& it : map) {
        s << "{\"" << it.first << "\", " << leafDataToString(it.second) << "}" << std::endl;
    }
    s << "}" << std::endl;
    return s;
}
}

TEST_CASE("setting/getting values")
{
    trompeloeil::sequence seq1;
    MockRecorder mock;
    SysrepoSubscription subscription(&mock);

#ifdef sysrepo_BACKEND
    SysrepoAccess datastore("netconf-cli-test");
#elif defined(netconf_BACKEND)
    NetconfAccess datastore(NETOPEER_SOCKET_PATH);
#else
#error "Unknown backend"
#endif

    SECTION("set leafInt8 to -128")
    {
        REQUIRE_CALL(mock, write("/example-schema:leafInt8", "", "-128"));
        datastore.setLeaf("/example-schema:leafInt8", int8_t{-128});
        datastore.commitChanges();
    }

    SECTION("set leafInt16 to -32768")
    {
        REQUIRE_CALL(mock, write("/example-schema:leafInt16", "", "-32768"));
        datastore.setLeaf("/example-schema:leafInt16", int16_t{-32768});
        datastore.commitChanges();
    }

    SECTION("set leafInt32 to -2147483648")
    {
        REQUIRE_CALL(mock, write("/example-schema:leafInt32", "", "-2147483648"));
        datastore.setLeaf("/example-schema:leafInt32", int32_t{-2147483648});
        datastore.commitChanges();
    }

    SECTION("set leafInt64 to -50000000000")
    {
        REQUIRE_CALL(mock, write("/example-schema:leafInt64", "", "-50000000000"));
        datastore.setLeaf("/example-schema:leafInt64", int64_t{-50000000000});
        datastore.commitChanges();
    }

    SECTION("set leafUInt8 to 255")
    {
        REQUIRE_CALL(mock, write("/example-schema:leafUInt8", "", "255"));
        datastore.setLeaf("/example-schema:leafUInt8", uint8_t{255});
        datastore.commitChanges();
    }

    SECTION("set leafUInt16 to 65535")
    {
        REQUIRE_CALL(mock, write("/example-schema:leafUInt16", "", "65535"));
        datastore.setLeaf("/example-schema:leafUInt16", uint16_t{65535});
        datastore.commitChanges();
    }

    SECTION("set leafUInt32 to 4294967295")
    {
        REQUIRE_CALL(mock, write("/example-schema:leafUInt32", "", "4294967295"));
        datastore.setLeaf("/example-schema:leafUInt32", uint32_t{4294967295});
        datastore.commitChanges();
    }

    SECTION("set leafUInt64 to 50000000000")
    {
        REQUIRE_CALL(mock, write("/example-schema:leafUInt64", "", "50000000000"));
        datastore.setLeaf("/example-schema:leafUInt64", uint64_t{50000000000});
        datastore.commitChanges();
    }

    SECTION("set leafEnum to coze")
    {
        REQUIRE_CALL(mock, write("/example-schema:leafEnum", "", "coze"));
        datastore.setLeaf("/example-schema:leafEnum", enum_{"coze"});
        datastore.commitChanges();
    }

    SECTION("set leafDecimal to 123.544")
    {
        REQUIRE_CALL(mock, write("/example-schema:leafDecimal", "", "123.544"));
        datastore.setLeaf("/example-schema:leafDecimal", 123.544);
        datastore.commitChanges();
    }

    SECTION("create presence container")
    {
        REQUIRE_CALL(mock, write("/example-schema:pContainer", "", ""));
        datastore.createPresenceContainer("/example-schema:pContainer");
        datastore.commitChanges();
    }

    SECTION("create a list instance")
    {
        REQUIRE_CALL(mock, write("/example-schema:person[name='Nguyen']", "", ""));
        REQUIRE_CALL(mock, write("/example-schema:person[name='Nguyen']/name", "", "Nguyen"));
        datastore.createListInstance("/example-schema:person[name='Nguyen']");
        datastore.commitChanges();
    }

    SECTION("leafref pointing to a key of a list")
    {
        {
            REQUIRE_CALL(mock, write("/example-schema:person[name='Dan']", "", ""));
            REQUIRE_CALL(mock, write("/example-schema:person[name='Dan']/name", "", "Dan"));
            REQUIRE_CALL(mock, write("/example-schema:person[name='Elfi']", "", ""));
            REQUIRE_CALL(mock, write("/example-schema:person[name='Elfi']/name", "", "Elfi"));
            REQUIRE_CALL(mock, write("/example-schema:person[name='Kolafa']", "", ""));
            REQUIRE_CALL(mock, write("/example-schema:person[name='Kolafa']/name", "", "Kolafa"));
            datastore.createListInstance("/example-schema:person[name='Dan']");
            datastore.createListInstance("/example-schema:person[name='Elfi']");
            datastore.createListInstance("/example-schema:person[name='Kolafa']");
            datastore.commitChanges();
        }

        // The commitChanges method has to be called in each of the
        // SECTIONs, because the REQUIRE_CALL only works inside the given
        // SECTION.
        SECTION("Dan")
        {
            REQUIRE_CALL(mock, write("/example-schema:bossPerson", "", "Dan"));
            datastore.setLeaf("/example-schema:bossPerson", std::string{"Dan"});
            datastore.commitChanges();
        }

        SECTION("Elfi")
        {
            REQUIRE_CALL(mock, write("/example-schema:bossPerson", "", "Elfi"));
            datastore.setLeaf("/example-schema:bossPerson", std::string{"Elfi"});
            datastore.commitChanges();
        }

        SECTION("Kolafa")
        {
            REQUIRE_CALL(mock, write("/example-schema:bossPerson", "", "Kolafa"));
            datastore.setLeaf("/example-schema:bossPerson", std::string{"Kolafa"});
            datastore.commitChanges();
        }
    }
    SECTION("bool values get correctly represented as bools")
    {
        {
            REQUIRE_CALL(mock, write("/example-schema:down", "", "true"));
            datastore.setLeaf("/example-schema:down", bool{true});
            datastore.commitChanges();
        }

        std::map<std::string, leaf_data_> expected{{"/example-schema:down", bool{true}}};
        REQUIRE(datastore.getItems("/example-schema:down", Recursion::Recursive) == expected);
    }

    SECTION("getting items from the whole module")
    {
        {
            REQUIRE_CALL(mock, write("/example-schema:up", "", "true"));
            REQUIRE_CALL(mock, write("/example-schema:down", "", "false"));
            datastore.setLeaf("/example-schema:up", bool{true});
            datastore.setLeaf("/example-schema:down", bool{false});
            datastore.commitChanges();
        }

        std::map<std::string, leaf_data_> expected{{"/example-schema:down", bool{false}},
        // Sysrepo always returns containers when getting values, but
        // libnetconf does not. This is fine by the YANG standard:
        // https://tools.ietf.org/html/rfc7950#section-7.5.7 Furthermore,
        // NetconfAccess implementation actually only iterates over leafs,
        // so even if libnetconf did include containers, they wouldn't get
        // shown here anyway. With sysrepo2, this won't be necessary,
        // because it'll use the same data structure as libnetconf, so the
        // results will be consistent.
#ifdef sysrepo_BACKEND
                                                   {"/example-schema:lol", special_{SpecialValue::Container}},
#endif
                                                   {"/example-schema:up", bool{true}}};
        REQUIRE(datastore.getItems("/example-schema:*", Recursion::Recursive) == expected);
    }

    SECTION("getItems returns correct datatypes")
    {
        {
            REQUIRE_CALL(mock, write("/example-schema:leafEnum", "", "lol"));
            datastore.setLeaf("/example-schema:leafEnum", enum_{"lol"});
            datastore.commitChanges();
        }
        std::map<std::string, leaf_data_> expected{{"/example-schema:leafEnum", enum_{"lol"}}};

        REQUIRE(datastore.getItems("/example-schema:leafEnum", Recursion::Recursive) == expected);
    }

    SECTION("getItems on a path with a list")
    {
        {
            REQUIRE_CALL(mock, write("/example-schema:person[name='Jan']", "", ""));
            REQUIRE_CALL(mock, write("/example-schema:person[name='Jan']/name", "", "Jan"));
            REQUIRE_CALL(mock, write("/example-schema:person[name='Michal']", "", ""));
            REQUIRE_CALL(mock, write("/example-schema:person[name='Michal']/name", "", "Michal"));
            REQUIRE_CALL(mock, write("/example-schema:person[name='Petr']", "", ""));
            REQUIRE_CALL(mock, write("/example-schema:person[name='Petr']/name", "", "Petr"));
            datastore.createListInstance("/example-schema:person[name='Jan']");
            datastore.createListInstance("/example-schema:person[name='Michal']");
            datastore.createListInstance("/example-schema:person[name='Petr']");
            datastore.commitChanges();
        }

        std::string path;
        std::map<std::string, leaf_data_> expected;
        Recursion recursion;

        SECTION("getting the actual list")
        {
            path = "/example-schema:person";
            SECTION("recursive")
            {
                recursion = Recursion::Recursive;
                expected = {
                    {"/example-schema:person[name='Jan']", special_{SpecialValue::List}},
                    {"/example-schema:person[name='Jan']/name", std::string{"Jan"}},
                    {"/example-schema:person[name='Michal']", special_{SpecialValue::List}},
                    {"/example-schema:person[name='Michal']/name", std::string{"Michal"}},
                    {"/example-schema:person[name='Petr']", special_{SpecialValue::List}},
                    {"/example-schema:person[name='Petr']/name", std::string{"Petr"}}
                };
            }
            SECTION("non-recursive")
            {
                recursion = Recursion::NonRecursive;
                expected = {
                    {"/example-schema:person[name='Jan']", special_{SpecialValue::List}},
                    {"/example-schema:person[name='Michal']", special_{SpecialValue::List}},
                    {"/example-schema:person[name='Petr']", special_{SpecialValue::List}},
                };
            }
        }

        SECTION("getting a leaf inside the list")
        {
            recursion = Recursion::Recursive;
            path = "/example-schema:person/name";
            expected = {
                {"/example-schema:person[name='Jan']/name", std::string{"Jan"}},
                {"/example-schema:person[name='Michal']/name", std::string{"Michal"}},
                {"/example-schema:person[name='Petr']/name", std::string{"Petr"}}
            };
        }

        REQUIRE(datastore.getItems(path, recursion) == expected);
    }

    waitForCompletionAndBitMore(seq1);
}
