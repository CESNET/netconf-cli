/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/

#include "trompeloeil_doctest.hpp"
#include <sysrepo-cpp/Session.hpp>
#include <sysrepo-cpp/utils/exception.hpp>
#include <sysrepo-cpp/utils/utils.hpp>
#include "proxy_datastore.hpp"
#include "yang_schema.hpp"

#ifdef sysrepo_BACKEND
#include "sysrepo_access.hpp"
using OnInvalidSchemaPathCreate = DatastoreException;
using OnInvalidSchemaPathDelete = DatastoreException;
using OnInvalidSchemaPathMove = sysrepo::ErrorWithCode;
using OnInvalidRpcPath = std::runtime_error;
using OnInvalidRpcInput = sysrepo::ErrorWithCode;
using OnKeyNotFound = void;
using OnExec = void;
using OnInvalidRequireInstance = DatastoreException;
#elif defined(netconf_BACKEND)
using OnInvalidSchemaPathCreate = std::runtime_error;
using OnInvalidSchemaPathDelete = std::runtime_error;
using OnInvalidSchemaPathMove = std::runtime_error;
using OnInvalidRpcPath = std::runtime_error;
using OnInvalidRpcInput = std::runtime_error;
using OnKeyNotFound = std::runtime_error;
using OnExec = void;
using OnInvalidRequireInstance = std::runtime_error;
#include "netconf_access.hpp"
#elif defined(yang_BACKEND)
#include <fstream>
#include "yang_access.hpp"
#include "yang_access_test_vars.hpp"
using OnInvalidSchemaPathCreate = DatastoreException;
using OnInvalidSchemaPathDelete = DatastoreException;
using OnInvalidSchemaPathMove = DatastoreException;
using OnInvalidRpcPath = DatastoreException;
using OnInvalidRpcInput = std::logic_error;
using OnKeyNotFound = DatastoreException;
using OnExec = std::logic_error;
using OnInvalidRequireInstance = std::runtime_error;
#else
#error "Unknown backend"
#endif
#include "pretty_printers.hpp"
#include "sysrepo_subscription.hpp"
#include "utils.hpp"

using namespace std::literals::string_literals;

class MockRecorder : public trompeloeil::mock_interface<Recorder> {
public:
    IMPLEMENT_MOCK5(write);
};

class MockDataSupplier : public trompeloeil::mock_interface<DataSupplier> {
public:
    IMPLEMENT_CONST_MOCK1(get_data);
};

namespace {
template <class ...> constexpr std::false_type always_false [[maybe_unused]] {};
template <class Exception, typename Callable> void catching(const Callable& what) {

    if constexpr (std::is_same_v<Exception, void>) {
        what();
    } else if constexpr (std::is_same<Exception, std::runtime_error>()) {
        // cannot use REQUIRE_THROWS_AS(..., Exception) directly because that one
        // needs an extra `typename` deep in the bowels of doctest
        REQUIRE_THROWS_AS(what(), std::runtime_error);
    } else if constexpr (std::is_same<Exception, std::logic_error>()) {
        REQUIRE_THROWS_AS(what(), std::logic_error);
    } else if constexpr (std::is_same<Exception, DatastoreException>()) {
        REQUIRE_THROWS_AS(what(), DatastoreException);
    } else if constexpr (std::is_same<Exception, sysrepo::ErrorWithCode>()) {
        REQUIRE_THROWS_AS(what(), sysrepo::ErrorWithCode);
    } else {
        static_assert(always_false<Exception>); // https://stackoverflow.com/a/53945549/2245623
    }
}
}

#if defined(yang_BACKEND)
class TestYangAccess : public YangAccess {
public:
    void commitChanges() override
    {
        YangAccess::commitChanges();
        dumpToSysrepo();
    }

    void copyConfig(const Datastore source, const Datastore destination) override
    {
        YangAccess::copyConfig(source, destination);
        dumpToSysrepo();
    }

private:
    void dumpToSysrepo()
    {
        {
            std::ofstream of(testConfigFile);
            of << dump(DataFormat::Xml);
        }
        auto command = std::string(sysrepocfgExecutable) + " --import=" + testConfigFile + " --format=xml --datastore=running --module=example-schema";
        if (std::system(command.c_str())) {
            throw std::runtime_error{"sysrepocfg import failed"};
        }
    }
};
#endif

