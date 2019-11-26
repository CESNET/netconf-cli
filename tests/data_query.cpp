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
std::ostream& operator<<(std::ostream& s, const std::vector<std::map<std::string, leaf_data_>> set)
{
    s << std::endl << "{" << std::endl;
    std::transform(set.begin(), set.end(), std::experimental::make_ostream_joiner(s, ", \n"), [](const auto& map) {
        std::ostringstream ss;
        ss << "    {" << std::endl << "        ";
        std::transform(map.begin(), map.end(), std::experimental::make_ostream_joiner(ss, ", \n        "), [](const auto& keyValue){
            return "{" + keyValue.first + "{" + boost::core::demangle(keyValue.second.type().name()) + "}" + ", " + leafDataToString(keyValue.second) + "}";
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
    SysrepoSubscription subscriptionExample("example-schema");
    SysrepoSubscription subscriptionOther("other-module");

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
        std::vector<std::map<std::string, leaf_data_>> expected;

        SECTION("example-schema:person")
        {
            datastore.createListInstance("/example-schema:person[name='Vaclav']");
            datastore.createListInstance("/example-schema:person[name='Tomas']");
            datastore.createListInstance("/example-schema:person[name='Jan Novak']");
            node.first = "example-schema";
            node.second = "person";
            expected = {
                {{"name", std::string{"Jan Novak"}}},
                {{"name", std::string{"Tomas"}}},
                {{"name", std::string{"Vaclav"}}}
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
                {{"value", int8_t{127}}},
                {{"value", int8_t{45}}},
                {{"value", int8_t{99}}}
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
                {{"name", std::string{"Cat"}}, {"color", std::string{"grey"}}},
                {{"name", std::string{"Dog"}}, {"color", std::string{"brown"}}},
                {{"name", std::string{"Dog"}}, {"color", std::string{"white"}}}
            };
        }

        SECTION("example-schema:animalWithColor - quotes in values")
        {
            datastore.createListInstance("/example-schema:animalWithColor[name='D\"o\"g'][color=\"b'r'own\"]");
            node.first = "example-schema";
            node.second = "animalWithColor";
            expected = {
                {{"name", std::string{"D\"o\"g"}}, {"color", std::string{"b'r'own"}}}
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
                {{"name", enum_{"A"}}},
                {{"name", enum_{"B"}}},
                {{"name", enum_{"E"}}},
            };
        }

        SECTION("example-schema:org/example:people - nested list")
        {
            datastore.createListInstance("/example-schema:org[department='accounting']");
            datastore.createListInstance("/example-schema:org[department='sales']");
            datastore.createListInstance("/example-schema:org[department='programmers']");
            datastore.createListInstance("/example-schema:org[department='accounting']/people[name='Alice']");
            datastore.createListInstance("/example-schema:org[department='accounting']/people[name='Bob']");
            datastore.createListInstance("/example-schema:org[department='sales']/people[name='Alice']");
            datastore.createListInstance("/example-schema:org[department='sales']/people[name='Cyril']");
            datastore.createListInstance("/example-schema:org[department='sales']/people[name='Alice']/computers[type='laptop']");
            datastore.createListInstance("/example-schema:org[department='sales']/people[name='Alice']/computers[type='server']");
            datastore.createListInstance("/example-schema:org[department='sales']/people[name='Cyril']/computers[type='PC']");
            datastore.createListInstance("/example-schema:org[department='sales']/people[name='Cyril']/computers[type='server']");

            SECTION("outer list")
            {
                node.first = "example-schema";
                node.second = "org";
                expected = {
                    {{"department", std::string{"accounting"}}},
                    {{"department", std::string{"sales"}}},
                    {{"department", std::string{"programmers"}}},
                };
            }

            SECTION("nested list")
            {
                listElement_ list;
                list.m_name = "org";
                node.second = "people";
                SECTION("accounting department")
                {
                    list.m_keys = {
                        {"department", std::string{"accounting"}}
                    };
                    expected = {
                        {{"name", std::string{"Alice"}}},
                        {{"name", std::string{"Bob"}}},
                    };
                }
                SECTION("sales department")
                {
                    list.m_keys = {
                        {"department", std::string{"sales"}}
                    };
                    expected = {
                        {{"name", std::string{"Alice"}}},
                        {{"name", std::string{"Cyril"}}},
                    };
                }
                SECTION("programmers department")
                {
                    list.m_keys = {
                        {"department", std::string{"programmers"}}
                    };
                    expected = {
                    };
                }
                location.m_nodes.push_back(dataNode_{{"example-schema"}, list});
            }

            SECTION("THREE MF NESTED LISTS")
            {
                listElement_ listOrg;
                listOrg.m_name = "org";
                listOrg.m_keys = {
                    {"department", std::string{"sales"}}
                };

                listElement_ listPeople;
                node.second = "computers";

                SECTION("alice computers")
                {
                    listPeople.m_name = "people";
                    listPeople.m_keys = {
                        {"name", std::string{"Alice"}}
                    };
                    expected = {
                        {{"type", enum_{"laptop"}}},
                        {{"type", enum_{"server"}}},
                    };

                }

                SECTION("cyril computers")
                {
                    listPeople.m_name = "people";
                    listPeople.m_keys = {
                        {"name", std::string{"Cyril"}}
                    };
                    expected = {
                        {{"type", enum_{"PC"}}},
                        {{"type", enum_{"server"}}},
                    };
                }

                location.m_nodes.push_back(dataNode_{{"example-schema"}, listOrg});
                location.m_nodes.push_back(dataNode_{listPeople});

            }
        }

        SECTION("/other-module:parking-lot/example-schema:cars - list coming from an augment")
        {
            datastore.createListInstance("/other-module:parking-lot/example-schema:cars[id='1']");
            datastore.createListInstance("/other-module:parking-lot/example-schema:cars[id='2']");

            location.m_nodes.push_back(dataNode_{{"other-module"}, container_{"parking-lot"}});
            node.first = "example-schema";
            node.second = "cars";
            expected = {
                {{"id", int32_t{1}}},
                {{"id", int32_t{2}}},
            };

        }

        datastore.commitChanges();
        std::sort(expected.begin(), expected.end());
        auto keys = dataquery.listKeys(location, node);
        std::sort(keys.begin(), keys.end());
        REQUIRE(keys == expected);
    }

    waitForCompletionAndBitMore(seq1);
}
