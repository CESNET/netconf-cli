/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/

#include "trompeloeil_doctest.h"
#include <sysrepo-cpp/Session.hpp>

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

using namespace std::literals::string_literals;

class MockRecorder : public trompeloeil::mock_interface<Recorder> {
public:
    IMPLEMENT_MOCK3(write);
};

namespace std {
std::ostream& operator<<(std::ostream& s, const std::optional<std::string>& opt)
{
    s << (opt ? *opt : "std::nullopt");
    return s;
}

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

#if 0
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
        REQUIRE_CALL(mock, write("/example-schema:leafInt8", std::nullopt, "-128"s));
        datastore.setLeaf("/example-schema:leafInt8", int8_t{-128});
        datastore.commitChanges();
    }

    SECTION("set leafInt16 to -32768")
    {
        REQUIRE_CALL(mock, write("/example-schema:leafInt16", std::nullopt, "-32768"s));
        datastore.setLeaf("/example-schema:leafInt16", int16_t{-32768});
        datastore.commitChanges();
    }

    SECTION("set leafInt32 to -2147483648")
    {
        REQUIRE_CALL(mock, write("/example-schema:leafInt32", std::nullopt, "-2147483648"s));
        datastore.setLeaf("/example-schema:leafInt32", int32_t{-2147483648});
        datastore.commitChanges();
    }

    SECTION("set leafInt64 to -50000000000")
    {
        REQUIRE_CALL(mock, write("/example-schema:leafInt64", std::nullopt, "-50000000000"s));
        datastore.setLeaf("/example-schema:leafInt64", int64_t{-50000000000});
        datastore.commitChanges();
    }

    SECTION("set leafUInt8 to 255")
    {
        REQUIRE_CALL(mock, write("/example-schema:leafUInt8", std::nullopt, "255"s));
        datastore.setLeaf("/example-schema:leafUInt8", uint8_t{255});
        datastore.commitChanges();
    }

    SECTION("set leafUInt16 to 65535")
    {
        REQUIRE_CALL(mock, write("/example-schema:leafUInt16", std::nullopt, "65535"s));
        datastore.setLeaf("/example-schema:leafUInt16", uint16_t{65535});
        datastore.commitChanges();
    }

    SECTION("set leafUInt32 to 4294967295")
    {
        REQUIRE_CALL(mock, write("/example-schema:leafUInt32", std::nullopt, "4294967295"s));
        datastore.setLeaf("/example-schema:leafUInt32", uint32_t{4294967295});
        datastore.commitChanges();
    }

    SECTION("set leafUInt64 to 50000000000")
    {
        REQUIRE_CALL(mock, write("/example-schema:leafUInt64", std::nullopt, "50000000000"s));
        datastore.setLeaf("/example-schema:leafUInt64", uint64_t{50000000000});
        datastore.commitChanges();
    }

    SECTION("set leafEnum to coze")
    {
        REQUIRE_CALL(mock, write("/example-schema:leafEnum", std::nullopt, "coze"s));
        datastore.setLeaf("/example-schema:leafEnum", enum_{"coze"});
        datastore.commitChanges();
    }

    SECTION("set leafDecimal to 123.544")
    {
        REQUIRE_CALL(mock, write("/example-schema:leafDecimal", std::nullopt, "123.544"s));
        datastore.setLeaf("/example-schema:leafDecimal", 123.544);
        datastore.commitChanges();
    }

    SECTION("create presence container")
    {
        REQUIRE_CALL(mock, write("/example-schema:pContainer", std::nullopt, ""s));
        datastore.createPresenceContainer("/example-schema:pContainer");
        datastore.commitChanges();
    }

    SECTION("create a list instance")
    {
        REQUIRE_CALL(mock, write("/example-schema:person[name='Nguyen']", std::nullopt, ""s));
        REQUIRE_CALL(mock, write("/example-schema:person[name='Nguyen']/name", std::nullopt, "Nguyen"s));
        datastore.createListInstance("/example-schema:person[name='Nguyen']");
        datastore.commitChanges();
    }

    SECTION("leafref pointing to a key of a list")
    {
        {
            REQUIRE_CALL(mock, write("/example-schema:person[name='Dan']", std::nullopt, ""s));
            REQUIRE_CALL(mock, write("/example-schema:person[name='Dan']/name", std::nullopt, "Dan"s));
            REQUIRE_CALL(mock, write("/example-schema:person[name='Elfi']", std::nullopt, ""s));
            REQUIRE_CALL(mock, write("/example-schema:person[name='Elfi']/name", std::nullopt, "Elfi"s));
            REQUIRE_CALL(mock, write("/example-schema:person[name='Kolafa']", std::nullopt, ""s));
            REQUIRE_CALL(mock, write("/example-schema:person[name='Kolafa']/name", std::nullopt, "Kolafa"s));
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
            REQUIRE_CALL(mock, write("/example-schema:bossPerson", std::nullopt, "Dan"s));
            datastore.setLeaf("/example-schema:bossPerson", std::string{"Dan"});
            datastore.commitChanges();
        }

        SECTION("Elfi")
        {
            REQUIRE_CALL(mock, write("/example-schema:bossPerson", std::nullopt, "Elfi"s));
            datastore.setLeaf("/example-schema:bossPerson", std::string{"Elfi"});
            datastore.commitChanges();
        }

        SECTION("Kolafa")
        {
            REQUIRE_CALL(mock, write("/example-schema:bossPerson", std::nullopt, "Kolafa"s));
            datastore.setLeaf("/example-schema:bossPerson", std::string{"Kolafa"});
            datastore.commitChanges();
        }
    }
    SECTION("bool values get correctly represented as bools")
    {
        {
            REQUIRE_CALL(mock, write("/example-schema:down", std::nullopt, "true"s));
            datastore.setLeaf("/example-schema:down", bool{true});
            datastore.commitChanges();
        }

        DatastoreAccess::Tree expected{{"/example-schema:down", bool{true}}};
        REQUIRE(datastore.getItems("/example-schema:down") == expected);
    }

    SECTION("getting items from the whole module")
    {
        {
            REQUIRE_CALL(mock, write("/example-schema:up", std::nullopt, "true"s));
            REQUIRE_CALL(mock, write("/example-schema:down", std::nullopt, "false"s));
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
            REQUIRE_CALL(mock, write("/example-schema:leafEnum", std::nullopt, "lol"s));
            datastore.setLeaf("/example-schema:leafEnum", enum_{"lol"});
            datastore.commitChanges();
        }
        DatastoreAccess::Tree expected{{"/example-schema:leafEnum", enum_{"lol"}}};

        REQUIRE(datastore.getItems("/example-schema:leafEnum") == expected);
    }

    SECTION("getItems on a list")
    {
        {
            REQUIRE_CALL(mock, write("/example-schema:person[name='Jan']", std::nullopt, ""s));
            REQUIRE_CALL(mock, write("/example-schema:person[name='Jan']/name", std::nullopt, "Jan"s));
            REQUIRE_CALL(mock, write("/example-schema:person[name='Michal']", std::nullopt, ""s));
            REQUIRE_CALL(mock, write("/example-schema:person[name='Michal']/name", std::nullopt, "Michal"s));
            REQUIRE_CALL(mock, write("/example-schema:person[name='Petr']", std::nullopt, ""s));
            REQUIRE_CALL(mock, write("/example-schema:person[name='Petr']/name", std::nullopt, "Petr"s));
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
            REQUIRE_CALL(mock, write("/example-schema:pContainer", std::nullopt, ""s));
            datastore.createPresenceContainer("/example-schema:pContainer");
            datastore.commitChanges();
        }
        expected = {
            {"/example-schema:pContainer", special_{SpecialValue::PresenceContainer}}
        };
        REQUIRE(datastore.getItems("/example-schema:pContainer") == expected);

        // Make sure it's not there after we delete it
        {
            REQUIRE_CALL(mock, write("/example-schema:pContainer", ""s, std::nullopt));
            datastore.deletePresenceContainer("/example-schema:pContainer");
            datastore.commitChanges();
        }
        expected = {};
        REQUIRE(datastore.getItems("/example-schema:pContainer") == expected);

    }

    waitForCompletionAndBitMore(seq1);
}
#endif

