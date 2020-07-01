/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/

#include "trompeloeil_doctest.hpp"
#include <sysrepo-cpp/Session.hpp>

#ifdef sysrepo_BACKEND
#include "sysrepo_access.hpp"
using OnInvalidSchemaPathCreate = DatastoreException;
using OnInvalidSchemaPathDelete = void;
using OnInvalidSchemaPathMove = sysrepo::sysrepo_exception;
using OnInvalidRpcPath = sysrepo::sysrepo_exception;
using OnKeyNotFound = void;
using OnRPC = void;
#elif defined(netconf_BACKEND)
using OnInvalidSchemaPathCreate = std::runtime_error;
using OnInvalidSchemaPathDelete = std::runtime_error;
using OnInvalidSchemaPathMove = std::runtime_error;
using OnInvalidRpcPath = std::runtime_error;
using OnKeyNotFound = std::runtime_error;
using OnRPC = void;
#include "netconf_access.hpp"
#include "netopeer_vars.hpp"
#elif defined(yang_BACKEND)
#include <fstream>
#include "yang_access.hpp"
#include "yang_access_test_vars.hpp"
using OnInvalidSchemaPathCreate = DatastoreException;
using OnInvalidSchemaPathDelete = DatastoreException;
using OnInvalidSchemaPathMove = DatastoreException;
using OnInvalidRpcPath = DatastoreException;
using OnKeyNotFound = DatastoreException;
using OnRPC = std::logic_error;
#else
#error "Unknown backend"
#endif
#include "pretty_printers.hpp"
#include "sysrepo_subscription.hpp"
#include "utils.hpp"

using namespace std::literals::string_literals;

class MockRecorder : public trompeloeil::mock_interface<Recorder> {
public:
    IMPLEMENT_MOCK3(write);
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
    } else if constexpr (std::is_same<Exception, sysrepo::sysrepo_exception>()) {
        REQUIRE_THROWS_AS(what(), sysrepo::sysrepo_exception);
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
        auto command = std::string(sysrepocfgExecutable) + " --import=" + testConfigFile + " --format=xml --datastore=running example-schema";
        REQUIRE(std::system(command.c_str()) == 0);
    }
};
#endif