TEST_CASE("setting/getting values")
{
    sysrepo::setLogLevelStderr(sysrepo::LogLevel::Information);
    trompeloeil::sequence seq1;
    MockRecorder mockRunning;
    MockRecorder mockStartup;

    sysrepo::Connection{}.sessionStart().copyConfig(sysrepo::Datastore::Startup, "example-schema", std::chrono::milliseconds(1000));

    SysrepoSubscription subRunning("example-schema", &mockRunning);
    SysrepoSubscription subStartup("example-schema", &mockStartup, sysrepo::Datastore::Startup);

#ifdef sysrepo_BACKEND
    SysrepoAccess datastore;
#elif defined(netconf_BACKEND)
    const auto NETOPEER_SOCKET = getenv("NETOPEER_SOCKET");
    NetconfAccess datastore(NETOPEER_SOCKET);
#elif defined(yang_BACKEND)
    TestYangAccess datastore;
    datastore.addSchemaDir(schemaDir);
    datastore.addSchemaFile(exampleSchemaFile);
#else
#error "Unknown backend"
#endif


    SECTION("set leafInt8 to -128")
    {
        REQUIRE_CALL(mockRunning, write(sysrepo::ChangeOperation::Created, "/example-schema:leafInt8", std::nullopt, "-128"s, std::nullopt));
        datastore.setLeaf("/example-schema:leafInt8", int8_t{-128});
        datastore.commitChanges();
    }

    SECTION("set leafInt16 to -32768")
    {
        REQUIRE_CALL(mockRunning, write(sysrepo::ChangeOperation::Created, "/example-schema:leafInt16", std::nullopt, "-32768"s, std::nullopt));
        datastore.setLeaf("/example-schema:leafInt16", int16_t{-32768});
        datastore.commitChanges();
    }

    SECTION("set leafInt32 to -2147483648")
    {
        REQUIRE_CALL(mockRunning, write(sysrepo::ChangeOperation::Created, "/example-schema:leafInt32", std::nullopt, "-2147483648"s, std::nullopt));
        datastore.setLeaf("/example-schema:leafInt32", int32_t{-2147483648});
        datastore.commitChanges();
    }

    SECTION("set leafInt64 to -50000000000")
    {
        REQUIRE_CALL(mockRunning, write(sysrepo::ChangeOperation::Created, "/example-schema:leafInt64", std::nullopt, "-50000000000"s, std::nullopt));
        datastore.setLeaf("/example-schema:leafInt64", int64_t{-50000000000});
        datastore.commitChanges();
    }

    SECTION("set leafUInt8 to 255")
    {
        REQUIRE_CALL(mockRunning, write(sysrepo::ChangeOperation::Created, "/example-schema:leafUInt8", std::nullopt, "255"s, std::nullopt));
        datastore.setLeaf("/example-schema:leafUInt8", uint8_t{255});
        datastore.commitChanges();
    }

    SECTION("set leafUInt16 to 65535")
    {
        REQUIRE_CALL(mockRunning, write(sysrepo::ChangeOperation::Created, "/example-schema:leafUInt16", std::nullopt, "65535"s, std::nullopt));
        datastore.setLeaf("/example-schema:leafUInt16", uint16_t{65535});
        datastore.commitChanges();
    }

    SECTION("set leafUInt32 to 4294967295")
    {
        REQUIRE_CALL(mockRunning, write(sysrepo::ChangeOperation::Created, "/example-schema:leafUInt32", std::nullopt, "4294967295"s, std::nullopt));
        datastore.setLeaf("/example-schema:leafUInt32", uint32_t{4294967295});
        datastore.commitChanges();
    }

    SECTION("set leafUInt64 to 50000000000")
    {
        REQUIRE_CALL(mockRunning, write(sysrepo::ChangeOperation::Created, "/example-schema:leafUInt64", std::nullopt, "50000000000"s, std::nullopt));
        datastore.setLeaf("/example-schema:leafUInt64", uint64_t{50000000000});
        datastore.commitChanges();
    }

    SECTION("set leafEnum to coze")
    {
        REQUIRE_CALL(mockRunning, write(sysrepo::ChangeOperation::Created, "/example-schema:leafEnum", std::nullopt, "coze"s, std::nullopt));
        datastore.setLeaf("/example-schema:leafEnum", enum_{"coze"});
        datastore.commitChanges();
    }

    SECTION("set leafDecimal to 123.544")
    {
        REQUIRE_CALL(mockRunning, write(sysrepo::ChangeOperation::Created, "/example-schema:leafDecimal", std::nullopt, "123.544"s, std::nullopt));
        datastore.setLeaf("/example-schema:leafDecimal", 123.544);
        datastore.commitChanges();
    }

    SECTION("set a string, then delete it")
    {
        REQUIRE_CALL(mockRunning, write(sysrepo::ChangeOperation::Created, "/example-schema:leafString", std::nullopt, "blah"s, std::nullopt));
        datastore.setLeaf("/example-schema:leafString", "blah"s);
        datastore.commitChanges();
        DatastoreAccess::Tree expected{{"/example-schema:leafString", "blah"s}};
        REQUIRE(datastore.getItems("/example-schema:leafString") == expected);

        REQUIRE_CALL(mockRunning, write(sysrepo::ChangeOperation::Deleted, "/example-schema:leafString", "blah"s, std::nullopt, std::nullopt));
        datastore.deleteItem("/example-schema:leafString");
        datastore.commitChanges();
        expected.clear();
        REQUIRE(datastore.getItems("/example-schema:leafString") == expected);
    }

    SECTION("set a string, then set it to something else without commiting")
    {
        REQUIRE_CALL(mockRunning, write(sysrepo::ChangeOperation::Created, "/example-schema:leafString", std::nullopt, "oops"s, std::nullopt));
        datastore.setLeaf("/example-schema:leafString", "blah"s);
        datastore.setLeaf("/example-schema:leafString", "oops"s);
        datastore.commitChanges();
        DatastoreAccess::Tree expected{{"/example-schema:leafString", "oops"s}};
        REQUIRE(datastore.getItems("/example-schema:leafString") == expected);

    }

    SECTION("set a non-existing leaf")
    {
        catching<OnInvalidSchemaPathCreate>([&] {
            datastore.setLeaf("/example-schema:non-existing", "what"s);
        });
    }

    SECTION("create presence container")
    {
        REQUIRE(datastore.dump(DataFormat::Json).find("example-schema:pContainer") == std::string::npos);
        REQUIRE_CALL(mockRunning, write(sysrepo::ChangeOperation::Created, "/example-schema:pContainer", std::nullopt, std::nullopt, std::nullopt));
        datastore.createItem("/example-schema:pContainer");
        datastore.commitChanges();
        REQUIRE(datastore.dump(DataFormat::Json).find("example-schema:pContainer") != std::string::npos);
    }

    SECTION("create/delete a list instance")
    {
        {
            REQUIRE_CALL(mockRunning, write(sysrepo::ChangeOperation::Created, "/example-schema:person[name='Nguyen']", std::nullopt, std::nullopt, std::nullopt));
            REQUIRE_CALL(mockRunning, write(sysrepo::ChangeOperation::Created, "/example-schema:person[name='Nguyen']/name", std::nullopt, "Nguyen"s, std::nullopt));
            datastore.createItem("/example-schema:person[name='Nguyen']");
            datastore.commitChanges();
        }
        {
            REQUIRE_CALL(mockRunning, write(sysrepo::ChangeOperation::Deleted, "/example-schema:person[name='Nguyen']", std::nullopt, std::nullopt, std::nullopt));
            REQUIRE_CALL(mockRunning, write(sysrepo::ChangeOperation::Deleted, "/example-schema:person[name='Nguyen']/name", "Nguyen"s, std::nullopt, std::nullopt));
            datastore.deleteItem("/example-schema:person[name='Nguyen']");
            datastore.commitChanges();
        }
    }

    SECTION("deleting non-existing list keys")
    {
        catching<OnKeyNotFound>([&] {
            datastore.deleteItem("/example-schema:person[name='non existing']");
            datastore.commitChanges();
        });
    }

    SECTION("accessing non-existing schema nodes as a list")
    {
        catching<OnInvalidSchemaPathCreate>([&] {
            datastore.createItem("/example-schema:non-existing-list[xxx='blah']");
            datastore.commitChanges();
        });
        catching<OnInvalidSchemaPathDelete>([&] {
            datastore.deleteItem("/example-schema:non-existing-list[xxx='non existing']");
            datastore.commitChanges();
        });
    }

    SECTION("leafref pointing to a key of a list")
    {
        {
            REQUIRE_CALL(mockRunning, write(sysrepo::ChangeOperation::Created, "/example-schema:person[name='Dan']", std::nullopt, std::nullopt, std::nullopt));
            REQUIRE_CALL(mockRunning, write(sysrepo::ChangeOperation::Created, "/example-schema:person[name='Dan']/name", std::nullopt, "Dan"s, std::nullopt));
            REQUIRE_CALL(mockRunning, write(sysrepo::ChangeOperation::Created, "/example-schema:person[name='Elfi']", std::nullopt, std::nullopt, std::nullopt));
            REQUIRE_CALL(mockRunning, write(sysrepo::ChangeOperation::Created, "/example-schema:person[name='Elfi']/name", std::nullopt, "Elfi"s, std::nullopt));
            REQUIRE_CALL(mockRunning, write(sysrepo::ChangeOperation::Created, "/example-schema:person[name='Kolafa']", std::nullopt, std::nullopt, std::nullopt));
            REQUIRE_CALL(mockRunning, write(sysrepo::ChangeOperation::Created, "/example-schema:person[name='Kolafa']/name", std::nullopt, "Kolafa"s, std::nullopt));
            datastore.createItem("/example-schema:person[name='Dan']");
            datastore.createItem("/example-schema:person[name='Elfi']");
            datastore.createItem("/example-schema:person[name='Kolafa']");
            datastore.commitChanges();
        }

        std::string value;
        SECTION("Dan")
        {
            value = "Dan";
        }

        SECTION("Elfi")
        {
            value = "Elfi";
        }

        SECTION("Kolafa")
        {
            value = "Kolafa";
        }

        datastore.setLeaf("/example-schema:bossPerson", value);
        {
            REQUIRE_CALL(mockRunning, write(sysrepo::ChangeOperation::Created, "/example-schema:bossPerson", std::nullopt, value, std::nullopt));
            datastore.commitChanges();
        }
        REQUIRE(datastore.getItems("/example-schema:bossPerson") == DatastoreAccess::Tree{{"/example-schema:bossPerson", value}});
    }

    SECTION("bool values get correctly represented as bools")
    {
        {
            REQUIRE_CALL(mockRunning, write(sysrepo::ChangeOperation::Created, "/example-schema:down", std::nullopt, "true"s, std::nullopt));
            datastore.setLeaf("/example-schema:down", bool{true});
            datastore.commitChanges();
        }

        DatastoreAccess::Tree expected{{"/example-schema:down", bool{true}}};
        REQUIRE(datastore.getItems("/example-schema:down") == expected);
    }

    SECTION("getting items from the whole module")
    {
        {
            REQUIRE_CALL(mockRunning, write(sysrepo::ChangeOperation::Created, "/example-schema:up", std::nullopt, "true"s, std::nullopt));
            REQUIRE_CALL(mockRunning, write(sysrepo::ChangeOperation::Created, "/example-schema:down", std::nullopt, "false"s, std::nullopt));
            datastore.setLeaf("/example-schema:up", bool{true});
            datastore.setLeaf("/example-schema:down", bool{false});
            datastore.commitChanges();
        }

        DatastoreAccess::Tree expected{
            {"/example-schema:up", bool{true}},
            {"/example-schema:down", bool{false}}
        };
        REQUIRE(datastore.getItems("/example-schema:*") == expected);
    }

    SECTION("getItems returns correct datatypes")
    {
        {
            REQUIRE_CALL(mockRunning, write(sysrepo::ChangeOperation::Created, "/example-schema:leafEnum", std::nullopt, "lol"s, std::nullopt));
            datastore.setLeaf("/example-schema:leafEnum", enum_{"lol"});
            datastore.commitChanges();
        }
        DatastoreAccess::Tree expected{{"/example-schema:leafEnum", enum_{"lol"}}};

        REQUIRE(datastore.getItems("/example-schema:leafEnum") == expected);
    }

    SECTION("getItems on a list")
    {
        {
            REQUIRE_CALL(mockRunning, write(sysrepo::ChangeOperation::Created, "/example-schema:person[name='Jan']", std::nullopt, std::nullopt, std::nullopt));
            REQUIRE_CALL(mockRunning, write(sysrepo::ChangeOperation::Created, "/example-schema:person[name='Jan']/name", std::nullopt, "Jan"s, std::nullopt));
            REQUIRE_CALL(mockRunning, write(sysrepo::ChangeOperation::Created, "/example-schema:person[name='Michal']", std::nullopt, std::nullopt, std::nullopt));
            REQUIRE_CALL(mockRunning, write(sysrepo::ChangeOperation::Created, "/example-schema:person[name='Michal']/name", std::nullopt, "Michal"s, std::nullopt));
            REQUIRE_CALL(mockRunning, write(sysrepo::ChangeOperation::Created, "/example-schema:person[name='Petr']", std::nullopt, std::nullopt, std::nullopt));
            REQUIRE_CALL(mockRunning, write(sysrepo::ChangeOperation::Created, "/example-schema:person[name='Petr']/name", std::nullopt, "Petr"s, std::nullopt));
            datastore.createItem("/example-schema:person[name='Jan']");
            datastore.createItem("/example-schema:person[name='Michal']");
            datastore.createItem("/example-schema:person[name='Petr']");
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
            REQUIRE_CALL(mockRunning, write(sysrepo::ChangeOperation::Created, "/example-schema:pContainer", std::nullopt, std::nullopt, std::nullopt));
            datastore.createItem("/example-schema:pContainer");
            datastore.commitChanges();
        }
        expected = {
            {"/example-schema:pContainer", special_{SpecialValue::PresenceContainer}}
        };
        REQUIRE(datastore.getItems("/example-schema:pContainer") == expected);

        // Make sure it's not there after we delete it
        {
            REQUIRE_CALL(mockRunning, write(sysrepo::ChangeOperation::Deleted, "/example-schema:pContainer", std::nullopt, std::nullopt, std::nullopt));
            datastore.deleteItem("/example-schema:pContainer");
            datastore.commitChanges();
        }
        expected = {};
        REQUIRE(datastore.getItems("/example-schema:pContainer") == expected);
    }

    SECTION("creating a non-existing schema node as a container")
    {
        catching<OnInvalidSchemaPathCreate>([&] {
            datastore.createItem("/example-schema:non-existing-presence-container");
            datastore.commitChanges();
        });
    }

    SECTION("deleting a non-existing schema node as a container or leaf")
    {
        catching<OnInvalidSchemaPathDelete>([&] {
            datastore.deleteItem("/example-schema:non-existing-presence-container");
            datastore.commitChanges();
        });
    }

    SECTION("nested presence container")
    {
        DatastoreAccess::Tree expected;
        // Make sure it's not there before we create it
        REQUIRE(datastore.getItems("/example-schema:inventory/stuff") == expected);
        {
            REQUIRE_CALL(mockRunning, write(sysrepo::ChangeOperation::Created, "/example-schema:inventory/stuff", std::nullopt, std::nullopt, std::nullopt));
            datastore.createItem("/example-schema:inventory/stuff");
            datastore.commitChanges();
        }
        expected = {
            {"/example-schema:inventory/stuff", special_{SpecialValue::PresenceContainer}}
        };
        REQUIRE(datastore.getItems("/example-schema:inventory/stuff") == expected);
        {
            REQUIRE_CALL(mockRunning, write(sysrepo::ChangeOperation::Deleted, "/example-schema:inventory/stuff", std::nullopt, std::nullopt, std::nullopt));
            datastore.deleteItem("/example-schema:inventory/stuff");
            datastore.commitChanges();
        }
        expected = {};
        REQUIRE(datastore.getItems("/example-schema:inventory/stuff") == expected);
    }

    SECTION("floats")
    {
        datastore.setLeaf("/example-schema:leafDecimal", 123.4);
        REQUIRE_CALL(mockRunning, write(sysrepo::ChangeOperation::Created, "/example-schema:leafDecimal", std::nullopt, "123.4"s, std::nullopt));
        datastore.commitChanges();
        DatastoreAccess::Tree expected{
            {"/example-schema:leafDecimal", 123.4},
        };
        REQUIRE(datastore.getItems("/example-schema:leafDecimal") == expected);
    }

    SECTION("unions")
    {
        datastore.setLeaf("/example-schema:unionIntString", int32_t{10});
        REQUIRE_CALL(mockRunning, write(sysrepo::ChangeOperation::Created, "/example-schema:unionIntString", std::nullopt, "10"s, std::nullopt));
        datastore.commitChanges();
        DatastoreAccess::Tree expected{
            {"/example-schema:unionIntString", int32_t{10}},
        };
        REQUIRE(datastore.getItems("/example-schema:unionIntString") == expected);
    }

    SECTION("identityref")
    {
        datastore.setLeaf("/example-schema:beast", identityRef_{"example-schema", "Mammal"});
        REQUIRE_CALL(mockRunning, write(sysrepo::ChangeOperation::Created, "/example-schema:beast", std::nullopt, "example-schema:Mammal"s, std::nullopt));
        datastore.commitChanges();
        DatastoreAccess::Tree expected{
            {"/example-schema:beast", identityRef_{"example-schema", "Mammal"}},
        };
        REQUIRE(datastore.getItems("/example-schema:beast") == expected);

        datastore.setLeaf("/example-schema:beast", identityRef_{"Whale"});
        REQUIRE_CALL(mockRunning, write(sysrepo::ChangeOperation::Modified, "/example-schema:beast", "example-schema:Mammal", "example-schema:Whale"s, std::nullopt));
        datastore.commitChanges();
        expected = {
            {"/example-schema:beast", identityRef_{"example-schema", "Whale"}},
        };
        REQUIRE(datastore.getItems("/example-schema:beast") == expected);
    }

    SECTION("binary")
    {
        datastore.setLeaf("/example-schema:blob", binary_{"cHduegByIQ=="s});
        REQUIRE_CALL(mockRunning, write(sysrepo::ChangeOperation::Created, "/example-schema:blob", std::nullopt, "cHduegByIQ=="s, std::nullopt));
        datastore.commitChanges();
        DatastoreAccess::Tree expected{
            {"/example-schema:blob", binary_{"cHduegByIQ=="s}},
        };
        REQUIRE(datastore.getItems("/example-schema:blob") == expected);
    }

    SECTION("empty")
    {
        datastore.setLeaf("/example-schema:dummy", empty_{});
        REQUIRE_CALL(mockRunning, write(sysrepo::ChangeOperation::Created, "/example-schema:dummy", std::nullopt, ""s, std::nullopt));
        datastore.commitChanges();
        DatastoreAccess::Tree expected{
            {"/example-schema:dummy", empty_{}},
        };
        REQUIRE(datastore.getItems("/example-schema:dummy") == expected);
    }

    SECTION("bits")
    {
        datastore.setLeaf("/example-schema:flags", bits_{{"sign", "carry"}});
        REQUIRE_CALL(mockRunning, write(sysrepo::ChangeOperation::Created, "/example-schema:flags", std::nullopt, "carry sign"s, std::nullopt));
        datastore.commitChanges();
        DatastoreAccess::Tree expected{
            {"/example-schema:flags", bits_{{"carry", "sign"}}},
        };
        REQUIRE(datastore.getItems("/example-schema:flags") == expected);
    }

    SECTION("instance-identifier")
    {
        SECTION("simple")
        {
            datastore.setLeaf("/example-schema:lol/up", true);
            datastore.setLeaf("/example-schema:iid-valid", instanceIdentifier_{"/example-schema:lol/up"});
            REQUIRE_CALL(mockRunning, write(sysrepo::ChangeOperation::Created, "/example-schema:lol/up", std::nullopt, "true"s, std::nullopt));
            REQUIRE_CALL(mockRunning, write(sysrepo::ChangeOperation::Created, "/example-schema:iid-valid", std::nullopt, "/example-schema:lol/up"s, std::nullopt));
            datastore.commitChanges();
            DatastoreAccess::Tree expected{
                {"/example-schema:iid-valid", instanceIdentifier_{"/example-schema:lol/up"}},
            };
            REQUIRE(datastore.getItems("/example-schema:iid-valid") == expected);
        }

        SECTION("to a non-existing list without require-instance")
        {
            datastore.setLeaf("/example-schema:iid-relaxed", instanceIdentifier_{"/example-schema:ports[name='A']/shutdown"});
            REQUIRE_CALL(mockRunning, write(sysrepo::ChangeOperation::Created, "/example-schema:iid-relaxed", std::nullopt, "/example-schema:ports[name='A']/shutdown"s, std::nullopt));
            datastore.commitChanges();
            DatastoreAccess::Tree expected{
                {"/example-schema:iid-relaxed", instanceIdentifier_{"/example-schema:ports[name='A']/shutdown"}},
            };
            REQUIRE(datastore.getItems("/example-schema:iid-relaxed") == expected);
        }

        SECTION("to a non-existing list with require-instance")
        {
            catching<OnInvalidRequireInstance>([&] {
                datastore.setLeaf("/example-schema:iid-valid", instanceIdentifier_{"/example-schema:ports[name='A']/shutdown"});
                datastore.commitChanges();
            });
            datastore.discardChanges();
        }
    }


#if not defined(yang_BACKEND)
    SECTION("operational data")
    {
        MockDataSupplier mockOpsData;
        OperationalDataSubscription opsDataSub("example-schema", "/example-schema:temperature", mockOpsData);
        OperationalDataSubscription opsDataSub2("example-schema", "/example-schema:users", mockOpsData);
        DatastoreAccess::Tree expected;
        std::string xpath;

        SECTION("temperature")
        {
            expected = {{"/example-schema:temperature", int32_t{22}}};
            xpath = "/example-schema:temperature";
        }

        SECTION("key-less lists")
        {
            expected = {
                {"/example-schema:users/userList[1]", special_{SpecialValue::List}},
                {"/example-schema:users/userList[1]/name", std::string{"John"}},
                {"/example-schema:users/userList[1]/otherfield", std::string{"LOL"}},
                {"/example-schema:users/userList[2]", special_{SpecialValue::List}},
                {"/example-schema:users/userList[2]/name", std::string{"Foo"}},
                {"/example-schema:users/userList[2]/otherfield", std::string{"Bar"}},
            };
            xpath = "/example-schema:users";
        }
        REQUIRE_CALL(mockOpsData, get_data(xpath)).RETURN(expected);
        REQUIRE(datastore.getItems(xpath) == expected);
    }
#endif

    SECTION("leaf list")
    {
        DatastoreAccess::Tree expected;
        REQUIRE_CALL(mockRunning, write(sysrepo::ChangeOperation::Created, "/example-schema:addresses[.='0.0.0.0']", std::nullopt, "0.0.0.0"s, std::nullopt));
        REQUIRE_CALL(mockRunning, write(sysrepo::ChangeOperation::Created, "/example-schema:addresses[.='127.0.0.1']", std::nullopt, "127.0.0.1"s, std::nullopt));
        datastore.createItem("/example-schema:addresses[.='0.0.0.0']");
        datastore.createItem("/example-schema:addresses[.='127.0.0.1']");
        datastore.commitChanges();
        expected = {
            {"/example-schema:addresses", special_{SpecialValue::LeafList}},
            {"/example-schema:addresses[.='0.0.0.0']", "0.0.0.0"s},
            {"/example-schema:addresses[.='127.0.0.1']", "127.0.0.1"s},
        };
        REQUIRE(datastore.getItems("/example-schema:addresses") == expected);

        REQUIRE_CALL(mockRunning, write(sysrepo::ChangeOperation::Deleted, "/example-schema:addresses[.='0.0.0.0']", "0.0.0.0"s, std::nullopt, std::nullopt));
        datastore.deleteItem("/example-schema:addresses[.='0.0.0.0']");
        datastore.commitChanges();
        expected = {
            {"/example-schema:addresses", special_{SpecialValue::LeafList}},
            {"/example-schema:addresses[.='127.0.0.1']", "127.0.0.1"s},
        };
        REQUIRE(datastore.getItems("/example-schema:addresses") == expected);

        REQUIRE_CALL(mockRunning, write(sysrepo::ChangeOperation::Deleted, "/example-schema:addresses[.='127.0.0.1']", "127.0.0.1"s, std::nullopt, std::nullopt));
        datastore.deleteItem("/example-schema:addresses[.='127.0.0.1']");
        datastore.commitChanges();
        expected = {};
        REQUIRE(datastore.getItems("/example-schema:addresses") == expected);
    }

    SECTION("deleting a non-existing leaf-list")
    {
        catching<OnKeyNotFound>([&] {
            datastore.deleteItem("/example-schema:addresses[.='non-existing']");
            datastore.commitChanges();
        });
    }

    SECTION("accessing a non-existing schema node as a leaf-list")
    {
        catching<OnInvalidSchemaPathCreate>([&] {
            datastore.createItem("/example-schema:non-existing[.='non-existing']");
            datastore.commitChanges();
        });

        catching<OnInvalidSchemaPathDelete>([&] {
            datastore.deleteItem("/example-schema:non-existing[.='non-existing']");
            datastore.commitChanges();
        });
    }

    SECTION("copying data from startup refreshes the data")
    {
        {
            REQUIRE(datastore.getItems("/example-schema:leafInt16") == DatastoreAccess::Tree{});
            REQUIRE_CALL(mockRunning, write(sysrepo::ChangeOperation::Created, "/example-schema:leafInt16", std::nullopt, "123"s, std::nullopt));
            datastore.setLeaf("/example-schema:leafInt16", int16_t{123});
            datastore.commitChanges();
        }
        REQUIRE(datastore.getItems("/example-schema:leafInt16") == DatastoreAccess::Tree{{"/example-schema:leafInt16", int16_t{123}}});
        REQUIRE_CALL(mockRunning, write(sysrepo::ChangeOperation::Deleted, "/example-schema:leafInt16", "123"s, std::nullopt, std::nullopt));
        datastore.copyConfig(Datastore::Startup, Datastore::Running);
        REQUIRE(datastore.getItems("/example-schema:leafInt16") == DatastoreAccess::Tree{});
    }

    SECTION("moving leaflist instances")
    {
        DatastoreAccess::Tree expected;
        {
            REQUIRE_CALL(mockRunning, write(sysrepo::ChangeOperation::Created, "/example-schema:protocols[.='http']", "", "http"s, std::nullopt));
            REQUIRE_CALL(mockRunning, write(sysrepo::ChangeOperation::Created, "/example-schema:protocols[.='ftp']", "http"s, "ftp"s, std::nullopt));
            REQUIRE_CALL(mockRunning, write(sysrepo::ChangeOperation::Created, "/example-schema:protocols[.='pop3']", "ftp"s, "pop3"s, std::nullopt));
            datastore.createItem("/example-schema:protocols[.='http']");
            datastore.createItem("/example-schema:protocols[.='ftp']");
            datastore.createItem("/example-schema:protocols[.='pop3']");
            datastore.commitChanges();
            expected = {
                {"/example-schema:protocols", special_{SpecialValue::LeafList}},
                {"/example-schema:protocols[.='http']", "http"s},
                {"/example-schema:protocols[.='ftp']", "ftp"s},
                {"/example-schema:protocols[.='pop3']", "pop3"s},
            };
            REQUIRE(datastore.getItems("/example-schema:protocols") == expected);
        }

        std::string sourcePath;
        SECTION("begin")
        {
            REQUIRE_CALL(mockRunning, write(sysrepo::ChangeOperation::Moved, "/example-schema:protocols[.='pop3']", ""s, "pop3"s, std::nullopt));
            sourcePath = "/example-schema:protocols[.='pop3']";
            datastore.moveItem(sourcePath, yang::move::Absolute::Begin);
            datastore.commitChanges();
            expected = {
                {"/example-schema:protocols", special_{SpecialValue::LeafList}},
                {"/example-schema:protocols[.='pop3']", "pop3"s},
                {"/example-schema:protocols[.='http']", "http"s},
                {"/example-schema:protocols[.='ftp']", "ftp"s},
            };
            REQUIRE(datastore.getItems("/example-schema:protocols") == expected);
        }

        SECTION("end")
        {
            sourcePath = "/example-schema:protocols[.='http']";

#if defined(yang_BACKEND) || defined(netconf_BACKEND)
            // Due to the libyang diff algorithm being imperfect, the move operations differ between backends.
            // The same applies for the stuff below.
            // https://github.com/sysrepo/sysrepo/issues/2732
            REQUIRE_CALL(mockRunning, write(sysrepo::ChangeOperation::Moved, "/example-schema:protocols[.='ftp']", ""s, "ftp"s, std::nullopt));
            REQUIRE_CALL(mockRunning, write(sysrepo::ChangeOperation::Moved, "/example-schema:protocols[.='pop3']", "ftp"s, "pop3"s, std::nullopt));
#else
            REQUIRE_CALL(mockRunning, write(sysrepo::ChangeOperation::Moved, "/example-schema:protocols[.='http']", "pop3"s, "http"s, std::nullopt));
#endif
            datastore.moveItem(sourcePath, yang::move::Absolute::End);
            datastore.commitChanges();
            expected = {
                {"/example-schema:protocols", special_{SpecialValue::LeafList}},
                {"/example-schema:protocols[.='ftp']", "ftp"s},
                {"/example-schema:protocols[.='pop3']", "pop3"s},
                {"/example-schema:protocols[.='http']", "http"s},
            };
            REQUIRE(datastore.getItems("/example-schema:protocols") == expected);
        }

        SECTION("after")
        {
            sourcePath = "/example-schema:protocols[.='http']";
#if defined(yang_BACKEND) || defined(netconf_BACKEND)
            // see the test for "end" for explanation if this #ifdef
            REQUIRE_CALL(mockRunning, write(sysrepo::ChangeOperation::Moved, "/example-schema:protocols[.='ftp']", ""s, "ftp"s, std::nullopt));
#else
            REQUIRE_CALL(mockRunning, write(sysrepo::ChangeOperation::Moved, "/example-schema:protocols[.='http']", "ftp"s, "http"s, std::nullopt));
#endif
            datastore.moveItem(sourcePath, yang::move::Relative{yang::move::Relative::Position::After, {{".", "ftp"s}}});
            datastore.commitChanges();
            expected = {
                {"/example-schema:protocols", special_{SpecialValue::LeafList}},
                {"/example-schema:protocols[.='ftp']", "ftp"s},
                {"/example-schema:protocols[.='http']", "http"s},
                {"/example-schema:protocols[.='pop3']", "pop3"s},
            };
            REQUIRE(datastore.getItems("/example-schema:protocols") == expected);
        }

        SECTION("before")
        {
            sourcePath = "/example-schema:protocols[.='http']";
#if defined(yang_BACKEND) || defined(netconf_BACKEND)
            // see the test for "end" for explanation if this #ifdef
            REQUIRE_CALL(mockRunning, write(sysrepo::ChangeOperation::Moved, "/example-schema:protocols[.='ftp']", ""s, "ftp"s, std::nullopt));
#else
            REQUIRE_CALL(mockRunning, write(sysrepo::ChangeOperation::Moved, "/example-schema:protocols[.='http']", "ftp"s, "http"s, std::nullopt));
#endif
            datastore.moveItem(sourcePath, yang::move::Relative{yang::move::Relative::Position::Before, {{".", "pop3"s}}});
            datastore.commitChanges();
            expected = {
                {"/example-schema:protocols", special_{SpecialValue::LeafList}},
                {"/example-schema:protocols[.='ftp']", "ftp"s},
                {"/example-schema:protocols[.='http']", "http"s},
                {"/example-schema:protocols[.='pop3']", "pop3"s},
            };
            REQUIRE(datastore.getItems("/example-schema:protocols") == expected);
        }
    }

    SECTION("moving non-existing schema nodes")
    {
        catching<OnInvalidSchemaPathMove>([&] {
            datastore.moveItem("/example-schema:non-existing", yang::move::Absolute::Begin);
            datastore.commitChanges();
        });
    }

    SECTION("moving list instances")
    {
        DatastoreAccess::Tree expected;
        {
            REQUIRE_CALL(mockRunning, write(sysrepo::ChangeOperation::Created, "/example-schema:players[name='John']", std::nullopt, std::nullopt, ""s));
            REQUIRE_CALL(mockRunning, write(sysrepo::ChangeOperation::Created, "/example-schema:players[name='John']/name", std::nullopt, "John"s, std::nullopt));
            REQUIRE_CALL(mockRunning, write(sysrepo::ChangeOperation::Created, "/example-schema:players[name='Eve']", std::nullopt, std::nullopt, "[name='John']"));
            REQUIRE_CALL(mockRunning, write(sysrepo::ChangeOperation::Created, "/example-schema:players[name='Eve']/name", std::nullopt, "Eve"s, std::nullopt));
            REQUIRE_CALL(mockRunning, write(sysrepo::ChangeOperation::Created, "/example-schema:players[name='Adam']", std::nullopt, std::nullopt, "[name='Eve']"));
            REQUIRE_CALL(mockRunning, write(sysrepo::ChangeOperation::Created, "/example-schema:players[name='Adam']/name", std::nullopt, "Adam"s, std::nullopt));
            datastore.createItem("/example-schema:players[name='John']");
            datastore.createItem("/example-schema:players[name='Eve']");
            datastore.createItem("/example-schema:players[name='Adam']");
            datastore.commitChanges();
            expected = {
                {"/example-schema:players[name='John']", special_{SpecialValue::List}},
                {"/example-schema:players[name='John']/name", "John"s},
                {"/example-schema:players[name='Eve']", special_{SpecialValue::List}},
                {"/example-schema:players[name='Eve']/name", "Eve"s},
                {"/example-schema:players[name='Adam']", special_{SpecialValue::List}},
                {"/example-schema:players[name='Adam']/name", "Adam"s},
            };
            REQUIRE(datastore.getItems("/example-schema:players") == expected);
        }

        std::string sourcePath;
        SECTION("begin")
        {
            sourcePath = "/example-schema:players[name='Adam']";
            REQUIRE_CALL(mockRunning, write(sysrepo::ChangeOperation::Moved, "/example-schema:players[name='Adam']", std::nullopt, std::nullopt, ""s));
            datastore.moveItem(sourcePath, yang::move::Absolute::Begin);
            datastore.commitChanges();
            expected = {
                {"/example-schema:players[name='Adam']", special_{SpecialValue::List}},
                {"/example-schema:players[name='Adam']/name", "Adam"s},
                {"/example-schema:players[name='John']", special_{SpecialValue::List}},
                {"/example-schema:players[name='John']/name", "John"s},
                {"/example-schema:players[name='Eve']", special_{SpecialValue::List}},
                {"/example-schema:players[name='Eve']/name", "Eve"s},
            };
            REQUIRE(datastore.getItems("/example-schema:players") == expected);
        }

        SECTION("end")
        {
            sourcePath = "/example-schema:players[name='John']";
#if defined(yang_BACKEND) || defined(netconf_BACKEND)
            // TODO: see TODO comment in leaflist/end
            // Although these make much less sense
            REQUIRE_CALL(mockRunning, write(sysrepo::ChangeOperation::Moved, "/example-schema:players[name='Eve']", std::nullopt, std::nullopt, ""));
            REQUIRE_CALL(mockRunning, write(sysrepo::ChangeOperation::Moved, "/example-schema:players[name='Adam']", std::nullopt, std::nullopt, "[name='Eve']"));
#else
            REQUIRE_CALL(mockRunning, write(sysrepo::ChangeOperation::Moved, "/example-schema:players[name='John']", std::nullopt, std::nullopt, "[name='Adam']"));
#endif
            datastore.moveItem(sourcePath, yang::move::Absolute::End);
            datastore.commitChanges();
            expected = {
                {"/example-schema:players[name='Eve']", special_{SpecialValue::List}},
                {"/example-schema:players[name='Eve']/name", "Eve"s},
                {"/example-schema:players[name='Adam']", special_{SpecialValue::List}},
                {"/example-schema:players[name='Adam']/name", "Adam"s},
                {"/example-schema:players[name='John']", special_{SpecialValue::List}},
                {"/example-schema:players[name='John']/name", "John"s},
            };
            REQUIRE(datastore.getItems("/example-schema:players") == expected);
        }

        SECTION("after")
        {
            sourcePath = "/example-schema:players[name='John']";
#if defined(yang_BACKEND) || defined(netconf_BACKEND)
            // TODO: see TODO comment in leaflist/end
            // Although these make much less sense
            REQUIRE_CALL(mockRunning, write(sysrepo::ChangeOperation::Moved, "/example-schema:players[name='Eve']", std::nullopt, std::nullopt, ""));
#else
            REQUIRE_CALL(mockRunning, write(sysrepo::ChangeOperation::Moved, "/example-schema:players[name='John']", std::nullopt, std::nullopt, "[name='Eve']"));
#endif
            datastore.moveItem(sourcePath, yang::move::Relative{yang::move::Relative::Position::After, {{"name", "Eve"s}}});
            datastore.commitChanges();
            expected = {
                {"/example-schema:players[name='Eve']", special_{SpecialValue::List}},
                {"/example-schema:players[name='Eve']/name", "Eve"s},
                {"/example-schema:players[name='John']", special_{SpecialValue::List}},
                {"/example-schema:players[name='John']/name", "John"s},
                {"/example-schema:players[name='Adam']", special_{SpecialValue::List}},
                {"/example-schema:players[name='Adam']/name", "Adam"s},
            };
            REQUIRE(datastore.getItems("/example-schema:players") == expected);
        }

        SECTION("before")
        {
            sourcePath = "/example-schema:players[name='John']";
#if defined(yang_BACKEND) || defined(netconf_BACKEND)
            // TODO: see TODO comment in leaflist/end
            // Although these make much less sense
            REQUIRE_CALL(mockRunning, write(sysrepo::ChangeOperation::Moved, "/example-schema:players[name='Eve']", std::nullopt, std::nullopt, ""));
#else
            REQUIRE_CALL(mockRunning, write(sysrepo::ChangeOperation::Moved, "/example-schema:players[name='John']", std::nullopt, std::nullopt, "[name='Eve']"));
#endif
            datastore.moveItem(sourcePath, yang::move::Relative{yang::move::Relative::Position::Before, {{"name", "Adam"s}}});
            datastore.commitChanges();
            expected = {
                {"/example-schema:players[name='Eve']", special_{SpecialValue::List}},
                {"/example-schema:players[name='Eve']/name", "Eve"s},
                {"/example-schema:players[name='John']", special_{SpecialValue::List}},
                {"/example-schema:players[name='John']/name", "John"s},
                {"/example-schema:players[name='Adam']", special_{SpecialValue::List}},
                {"/example-schema:players[name='Adam']/name", "Adam"s},
            };
            REQUIRE(datastore.getItems("/example-schema:players") == expected);
        }
    }

    SECTION("getting /")
    {
        {
            REQUIRE_CALL(mockRunning, write(sysrepo::ChangeOperation::Created, "/example-schema:leafInt32", std::nullopt, "64"s, std::nullopt));
            datastore.setLeaf("/example-schema:leafInt32", 64);
            datastore.commitChanges();
        }

        DatastoreAccess::Tree expected{
            {"/example-schema:leafInt32", 64}
        };
        // This tests if we at least get the data WE added.
        REQUIRE(std::all_of(expected.begin(), expected.end(), [items = datastore.getItems("/")] (const auto& item) {
            return std::find(items.begin(), items.end(), item) != items.end();
        }));
    }

    SECTION("setting and removing without commit")
    {
        datastore.setLeaf("/example-schema:leafInt32", 64);
        datastore.deleteItem("/example-schema:leafInt32");
    }

    SECTION("two key lists")
    {
        REQUIRE_CALL(mockRunning, write(sysrepo::ChangeOperation::Created, "/example-schema:point[x='12'][y='10']", std::nullopt, std::nullopt, std::nullopt));
        REQUIRE_CALL(mockRunning, write(sysrepo::ChangeOperation::Created, "/example-schema:point[x='12'][y='10']/x", std::nullopt, "12"s, std::nullopt));
        REQUIRE_CALL(mockRunning, write(sysrepo::ChangeOperation::Created, "/example-schema:point[x='12'][y='10']/y", std::nullopt, "10"s, std::nullopt));
        datastore.createItem("/example-schema:point[x='12'][y='10']");
        datastore.commitChanges();
        REQUIRE(datastore.dump(DataFormat::Json).find("example-schema:point") != std::string::npos);
    }

    waitForCompletionAndBitMore(seq1);
}