class RpcCb: public sysrepo::Callback {
    int rpc(const char *xpath, const ::sysrepo::S_Vals input, ::sysrepo::S_Vals_Holder output, void *) override
    {
        const auto nukes = "/example-schema:launch-nukes"s;
        const auto measure = "/example-schema:measure"s;
        if (xpath == "/example-schema:noop"s) {
            return SR_ERR_OK;
        } else if (xpath == nukes) {
            uint64_t kilotons = 0;
            bool hasCities = false;
            for (size_t i = 0; i < input->val_cnt(); ++i) {
                const auto& val = input->val(i);
                if (val->xpath() == nukes + "/payload/kilotons") {
                    kilotons = val->data()->get_uint64();
                } else if (val->xpath() == nukes + "/payload") {
                    // ignore, container
                } else if (val->xpath() == nukes + "/description") {
                    // unused
                } else if (std::string_view{val->xpath()}.find(nukes + "/cities") == 0) {
                    hasCities = true;
                } else {
                    throw std::runtime_error("RPC launch-nukes: unexpected input "s + val->xpath());
                }
            }
            if (kilotons == 333'666) {
                // magic, just do not generate any output. This is important because the NETCONF RPC returns just <ok/>.
                return SR_ERR_OK;
            }
            auto buf = output->allocate(2);
            size_t i = 0;
            buf->val(i++)->set((nukes + "/blast-radius").c_str(), uint32_t{33'666});
            buf->val(i++)->set((nukes + "/actual-yield").c_str(), static_cast<uint64_t>(1.33 * kilotons));
            if (hasCities) {
                buf = output->reallocate(output->val_cnt() + 2);
                buf->val(i++)->set((nukes + "/damaged-places/targets[city='London']/city").c_str(), "London");
                buf->val(i++)->set((nukes + "/damaged-places/targets[city='Berlin']/city").c_str(), "Berlin");
            }
            return SR_ERR_OK;
        } else if (xpath == measure) {
            for (size_t i = 0; i < input->val_cnt(); ++i) {
                const auto& val = input->val(i);
                if (val->xpath() == measure + "/port") {
                    val->data()->get_uint8();
                } else {
                    throw std::runtime_error("RPC measure: pwned XPath "s + val->xpath());
                }
            }
            auto buf = output->allocate(1);
            size_t i = 0;
            buf->val(i++)->set((measure + "/power").c_str(), double{-12.34});
            //buf->val(i++)->set((measure + "/power").c_str(), uint32_t{123'456});
            return SR_ERR_OK;
        }
        throw std::runtime_error("unrecognized RPC");
    }
};

TEST_CASE("rpc") {
    trompeloeil::sequence seq1;
    auto srConn = std::make_shared<sysrepo::Connection>("netconf-cli-test-rpc");
    auto srSession = std::make_shared<sysrepo::Session>(srConn);
    auto srSubscription = std::make_shared<sysrepo::Subscribe>(srSession);
    auto cb = std::make_shared<RpcCb>();
    sysrepo::Logs{}.set_stderr(SR_LL_INF);
    srSubscription->rpc_subscribe("/example-schema:noop", cb, nullptr, SR_SUBSCR_CTX_REUSE);
    srSubscription->rpc_subscribe("/example-schema:launch-nukes", cb, nullptr, SR_SUBSCR_CTX_REUSE);
    srSubscription->rpc_subscribe("/example-schema:measure", cb, nullptr, SR_SUBSCR_CTX_REUSE);

#ifdef sysrepo_BACKEND
    SysrepoAccess datastore("netconf-cli-test");
#elif defined(netconf_BACKEND)
    NetconfAccess datastore(NETOPEER_SOCKET_PATH);
#else
#error "Unknown backend"
#endif

    std::string rpc;
    DatastoreAccess::Tree input, output;

#if 0
    SECTION("noop") {
        rpc = "/example-schema:noop";
    }

    SECTION("small nuke") {
        rpc = "/example-schema:launch-nukes";
        input = {
            {"description", "dummy"s},
            {"payload/kilotons", uint64_t{333'666}},
        };
        // no data are returned
    }

    SECTION("small nuke") {
        rpc = "/example-schema:launch-nukes";
        input = {
            {"description", "dummy"s},
            {"payload/kilotons", uint64_t{4}},
        };
        output = {
            {"blast-radius", uint32_t{33'666}},
            {"actual-yield", uint64_t{5}},
        };
    }

    SECTION("with lists") {
        rpc = "/example-schema:launch-nukes";
        input = {
            {"payload/kilotons", uint64_t{6}},
            {"cities/targets[city='Prague']/city", "Prague"s},
        };
        output = {
            {"blast-radius", uint32_t{33'666}},
            {"actual-yield", uint64_t{7}},
            {"damaged-places", special_{SpecialValue::PresenceContainer}},
            {"damaged-places/targets[city='London']", special_{SpecialValue::List}},
            {"damaged-places/targets[city='London']/city", "London"s},
            {"damaged-places/targets[city='Berlin']", special_{SpecialValue::List}},
            {"damaged-places/targets[city='Berlin']/city", "Berlin"s},
        };
    }

    SECTION("measure") {
#endif
        rpc = "/example-schema:measure";
        input = {
            {"port", uint8_t{6}},
        };
        output = {
            {"power", double{-12.34}},
            //{"power", uint32_t{123'456}},
        };
#if 0
    }
#endif


    REQUIRE(datastore.executeRpc(rpc, input) == output);

    waitForCompletionAndBitMore(seq1);
}