TEST_CASE("setting/getting values")
{
    trompeloeil::sequence seq1;
    MockRecorder mock;
    SysrepoSubscription subscription("example-schema", &mock);

#ifdef sysrepo_BACKEND
    SysrepoAccess datastore("netconf-cli-test", Datastore::Running);
#elif defined(netconf_BACKEND)
    NetconfAccess datastore(NETOPEER_SOCKET_PATH);
#elif defined(yang_BACKEND)
    TestYangAccess datastore;
    datastore.addSchemaDir(schemaDir);
    datastore.addSchemaFile(exampleSchemaFile);
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

    SECTION("set a string, then delete it")
    {
        REQUIRE_CALL(mock, write("/example-schema:leafString", std::nullopt, "blah"s));
        datastore.setLeaf("/example-schema:leafString", "blah"s);
        datastore.commitChanges();
        DatastoreAccess::Tree expected{{"/example-schema:leafString", "blah"s}};
        REQUIRE(datastore.getItems("/example-schema:leafString") == expected);

        REQUIRE_CALL(mock, write("/example-schema:leafString", "blah"s, std::nullopt));
        datastore.deleteItem("/example-schema:leafString");
        datastore.commitChanges();
        expected.clear();
        REQUIRE(datastore.getItems("/example-schema:leafString") == expected);
    }

    SECTION("set a non-existing leaf")
    {
        catching<OnInvalidSchemaPathCreate>([&]{
            datastore.setLeaf("/example-schema:non-existing", "what"s);
        });
    }

    SECTION("create presence container")
    {
        REQUIRE(datastore.dump(DataFormat::Json).find("example-schema:pContainer") == std::string::npos);
        REQUIRE_CALL(mock, write("/example-schema:pContainer", std::nullopt, ""s));
        datastore.createItem("/example-schema:pContainer");
        datastore.commitChanges();
        REQUIRE(datastore.dump(DataFormat::Json).find("example-schema:pContainer") != std::string::npos);
    }

    SECTION("create/delete a list instance")
    {
        {
            REQUIRE_CALL(mock, write("/example-schema:person[name='Nguyen']", std::nullopt, ""s));
            REQUIRE_CALL(mock, write("/example-schema:person[name='Nguyen']/name", std::nullopt, "Nguyen"s));
            datastore.createItem("/example-schema:person[name='Nguyen']");
            datastore.commitChanges();
        }
        {
            REQUIRE_CALL(mock, write("/example-schema:person[name='Nguyen']", ""s, std::nullopt));
            REQUIRE_CALL(mock, write("/example-schema:person[name='Nguyen']/name", "Nguyen"s, std::nullopt));
            datastore.deleteItem("/example-schema:person[name='Nguyen']");
            datastore.commitChanges();
        }
    }

    SECTION("deleting non-existing list keys")
    {
        catching<OnKeyNotFound>([&]{
            datastore.deleteItem("/example-schema:person[name='non existing']");
            datastore.commitChanges();
        });
    }

    SECTION("accessing non-existing schema nodes as a list")
    {
        catching<OnInvalidSchemaPathCreate>([&]{
            datastore.createItem("/example-schema:non-existing-list[xxx='blah']");
            datastore.commitChanges();
        });
        catching<OnInvalidSchemaPathDelete>([&]{
            datastore.deleteItem("/example-schema:non-existing-list[xxx='non existing']");
            datastore.commitChanges();
        });
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
            REQUIRE_CALL(mock, write("/example-schema:bossPerson", std::nullopt, value));
            datastore.commitChanges();
        }
        REQUIRE(datastore.getItems("/example-schema:bossPerson") == DatastoreAccess::Tree{{"/example-schema:bossPerson", value}});
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

        DatastoreAccess::Tree expected{
        // Sysrepo always returns containers when getting values, but
        // libnetconf does not. This is fine by the YANG standard:
        // https://tools.ietf.org/html/rfc7950#section-7.5.7 Furthermore,
        // NetconfAccess implementation actually only iterates over leafs,
        // so even if libnetconf did include containers, they wouldn't get
        // shown here anyway. With sysrepo2, this won't be necessary,
        // because it'll use the same data structure as libnetconf, so the
        // results will be consistent.
#ifdef sysrepo_BACKEND
                                                   {"/example-schema:inventory", special_{SpecialValue::Container}},
                                                   {"/example-schema:lol", special_{SpecialValue::Container}},
#endif
                                                   {"/example-schema:up", bool{true}},
                                                   {"/example-schema:down", bool{false}}};
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
            REQUIRE_CALL(mock, write("/example-schema:pContainer", std::nullopt, ""s));
            datastore.createItem("/example-schema:pContainer");
            datastore.commitChanges();
        }
        expected = {
            {"/example-schema:pContainer", special_{SpecialValue::PresenceContainer}}
        };
        REQUIRE(datastore.getItems("/example-schema:pContainer") == expected);

        // Make sure it's not there after we delete it
        {
            REQUIRE_CALL(mock, write("/example-schema:pContainer", ""s, std::nullopt));
            datastore.deleteItem("/example-schema:pContainer");
            datastore.commitChanges();
        }
        expected = {};
        REQUIRE(datastore.getItems("/example-schema:pContainer") == expected);
    }

    SECTION("creating a non-existing schema node as a container")
    {
        catching<OnInvalidSchemaPathCreate>([&]{
            datastore.createItem("/example-schema:non-existing-presence-container");
            datastore.commitChanges();
        });
    }

    SECTION("deleting a non-existing schema node as a container or leaf")
    {
        catching<OnInvalidSchemaPathDelete>([&]{
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
            REQUIRE_CALL(mock, write("/example-schema:inventory", std::nullopt, ""s));
            REQUIRE_CALL(mock, write("/example-schema:inventory/stuff", std::nullopt, ""s));
            datastore.createItem("/example-schema:inventory/stuff");
            datastore.commitChanges();
        }
        expected = {
            {"/example-schema:inventory/stuff", special_{SpecialValue::PresenceContainer}}
        };
        REQUIRE(datastore.getItems("/example-schema:inventory/stuff") == expected);
        {
            REQUIRE_CALL(mock, write("/example-schema:inventory", ""s, std::nullopt));
            REQUIRE_CALL(mock, write("/example-schema:inventory/stuff", ""s, std::nullopt));
            datastore.deleteItem("/example-schema:inventory/stuff");
            datastore.commitChanges();
        }
        expected = {};
        REQUIRE(datastore.getItems("/example-schema:inventory/stuff") == expected);
    }

    SECTION("floats")
    {
        datastore.setLeaf("/example-schema:leafDecimal", 123.4);
        REQUIRE_CALL(mock, write("/example-schema:leafDecimal", std::nullopt, "123.4"s));
        datastore.commitChanges();
        DatastoreAccess::Tree expected {
            {"/example-schema:leafDecimal", 123.4},
        };
        REQUIRE(datastore.getItems("/example-schema:leafDecimal") == expected);
    }

    SECTION("unions")
    {
        datastore.setLeaf("/example-schema:unionIntString", int32_t{10});
        REQUIRE_CALL(mock, write("/example-schema:unionIntString", std::nullopt, "10"s));
        datastore.commitChanges();
        DatastoreAccess::Tree expected {
            {"/example-schema:unionIntString", int32_t{10}},
        };
        REQUIRE(datastore.getItems("/example-schema:unionIntString") == expected);
    }

    SECTION("identityref") {
        datastore.setLeaf("/example-schema:beast", identityRef_{"example-schema", "Mammal"});
        REQUIRE_CALL(mock, write("/example-schema:beast", std::nullopt, "example-schema:Mammal"s));
        datastore.commitChanges();
        DatastoreAccess::Tree expected {
            {"/example-schema:beast", identityRef_{"example-schema", "Mammal"}},
        };
        REQUIRE(datastore.getItems("/example-schema:beast") == expected);

        datastore.setLeaf("/example-schema:beast", identityRef_{"Whale"});
        REQUIRE_CALL(mock, write("/example-schema:beast", "example-schema:Mammal", "example-schema:Whale"s));
        datastore.commitChanges();
        expected = {
            {"/example-schema:beast", identityRef_{"example-schema", "Whale"}},
        };
        REQUIRE(datastore.getItems("/example-schema:beast") == expected);
    }

    SECTION("binary")
    {
        datastore.setLeaf("/example-schema:blob", binary_{"cHduegByIQ=="s});
        REQUIRE_CALL(mock, write("/example-schema:blob", std::nullopt, "cHduegByIQ=="s));
        datastore.commitChanges();
        DatastoreAccess::Tree expected {
            {"/example-schema:blob", binary_{"cHduegByIQ=="s}},
        };
        REQUIRE(datastore.getItems("/example-schema:blob") == expected);
    }

    SECTION("empty")
    {
        datastore.setLeaf("/example-schema:dummy", empty_{});
        REQUIRE_CALL(mock, write("/example-schema:dummy", std::nullopt, ""s));
        datastore.commitChanges();
        DatastoreAccess::Tree expected {
            {"/example-schema:dummy", empty_{}},
        };
        REQUIRE(datastore.getItems("/example-schema:dummy") == expected);
    }

#if not defined(yang_BACKEND)
    SECTION("operational data")
    {
        MockDataSupplier mockOpsData;
        OperationalDataSubscription opsDataSub("/example-schema:temperature", mockOpsData);
        DatastoreAccess::Tree expected;
        std::string xpath;
        SECTION("temperature")
        {
            expected = {{"/example-schema:temperature", int32_t{22}}};
            xpath = "/example-schema:temperature";
        }

        REQUIRE_CALL(mockOpsData, get_data(xpath)).RETURN(expected);
        REQUIRE(datastore.getItems(xpath) == expected);
    }
#endif

    SECTION("leaf list")
    {
        DatastoreAccess::Tree expected;
        REQUIRE_CALL(mock, write("/example-schema:addresses", std::nullopt, "0.0.0.0"s));
        REQUIRE_CALL(mock, write("/example-schema:addresses", std::nullopt, "127.0.0.1"s));
        datastore.createItem("/example-schema:addresses[.='0.0.0.0']");
        datastore.createItem("/example-schema:addresses[.='127.0.0.1']");
        datastore.commitChanges();
        expected = {
            {"/example-schema:addresses", special_{SpecialValue::LeafList}},
            {"/example-schema:addresses[.='0.0.0.0']", "0.0.0.0"s},
            {"/example-schema:addresses[.='127.0.0.1']", "127.0.0.1"s},
        };
        REQUIRE(datastore.getItems("/example-schema:addresses") == expected);

        REQUIRE_CALL(mock, write("/example-schema:addresses", "0.0.0.0"s, std::nullopt));
        datastore.deleteItem("/example-schema:addresses[.='0.0.0.0']");
        datastore.commitChanges();
        expected = {
            {"/example-schema:addresses", special_{SpecialValue::LeafList}},
            {"/example-schema:addresses[.='127.0.0.1']", "127.0.0.1"s},
        };
        REQUIRE(datastore.getItems("/example-schema:addresses") == expected);

        REQUIRE_CALL(mock, write("/example-schema:addresses", "127.0.0.1"s, std::nullopt));
        datastore.deleteItem("/example-schema:addresses[.='127.0.0.1']");
        datastore.commitChanges();
        expected = {};
        REQUIRE(datastore.getItems("/example-schema:addresses") == expected);
    }

    SECTION("deleting a non-existing leaf-list")
    {
        catching<OnKeyNotFound>([&]{
            datastore.deleteItem("/example-schema:addresses[.='non-existing']");
            datastore.commitChanges();
        });
    }

    SECTION("accessing a non-existing schema node as a leaf-list")
    {
        catching<OnInvalidSchemaPathCreate>([&]{
            datastore.createItem("/example-schema:non-existing[.='non-existing']");
            datastore.commitChanges();
        });

        catching<OnInvalidSchemaPathDelete>([&]{
            datastore.deleteItem("/example-schema:non-existing[.='non-existing']");
            datastore.commitChanges();
        });
    }

    SECTION("copying data from startup refreshes the data")
    {
        {
            REQUIRE(datastore.getItems("/example-schema:leafInt16") == DatastoreAccess::Tree{});
            REQUIRE_CALL(mock, write("/example-schema:leafInt16", std::nullopt, "123"s));
            datastore.setLeaf("/example-schema:leafInt16", int16_t{123});
            datastore.commitChanges();
        }
        REQUIRE(datastore.getItems("/example-schema:leafInt16") == DatastoreAccess::Tree{{"/example-schema:leafInt16", int16_t{123}}});
        REQUIRE_CALL(mock, write("/example-schema:leafInt16", "123"s, std::nullopt));
        datastore.copyConfig(Datastore::Startup, Datastore::Running);
        REQUIRE(datastore.getItems("/example-schema:leafInt16") == DatastoreAccess::Tree{});
    }

    SECTION("moving leaflist instances")
    {
        DatastoreAccess::Tree expected;
        {
            // sysrepo does this twice for some reason, it's possibly a bug
            REQUIRE_CALL(mock, write("/example-schema:protocols", std::nullopt, "http"s)).TIMES(2);
            REQUIRE_CALL(mock, write("/example-schema:protocols", std::nullopt, "ftp"s));
            REQUIRE_CALL(mock, write("/example-schema:protocols", std::nullopt, "pop3"s));
            REQUIRE_CALL(mock, write("/example-schema:protocols", "http"s, "ftp"s));
            REQUIRE_CALL(mock, write("/example-schema:protocols", "ftp"s, "pop3"s));
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
            REQUIRE_CALL(mock, write("/example-schema:protocols", std::nullopt, "pop3"s));
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
            REQUIRE_CALL(mock, write("/example-schema:protocols", "pop3"s, "http"s));
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
            REQUIRE_CALL(mock, write("/example-schema:protocols", "ftp"s, "http"s));
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
            REQUIRE_CALL(mock, write("/example-schema:protocols", "ftp"s, "http"s));
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
        catching<OnInvalidSchemaPathMove>([&]{
            datastore.moveItem("/example-schema:non-existing", yang::move::Absolute::Begin);
            datastore.commitChanges();
        });
    }

    SECTION("moving list instances")
    {
        DatastoreAccess::Tree expected;
        {
            // sysrepo does this twice for some reason, it's possibly a bug
            REQUIRE_CALL(mock, write("/example-schema:players[name='John']", std::nullopt, ""s)).TIMES(2);
            REQUIRE_CALL(mock, write("/example-schema:players[name='John']/name", std::nullopt, "John"s));
            REQUIRE_CALL(mock, write("/example-schema:players[name='Eve']", std::nullopt, ""s));
            REQUIRE_CALL(mock, write("/example-schema:players[name='Eve']", ""s, ""s));
            REQUIRE_CALL(mock, write("/example-schema:players[name='Eve']/name", std::nullopt, "Eve"s));
            REQUIRE_CALL(mock, write("/example-schema:players[name='Adam']", std::nullopt, ""s));
            REQUIRE_CALL(mock, write("/example-schema:players[name='Adam']/name", std::nullopt, "Adam"s));
            REQUIRE_CALL(mock, write("/example-schema:players[name='Adam']", ""s, ""s));
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
            REQUIRE_CALL(mock, write("/example-schema:players[name='Adam']", std::nullopt, ""s));
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
            REQUIRE_CALL(mock, write("/example-schema:players[name='John']", ""s, ""s));
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
            REQUIRE_CALL(mock, write("/example-schema:players[name='John']", ""s, ""s));
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
            REQUIRE_CALL(mock, write("/example-schema:players[name='John']", ""s, ""s));
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
            REQUIRE_CALL(mock, write("/example-schema:leafInt32", std::nullopt, "64"s));
            datastore.setLeaf("/example-schema:leafInt32", 64);
            datastore.commitChanges();
        }

        DatastoreAccess::Tree expected{
        // Sysrepo always returns containers when getting values, but
        // libnetconf does not. This is fine by the YANG standard:
        // https://tools.ietf.org/html/rfc7950#section-7.5.7 Furthermore,
        // NetconfAccess implementation actually only iterates over leafs,
        // so even if libnetconf did include containers, they wouldn't get
        // shown here anyway. With sysrepo2, this won't be necessary,
        // because it'll use the same data structure as libnetconf, so the
        // results will be consistent.
#ifdef sysrepo_BACKEND
                                                   {"/example-schema:inventory", special_{SpecialValue::Container}},
                                                   {"/example-schema:lol", special_{SpecialValue::Container}},
#endif
                                                   {"/example-schema:leafInt32", 64}};
        auto items = datastore.getItems("/");
        // This tests if we at least get the data WE added.
        REQUIRE(std::all_of(expected.begin(), expected.end(), [items] (const auto& item) { return std::find(items.begin(), items.end(), item) != items.end(); }));
    }

    SECTION("setting and removing without commit")
    {
        datastore.setLeaf("/example-schema:leafInt32", 64);
        datastore.deleteItem("/example-schema:leafInt32");
    }

    SECTION("two key lists")
    {
        REQUIRE_CALL(mock, write("/example-schema:point[x='12'][y='10']", std::nullopt, ""s));
        REQUIRE_CALL(mock, write("/example-schema:point[x='12'][y='10']/x", std::nullopt, "12"s));
        REQUIRE_CALL(mock, write("/example-schema:point[x='12'][y='10']/y", std::nullopt, "10"s));
        datastore.createItem("/example-schema:point[x='12'][y='10']");
        datastore.commitChanges();
        REQUIRE(datastore.dump(DataFormat::Json).find("example-schema:point") != std::string::npos);
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
    SysrepoAccess datastore("netconf-cli-test", Datastore::Running);
#elif defined(netconf_BACKEND)
    NetconfAccess datastore(NETOPEER_SOCKET_PATH);
#elif defined(yang_BACKEND)
    YangAccess datastore;
    datastore.addSchemaDir(schemaDir);
    datastore.addSchemaFile(exampleSchemaFile);
#else
#error "Unknown backend"
#endif

    SECTION("valid")
    {
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
                {"damaged-places", special_{SpecialValue::PresenceContainer}},
                {"damaged-places/targets[city='London']", special_{SpecialValue::List}},
                {"damaged-places/targets[city='London']/city", "London"s},
                {"damaged-places/targets[city='Berlin']", special_{SpecialValue::List}},
                {"damaged-places/targets[city='Berlin']/city", "Berlin"s},
            };
        }

        catching<OnRPC>([&] {REQUIRE(datastore.executeRpc(rpc, input) == output);});
    }

    SECTION("non-existing RPC")
    {
        catching<OnInvalidRpcPath>([&] {datastore.executeRpc("/example-schema:non-existing", DatastoreAccess::Tree{});});
    }

    waitForCompletionAndBitMore(seq1);
}