struct ActionCb {
    sysrepo::ErrorCode operator()(
        [[maybe_unused]] sysrepo::Session session,
        [[maybe_unused]] uint32_t subscriptionId,
        const std::string& xpath,
        [[maybe_unused]] const libyang::DataNode input,
        [[maybe_unused]] sysrepo::Event event,
        [[maybe_unused]] uint32_t requestId,
        libyang::DataNode output)
    {
        if (session.getContext().findPath(xpath.data()).path() == "/example-schema:ports/shutdown") {
            // `xpath` holds the subscription xpath which won't have list keys. We need the path with list keys and
            // we'll find that in the input.
            auto inputPath = input.findXPath("/example-schema:ports/shutdown").front().path();
            output.newPath(joinPaths(inputPath, "success"), "true", libyang::CreationOptions::Output);
            return sysrepo::ErrorCode::Ok;
        }
        throw std::runtime_error("unrecognized RPC");
    }
};

struct RpcCb {
    sysrepo::ErrorCode operator()(
        [[maybe_unused]] sysrepo::Session session,
        [[maybe_unused]] uint32_t subscriptionId,
        const std::string& xpath,
        const libyang::DataNode input,
        [[maybe_unused]] sysrepo::Event event,
        [[maybe_unused]] uint32_t requestId,
        libyang::DataNode output)
    {
        const auto nukes = "/example-schema:launch-nukes"s;
        if (xpath == "/example-schema:noop"s || xpath == "/example-schema:fire"s) {
            return sysrepo::ErrorCode::Ok;
        }

        if (xpath == nukes) {
            uint64_t kilotons = 0;
            bool hasCities = false;
            for (const auto& inputNode : input.childrenDfs()) {
                if (inputNode.path() == nukes) {
                    continue; // ignore, top-level RPC
                }
                if (inputNode.path() == nukes + "/payload") {
                    continue; // ignore, container
                }
                if (inputNode.path() == nukes + "/description") {
                    continue; // unused
                }

                if (inputNode.path() == nukes + "/payload/kilotons") {
                    kilotons = std::get<uint64_t>(inputNode.asTerm().value());
                } else if (inputNode.path().find(nukes + "/cities") == 0) {
                    hasCities = true;
                } else {
                    throw std::runtime_error("RPC launch-nukes: unexpected input "s + inputNode.path());
                }
            }
            if (kilotons == 333'666) {
                // magic, just do not generate any output. This is important because the NETCONF RPC returns just <ok/>.
                return sysrepo::ErrorCode::Ok;
            }
            output.newPath(nukes + "/blast-radius", "33666", libyang::CreationOptions::Output);
            output.newPath(nukes + "/actual-yield", std::to_string(static_cast<uint64_t>(1.33 * kilotons)), libyang::CreationOptions::Output);
            if (hasCities) {
                output.newPath(nukes + "/damaged-places/targets[city='London']/city", "London", libyang::CreationOptions::Output);
                output.newPath(nukes + "/damaged-places/targets[city='Berlin']/city", "Berlin", libyang::CreationOptions::Output);
            }
            return sysrepo::ErrorCode::Ok;
        }
        throw std::runtime_error("unrecognized RPC");
    }
};

