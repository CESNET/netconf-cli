/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/

#include <boost/optional/optional_io.hpp>
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

class MockRecorder : public trompeloeil::mock_interface<Recorder> {
public:
    IMPLEMENT_MOCK3(write);
};

namespace std {
std::ostream& operator<<(std::ostream& s, const DatastoreAccess::Tree& map)
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

    using namespace std::literals::string_literals;

    SECTION("set leafInt8 to -128")
    {
        REQUIRE_CALL(mock, write("/example-schema:leafInt8", boost::none, "-128"s));
        datastore.setLeaf("/example-schema:leafInt8", int8_t{-128});
        datastore.commitChanges();
    }

    SECTION("set leafInt16 to -32768")
    {
        REQUIRE_CALL(mock, write("/example-schema:leafInt16", boost::none, "-32768"s));
        datastore.setLeaf("/example-schema:leafInt16", int16_t{-32768});
        datastore.commitChanges();
    }

    SECTION("set leafInt32 to -2147483648")
    {
        REQUIRE_CALL(mock, write("/example-schema:leafInt32", boost::none, "-2147483648"s));
        datastore.setLeaf("/example-schema:leafInt32", int32_t{-2147483648});
        datastore.commitChanges();
    }

    SECTION("set leafInt64 to -50000000000")
    {
        REQUIRE_CALL(mock, write("/example-schema:leafInt64", boost::none, "-50000000000"s));
        datastore.setLeaf("/example-schema:leafInt64", int64_t{-50000000000});
        datastore.commitChanges();
    }

    SECTION("set leafUInt8 to 255")
    {
        REQUIRE_CALL(mock, write("/example-schema:leafUInt8", boost::none, "255"s));
        datastore.setLeaf("/example-schema:leafUInt8", uint8_t{255});
        datastore.commitChanges();
    }

    SECTION("set leafUInt16 to 65535")
    {
        REQUIRE_CALL(mock, write("/example-schema:leafUInt16", boost::none, "65535"s));
        datastore.setLeaf("/example-schema:leafUInt16", uint16_t{65535});
        datastore.commitChanges();
    }

    SECTION("set leafUInt32 to 4294967295")
    {
        REQUIRE_CALL(mock, write("/example-schema:leafUInt32", boost::none, "4294967295"s));
        datastore.setLeaf("/example-schema:leafUInt32", uint32_t{4294967295});
        datastore.commitChanges();
    }

    SECTION("set leafUInt64 to 50000000000")
    {
        REQUIRE_CALL(mock, write("/example-schema:leafUInt64", boost::none, "50000000000"s));
        datastore.setLeaf("/example-schema:leafUInt64", uint64_t{50000000000});
        datastore.commitChanges();
    }

    SECTION("set leafEnum to coze")
    {
        REQUIRE_CALL(mock, write("/example-schema:leafEnum", boost::none, "coze"s));
        datastore.setLeaf("/example-schema:leafEnum", enum_{"coze"});
        datastore.commitChanges();
    }

    SECTION("set leafDecimal to 123.544")
    {
        REQUIRE_CALL(mock, write("/example-schema:leafDecimal", boost::none, "123.544"s));
        datastore.setLeaf("/example-schema:leafDecimal", 123.544);
        datastore.commitChanges();
    }

    SECTION("create presence container")
    {
        REQUIRE_CALL(mock, write("/example-schema:pContainer", boost::none, ""s));
        datastore.createPresenceContainer("/example-schema:pContainer");
        datastore.commitChanges();
    }

    SECTION("create a list instance")
    {
        REQUIRE_CALL(mock, write("/example-schema:person[name='Nguyen']", boost::none, ""s));
        REQUIRE_CALL(mock, write("/example-schema:person[name='Nguyen']/name", boost::none, "Nguyen"s));
        datastore.createListInstance("/example-schema:person[name='Nguyen']");
        datastore.commitChanges();
    }

