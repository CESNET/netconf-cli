/*
 * Copyright (C) 2019 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/

#include <experimental/iterator>
#include "trompeloeil_doctest.h"

#ifdef sysrepo_BACKEND
#include "sysrepo_access.hpp"
#elif defined(netconf_BACKEND)
#include "netconf_access.hpp"
#include "netopeer_vars.hpp"
#else
#error "Unknown backend"
#endif
#include "data_query.hpp"
#include "sysrepo_subscription.hpp"
#include "utils.hpp"

namespace std {
std::ostream& operator<<(std::ostream& s, const std::vector<std::map<std::string, std::string>> set)
{
    s << std::endl << "{" << std::endl;
    std::transform(set.begin(), set.end(), std::experimental::make_ostream_joiner(s, ", \n"), [](const auto& map) {
        std::ostringstream ss;
        ss << "    {" << std::endl << "        ";
        std::transform(map.begin(), map.end(), std::experimental::make_ostream_joiner(ss, ", \n        "), [](const auto& keyValue){
            return "{" + keyValue.first + ", " + keyValue.second + "}";
        });
        ss << std::endl << "    }";
        return ss.str();
    });
    s << std::endl << "}" << std::endl;
    return s;
}
}

TEST_CASE("data query")
{
    trompeloeil::sequence seq1;
    SysrepoSubscription subscription;

#ifdef sysrepo_BACKEND
    SysrepoAccess datastore("netconf-cli-test");
#elif defined(netconf_BACKEND)
    NetconfAccess datastore(NETOPEER_SOCKET_PATH);
#else
#error "Unknown backend"
#endif

    DataQuery dataquery(datastore);

    SECTION("listKeys")
    {
        dataPath_ location;
        location.m_scope = Scope::Absolute;
        ModuleNodePair node;
        std::vector<std::map<std::string, std::string>> expected;

        SECTION("example-schema:person")
        {
            datastore.createListInstance("/example-schema:person[name='Vaclav']");
            datastore.createListInstance("/example-schema:person[name='Tomas']");
            datastore.createListInstance("/example-schema:person[name='Jan Novak']");
            node.first = "example-schema";
            node.second = "person";
            expected = {
                { {"name", "'Jan Novak'"} },
                { {"name", "'Tomas'"} },
                { {"name", "'Vaclav'"} }
            };
        }

        SECTION("example-schema:selectedNumbers")
        {
            datastore.createListInstance("/example-schema:selectedNumbers[value='45']");
            datastore.createListInstance("/example-schema:selectedNumbers[value='99']");
            datastore.createListInstance("/example-schema:selectedNumbers[value='127']");
            node.first = "example-schema";
            node.second = "selectedNumbers";
            expected = {
                { {"value", "127"} },
                { {"value", "45"} },
                { {"value", "99"} }
            };
        }

        SECTION("example-schema:animalWithColor")
        {
            datastore.createListInstance("/example-schema:animalWithColor[name='Dog'][color='brown']");
            datastore.createListInstance("/example-schema:animalWithColor[name='Dog'][color='white']");
            datastore.createListInstance("/example-schema:animalWithColor[name='Cat'][color='grey']");
            node.first = "example-schema";
            node.second = "animalWithColor";
            expected = {
                { {"name", "'Cat'"}, {"color", "'grey'"} },
                { {"name", "'Dog'"}, {"color", "'brown'"} },
                { {"name", "'Dog'"}, {"color", "'white'"} }
            };
        }

        SECTION("example-schema:animalWithColor - quotes in values")
        {
            datastore.createListInstance("/example-schema:animalWithColor[name='D\"o\"g'][color=\"b'r'own\"]");
            node.first = "example-schema";
            node.second = "animalWithColor";
            expected = {
                { {"name", "'D\"o\"g'"}, {"color", "\"b'r'own\""} }
            };
        }

        SECTION("example-schema:ports")
        {
            datastore.createListInstance("/example-schema:ports[name='A']");
            datastore.createListInstance("/example-schema:ports[name='B']");
            datastore.createListInstance("/example-schema:ports[name='E']");
            node.first = "example-schema";
            node.second = "ports";
            expected = {
                { {"name", "'A'"} },
                { {"name", "'B'"} },
                { {"name", "'E'"} },
            };
        }

        datastore.commitChanges();
        REQUIRE(dataquery.listKeys(location, node) == expected);
    }

    waitForCompletionAndBitMore(seq1);
}