TEST_CASE("rpc/action")
{
    trompeloeil::sequence seq1;

#ifdef sysrepo_BACKEND
    auto datastore = std::make_shared<SysrepoAccess>();
#elif defined(netconf_BACKEND)
    const auto NETOPEER_SOCKET = getenv("NETOPEER_SOCKET");
    auto datastore = std::make_shared<NetconfAccess>(NETOPEER_SOCKET);
#elif defined(yang_BACKEND)
    auto datastore = std::make_shared<YangAccess>();
    datastore->addSchemaDir(schemaDir);
    datastore->addSchemaFile(exampleSchemaFile);
#else
#error "Unknown backend"
#endif

    sysrepo::setLogLevelStderr(sysrepo::LogLevel::Information);

    auto srSubscription = sysrepo::Connection{}.sessionStart().onRPCAction("/example-schema:noop", RpcCb{});
    srSubscription.onRPCAction("/example-schema:launch-nukes", RpcCb{});
    srSubscription.onRPCAction("/example-schema:fire", RpcCb{});
    srSubscription.onRPCAction("/example-schema:ports/shutdown", ActionCb{});

    SysrepoSubscription subscription("example-schema", nullptr);

    SECTION("rpc")
    {
        auto createTemporaryDatastore = [](const std::shared_ptr<DatastoreAccess>& datastore) {
            return std::make_shared<YangAccess>(std::static_pointer_cast<YangSchema>(datastore->schema()));
        };
        ProxyDatastore proxyDatastore(datastore, createTemporaryDatastore);

        // ProxyDatastore cannot easily read DatastoreAccess::Tree, so we need to set the input via create/setLeaf/etc.
        SECTION("valid")
        {
            std::string rpc;
            DatastoreAccess::Tree input, output;

            SECTION("noop")
            {
                rpc = "/example-schema:noop";
                proxyDatastore.initiate(rpc);
            }

            SECTION("small nuke")
            {
                rpc = "/example-schema:launch-nukes";
                input = {
                    {joinPaths(rpc, "description"), "dummy"s},
                    {joinPaths(rpc, "payload/kilotons"), uint64_t{333'666}},
                };
                proxyDatastore.initiate(rpc);
                proxyDatastore.setLeaf("/example-schema:launch-nukes/example-schema:payload/example-schema:kilotons", uint64_t{333'666});
                // no data are returned
            }

            SECTION("small nuke")
            {
                rpc = "/example-schema:launch-nukes";
                input = {
                    {joinPaths(rpc, "description"), "dummy"s},
                    {joinPaths(rpc, "payload/kilotons"), uint64_t{4}},
                };
                proxyDatastore.initiate(rpc);
                proxyDatastore.setLeaf("/example-schema:launch-nukes/example-schema:payload/example-schema:kilotons", uint64_t{4});

                output = {
                    {"blast-radius", uint32_t{33'666}},
                    {"actual-yield", uint64_t{5}},
                };
            }

            SECTION("with lists")
            {
                rpc = "/example-schema:launch-nukes";
                input = {
                    {joinPaths(rpc, "payload/kilotons"), uint64_t{6}},
                    {joinPaths(rpc, "cities/targets[city='Prague']/city"), "Prague"s},
                };
                proxyDatastore.initiate(rpc);
                proxyDatastore.setLeaf("/example-schema:launch-nukes/example-schema:payload/example-schema:kilotons", uint64_t{6});
                proxyDatastore.createItem("/example-schema:launch-nukes/example-schema:cities/example-schema:targets[city='Prague']");
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

            SECTION("with leafref")
            {
                datastore->createItem("/example-schema:person[name='Colton']");
                datastore->commitChanges();

                rpc = "/example-schema:fire";
                input = {
                    {joinPaths(rpc, "whom"), "Colton"s}
                };
                proxyDatastore.initiate(rpc);
                proxyDatastore.setLeaf("/example-schema:fire/example-schema:whom", "Colton"s);
            }

            catching<OnExec>([&] { REQUIRE(datastore->execute(rpc, input) == output); });
            catching<OnExec>([&] { REQUIRE(proxyDatastore.execute() == output); });
        }

        SECTION("non-existing RPC")
        {
            catching<OnInvalidRpcPath>([&] { datastore->execute("/example-schema:non-existing", DatastoreAccess::Tree{}); });
        }

        SECTION("invalid RPC exec resets temporary datastore")
        {
            proxyDatastore.initiate("/example-schema:setIp");
            catching<OnInvalidRpcInput>([&] { auto output = proxyDatastore.execute(); });
            REQUIRE(proxyDatastore.inputDatastorePath() == std::nullopt);
        }
    }

    SECTION("action")
    {
        auto createTemporaryDatastore = [](const std::shared_ptr<DatastoreAccess>& datastore) {
            return std::make_shared<YangAccess>(std::static_pointer_cast<YangSchema>(datastore->schema()));
        };
        ProxyDatastore proxyDatastore(datastore, createTemporaryDatastore);
        std::string path;
        DatastoreAccess::Tree input, output;

        output = {
            {"success", true}
        };
        datastore->createItem("/example-schema:ports[name='A']");
        datastore->commitChanges();
        SECTION("shutdown")
        {
            path = "/example-schema:ports[name='A']/example-schema:shutdown";
            input = {
                {"/example-schema:ports[name='A']/shutdown/force", true}
            };
            proxyDatastore.initiate(path);
            proxyDatastore.setLeaf("/example-schema:ports[name='A']/example-schema:shutdown/example-schema:force", true);

        }

        catching<OnExec>([&] { REQUIRE(datastore->execute(path, input) == output); });
        catching<OnExec>([&] { REQUIRE(proxyDatastore.execute() == output); });
    }

    SECTION("parsed info in yang context")
    {
        auto schema = datastore->schema();
        auto leafTypeName = schema->leafTypeName("/example-schema:typedefedLeaf");

#if defined(sysrepo_BACKEND)
        // Sysrepo is not available yet, with libyang parsed info context
        REQUIRE(leafTypeName == std::nullopt);
#else
        REQUIRE(leafTypeName == "myType");
#endif
    }

    waitForCompletionAndBitMore(seq1);
}

#if not defined(yang_BACKEND)
TEST_CASE("datastore targets")
{
    const auto testNode = "/example-schema:leafInt32";
    {
        auto sess = sysrepo::Connection{}.sessionStart();
        sess.deleteItem(testNode);
        sess.applyChanges(std::chrono::milliseconds{1000});
        sess.switchDatastore(sysrepo::Datastore::Startup);
        sess.deleteItem(testNode);
        sess.applyChanges(std::chrono::milliseconds{1000});
    }
    MockRecorder mockRunning;
    MockRecorder mockStartup;

#ifdef sysrepo_BACKEND
    SysrepoAccess datastore;
#elif defined(netconf_BACKEND)
    const auto NETOPEER_SOCKET = getenv("NETOPEER_SOCKET");
    NetconfAccess datastore(NETOPEER_SOCKET);
#else
#error "Unknown backend"
#endif

    auto testGetItems = [&datastore] (const auto& path, const DatastoreAccess::Tree& expected) {
        REQUIRE(datastore.getItems(path) == expected);
    };

    SECTION("subscriptions change operational target")
    {
        // Default target is operational, so setting a value and reading it should return no values as there are no
        // subscriptions yet.
        datastore.setLeaf(testNode, 10);
        datastore.commitChanges();
        testGetItems(testNode, {});

        // Now we create a subscription and try again.
        SysrepoSubscription subRunning("example-schema", &mockRunning);
        testGetItems(testNode, {{testNode, 10}});
    }

    SECTION("running shows stuff even without subscriptions")
    {
        datastore.setTarget(DatastoreTarget::Running);
        datastore.setLeaf(testNode, 10);
        datastore.commitChanges();
        testGetItems(testNode, {{testNode, 10}});

        // Actually creating a subscription shouldn't make a difference.
        SysrepoSubscription subRunning("example-schema", &mockRunning);
        testGetItems(testNode, {{testNode, 10}});
    }

    SECTION("startup changes only affect startup")
    {
        datastore.setTarget(DatastoreTarget::Startup);
        datastore.setLeaf(testNode, 10);
        datastore.commitChanges();
        testGetItems(testNode, {{testNode, 10}});
        datastore.setTarget(DatastoreTarget::Running);
        testGetItems(testNode, {});
        datastore.setTarget(DatastoreTarget::Operational);
        testGetItems(testNode, {});
    }

}
#endif
