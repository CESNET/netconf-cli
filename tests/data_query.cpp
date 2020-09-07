/*
 * Copyright (C) 2019 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/

#include <experimental/iterator>
#include "trompeloeil_doctest.hpp"

#ifdef sysrepo_BACKEND
#include "sysrepo_access.hpp"
#elif defined(netconf_BACKEND)
#include "netconf_access.hpp"
#elif defined(yang_BACKEND)
#include "yang_access.hpp"
#include "yang_access_test_vars.hpp"
#else
#error "Unknown backend"
#endif
#include "data_query.hpp"
#include "pretty_printers.hpp"
#include "sysrepo_subscription.hpp"
#include "utils.hpp"


TEST_CASE("data query")
{
    trompeloeil::sequence seq1;
    SysrepoSubscription subscriptionExample("example-schema");
    SysrepoSubscription subscriptionOther("other-module");

#ifdef sysrepo_BACKEND
    SysrepoAccess datastore(Datastore::Running);
#elif defined(netconf_BACKEND)
    const auto NETOPEER_SOCKET = getenv("NETOPEER_SOCKET");
    NetconfAccess datastore(NETOPEER_SOCKET);
#elif defined(yang_BACKEND)
    YangAccess datastore;
    datastore.addSchemaDir(schemaDir);
    datastore.addSchemaFile(exampleSchemaFile);
#else
#error "Unknown backend"
#endif

    DataQuery dataquery(datastore);

    SECTION("listKeys")
    {
        dataPath_ listPath;
        listPath.m_scope = Scope::Absolute;
        std::vector<ListInstance> expected;

        SECTION("example-schema:person")
        {
            datastore.createItem("/example-schema:person[name='Vaclav']");
            datastore.createItem("/example-schema:person[name='Tomas']");
            datastore.createItem("/example-schema:person[name='Jan Novak']");
            listPath.m_nodes.emplace_back(module_{"example-schema"}, list_{"person"});
            expected = {
                {{"name", std::string{"Jan Novak"}}},
                {{"name", std::string{"Tomas"}}},
                {{"name", std::string{"Vaclav"}}}
            };
        }

        SECTION("example-schema:person - no instances")
        {
            listPath.m_nodes.emplace_back(module_{"example-schema"}, list_{"person"});
            expected = {
            };
        }

        SECTION("example-schema:selectedNumbers")
        {
            datastore.createItem("/example-schema:selectedNumbers[value='45']");
            datastore.createItem("/example-schema:selectedNumbers[value='99']");
            datastore.createItem("/example-schema:selectedNumbers[value='127']");
            listPath.m_nodes.emplace_back(module_{"example-schema"}, list_{"selectedNumbers"});
            expected = {
                {{"value", int8_t{127}}},
                {{"value", int8_t{45}}},
                {{"value", int8_t{99}}}
            };
        }

        SECTION("example-schema:animalWithColor")
        {
            datastore.createItem("/example-schema:animalWithColor[name='Dog'][color='brown']");
            datastore.createItem("/example-schema:animalWithColor[name='Dog'][color='white']");
            datastore.createItem("/example-schema:animalWithColor[name='Cat'][color='grey']");
            listPath.m_nodes.emplace_back(module_{"example-schema"}, list_{"animalWithColor"});
            expected = {
                {{"name", std::string{"Cat"}}, {"color", std::string{"grey"}}},
                {{"name", std::string{"Dog"}}, {"color", std::string{"brown"}}},
                {{"name", std::string{"Dog"}}, {"color", std::string{"white"}}}
            };
        }

        SECTION("example-schema:animalWithColor - quotes in values")
        {
            datastore.createItem(R"(/example-schema:animalWithColor[name='D"o"g'][color="b'r'own"])");
            listPath.m_nodes.emplace_back(module_{"example-schema"}, list_{"animalWithColor"});
            expected = {
                {{"name", std::string{"D\"o\"g"}}, {"color", std::string{"b'r'own"}}}
            };
        }

        SECTION("example-schema:ports")
        {
            datastore.createItem("/example-schema:ports[name='A']");
            datastore.createItem("/example-schema:ports[name='B']");
            datastore.createItem("/example-schema:ports[name='E']");
            listPath.m_nodes.emplace_back(module_{"example-schema"}, list_{"ports"});
            expected = {
                {{"name", enum_{"A"}}},
                {{"name", enum_{"B"}}},
                {{"name", enum_{"E"}}},
            };
        }

        SECTION("example-schema:org/example:people - nested list")
        {
            datastore.createItem("/example-schema:org[department='accounting']");
            datastore.createItem("/example-schema:org[department='sales']");
            datastore.createItem("/example-schema:org[department='programmers']");
            datastore.createItem("/example-schema:org[department='accounting']/people[name='Alice']");
            datastore.createItem("/example-schema:org[department='accounting']/people[name='Bob']");
            datastore.createItem("/example-schema:org[department='sales']/people[name='Alice']");
            datastore.createItem("/example-schema:org[department='sales']/people[name='Cyril']");
            datastore.createItem("/example-schema:org[department='sales']/people[name='Alice']/computers[type='laptop']");
            datastore.createItem("/example-schema:org[department='sales']/people[name='Alice']/computers[type='server']");
            datastore.createItem("/example-schema:org[department='sales']/people[name='Cyril']/computers[type='PC']");
            datastore.createItem("/example-schema:org[department='sales']/people[name='Cyril']/computers[type='server']");

            SECTION("outer list")
            {
                listPath.m_nodes.emplace_back(module_{"example-schema"}, list_{"org"});
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
                listPath.m_nodes.emplace_back(module_{"example-schema"}, list);
                listPath.m_nodes.emplace_back(list_{"people"});
            }

            SECTION("THREE MF NESTED LISTS")
            {
                listElement_ listOrg;
                listOrg.m_name = "org";
                listOrg.m_keys = {
                    {"department", std::string{"sales"}}
                };

                listElement_ listPeople;

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

                listPath.m_nodes.emplace_back(module_{"example-schema"}, listOrg);
                listPath.m_nodes.emplace_back(listPeople);
                listPath.m_nodes.emplace_back(list_{"computers"});
            }
        }

        SECTION("/other-module:parking-lot/example-schema:cars - list coming from an augment")
        {
            datastore.createItem("/other-module:parking-lot/example-schema:cars[id='1']");
            datastore.createItem("/other-module:parking-lot/example-schema:cars[id='2']");

            listPath.m_nodes.emplace_back(module_{"other-module"}, container_{"parking-lot"});
            listPath.m_nodes.emplace_back(module_{"example-schema"}, list_{"cars"});
            expected = {
                {{"id", int32_t{1}}},
                {{"id", int32_t{2}}},
            };

        }

        datastore.commitChanges();
        std::sort(expected.begin(), expected.end());
        auto keys = dataquery.listKeys(listPath);
        std::sort(keys.begin(), keys.end());
        REQUIRE(keys == expected);
    }

    SECTION("empty datastore")
    {
        dataPath_ listPath;
        listPath.m_scope = Scope::Absolute;
        listPath.m_nodes.emplace_back(module_{"example-schema"}, list_{"person"});
        auto keys = dataquery.listKeys(listPath);
        REQUIRE(keys == std::vector<ListInstance>{});
    }

    waitForCompletionAndBitMore(seq1);
}
