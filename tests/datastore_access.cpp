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

using namespace std::literals;

class MockRecorder : public Recorder {
public:
    MAKE_MOCK3(write, void(const std::string&, const std::string&, const std::string&), override);
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

        DatastoreAccess::Tree expected{{"/example-schema:down", bool{true}}};
        REQUIRE(datastore.getItems("/example-schema:down") == expected);
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
            REQUIRE_CALL(mock, write("/example-schema:leafEnum", "", "lol"));
            datastore.setLeaf("/example-schema:leafEnum", enum_{"lol"});
            datastore.commitChanges();
        }
        DatastoreAccess::Tree expected{{"/example-schema:leafEnum", enum_{"lol"}}};

        REQUIRE(datastore.getItems("/example-schema:leafEnum") == expected);
    }

    SECTION("getItems on a list")
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

    waitForCompletionAndBitMore(seq1);
}

class RpcCb: public sysrepo::Callback {
    int rpc(const char *xpath, const ::sysrepo::S_Vals input, ::sysrepo::S_Vals_Holder output, void *) override
    {
        const auto nukes = "/example-schema:launch-nukes"s;
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

#ifdef sysrepo_BACKEND
    SysrepoAccess datastore("netconf-cli-test");
#elif defined(netconf_BACKEND)
    NetconfAccess datastore(NETOPEER_SOCKET_PATH);
#else
#error "Unknown backend"
#endif

    std::string rpc;
    DatastoreAccess::Tree input, output;

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
#ifdef sysrepo_BACKEND
            // FIXME: find out why this one is conditional, visible only via NETCONF
            {"damaged-places", special_{SpecialValue::PresenceContainer}},
#endif
            {"damaged-places/targets[city='London']", special_{SpecialValue::List}},
            {"damaged-places/targets[city='London']/city", "London"s},
            {"damaged-places/targets[city='Berlin']", special_{SpecialValue::List}},
            {"damaged-places/targets[city='Berlin']/city", "Berlin"s},
        };
    }

    REQUIRE(datastore.executeRpc(rpc, input) == output);

    waitForCompletionAndBitMore(seq1);
}