    SECTION("leafref pointing to a key of a list")
    {
        {
            REQUIRE_CALL(mock, write("/example-schema:person[name='Dan']", boost::none, ""s));
            REQUIRE_CALL(mock, write("/example-schema:person[name='Dan']/name", boost::none, "Dan"s));
            REQUIRE_CALL(mock, write("/example-schema:person[name='Elfi']", boost::none, ""s));
            REQUIRE_CALL(mock, write("/example-schema:person[name='Elfi']/name", boost::none, "Elfi"s));
            REQUIRE_CALL(mock, write("/example-schema:person[name='Kolafa']", boost::none, ""s));
            REQUIRE_CALL(mock, write("/example-schema:person[name='Kolafa']/name", boost::none, "Kolafa"s));
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
            REQUIRE_CALL(mock, write("/example-schema:bossPerson", boost::none, "Dan"s));
            datastore.setLeaf("/example-schema:bossPerson", std::string{"Dan"});
            datastore.commitChanges();
        }

        SECTION("Elfi")
        {
            REQUIRE_CALL(mock, write("/example-schema:bossPerson", boost::none, "Elfi"s));
            datastore.setLeaf("/example-schema:bossPerson", std::string{"Elfi"});
            datastore.commitChanges();
        }

        SECTION("Kolafa")
        {
            REQUIRE_CALL(mock, write("/example-schema:bossPerson", boost::none, "Kolafa"s));
            datastore.setLeaf("/example-schema:bossPerson", std::string{"Kolafa"});
            datastore.commitChanges();
        }
    }
    SECTION("bool values get correctly represented as bools")
    {
        {
            REQUIRE_CALL(mock, write("/example-schema:down", boost::none, "true"s));
            datastore.setLeaf("/example-schema:down", bool{true});
            datastore.commitChanges();
        }

        DatastoreAccess::Tree expected{{"/example-schema:down", bool{true}}};
        REQUIRE(datastore.getItems("/example-schema:down") == expected);
    }

    SECTION("getting items from the whole module")
    {
        {
            REQUIRE_CALL(mock, write("/example-schema:up", boost::none, "true"s));
            REQUIRE_CALL(mock, write("/example-schema:down", boost::none, "false"s));
            datastore.setLeaf("/example-schema:up", bool{true});
            datastore.setLeaf("/example-schema:down", bool{false});
            datastore.commitChanges();
        }

        DatastoreAccess::Tree expected{{"/example-schema:down", bool{false}},
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
        REQUIRE(datastore.getItems("/example-schema:*") == expected);
    }

    SECTION("getItems returns correct datatypes")
    {
        {
            REQUIRE_CALL(mock, write("/example-schema:leafEnum", boost::none, "lol"s));
            datastore.setLeaf("/example-schema:leafEnum", enum_{"lol"});
            datastore.commitChanges();
        }
        DatastoreAccess::Tree expected{{"/example-schema:leafEnum", enum_{"lol"}}};

        REQUIRE(datastore.getItems("/example-schema:leafEnum") == expected);
    }

    SECTION("getItems on a list")
    {
        {
            REQUIRE_CALL(mock, write("/example-schema:person[name='Jan']", boost::none, ""s));
            REQUIRE_CALL(mock, write("/example-schema:person[name='Jan']/name", boost::none, "Jan"s));
            REQUIRE_CALL(mock, write("/example-schema:person[name='Michal']", boost::none, ""s));
            REQUIRE_CALL(mock, write("/example-schema:person[name='Michal']/name", boost::none, "Michal"s));
            REQUIRE_CALL(mock, write("/example-schema:person[name='Petr']", boost::none, ""s));
            REQUIRE_CALL(mock, write("/example-schema:person[name='Petr']/name", boost::none, "Petr"s));
            datastore.createListInstance("/example-schema:person[name='Jan']");
            datastore.createListInstance("/example-schema:person[name='Michal']");
            datastore.createListInstance("/example-schema:person[name='Petr']");
            datastore.commitChanges();
        }
        DatastoreAccess::Tree expected{
            {"/example-schema:person[name='Jan']", special_{SpecialValue::List}},
            {"/example-schema:person[name='Jan']/name", std::string{"Jan"}},
            {"/example-schema:person[name='Michal']", special_{SpecialValue::List}},
            {"/example-schema:person[name='Michal']/name", std::string{"Michal"}},
            {"/example-schema:person[name='Petr']", special_{SpecialValue::List}},
            {"/example-schema:person[name='Petr']/name", std::string{"Petr"}}
        };

        REQUIRE(datastore.getItems("/example-schema:person") == expected);
    }

    SECTION("presence containers")
    {
        DatastoreAccess::Tree expected;
        // Make sure it's not there before we create it
        REQUIRE(datastore.getItems("/example-schema:pContainer") == expected);

        {
            REQUIRE_CALL(mock, write("/example-schema:pContainer", boost::none, ""s));
            datastore.createPresenceContainer("/example-schema:pContainer");
            datastore.commitChanges();
        }
        expected = {
            {"/example-schema:pContainer", special_{SpecialValue::PresenceContainer}}
        };
        REQUIRE(datastore.getItems("/example-schema:pContainer") == expected);

        // Make sure it's not there after we delete it
        {
            REQUIRE_CALL(mock, write("/example-schema:pContainer", ""s, boost::none));
            datastore.deletePresenceContainer("/example-schema:pContainer");
            datastore.commitChanges();
        }
        expected = {};
        REQUIRE(datastore.getItems("/example-schema:pContainer") == expected);

    }

    waitForCompletionAndBitMore(seq1);
}
