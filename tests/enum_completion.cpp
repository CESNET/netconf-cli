
/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
 */

#include <experimental/iterator>
#include "trompeloeil_doctest.h"
#include "datastoreaccess_mock.hpp"
#include "parser.hpp"
#include "static_schema.hpp"

namespace std {
std::ostream& operator<<(std::ostream& s, const Completions& completion)
{
    s << std::endl << "Completions {" << std::endl << "    m_completions: ";
    std::transform(completion.m_completions.begin(), completion.m_completions.end(),
            std::experimental::make_ostream_joiner(s, ", "),
            [] (auto it) { return '"' + it + '"'; });
    s << std::endl << "    m_contextLength: " << completion.m_contextLength << std::endl;
    s << "}" << std::endl;
    return s;
}
}

TEST_CASE("enum completion")
{
    auto schema = std::make_shared<StaticSchema>();
    schema->addModule("mod");
    schema->addContainer("/", "mod:contA");
    schema->addLeafEnum("/", "mod:leafEnum", {"lala", "lol", "data", "coze"});
    schema->addLeafEnum("/mod:contA", "mod:leafInCont", {"abc", "def"});
    schema->addList("/", "mod:list", {"number"});
    schema->addLeaf("/mod:list", "mod:number", yang::LeafDataTypes::Int32);
    schema->addLeafEnum("/mod:list", "mod:leafInList", {"ano", "anoda", "ne", "katoda"});
    auto mockDatastore = std::make_shared<MockDatastoreAccess>();

    // The parser will use DataQuery for key value completion, but I'm not testing that here, so I don't return anything.
    ALLOW_CALL(*mockDatastore, listInstances("/mod:list"))
        .RETURN(std::vector<ListInstance>{});

    // DataQuery gets the schema from DatastoreAccess once
    auto expectation = NAMED_REQUIRE_CALL(*mockDatastore, schema())
        .RETURN(schema);
    auto dataQuery = std::make_shared<DataQuery>(*mockDatastore);
    expectation.reset();
    Parser parser(schema, dataQuery);
    std::string input;
    std::ostringstream errorStream;

    std::set<std::string> expectedCompletions;
    int expectedContextLength;

    SECTION("set mod:leafEnum ")
    {
        input = "set mod:leafEnum ";
        expectedCompletions = {"lala", "lol", "data", "coze"};
        expectedContextLength = 0;
    }

    SECTION("set mod:leafEnum c")
    {
        input = "set mod:leafEnum c";
        expectedCompletions = {"coze"};
        expectedContextLength = 1;
    }

    SECTION("set mod:leafEnum l")
    {
        input = "set mod:leafEnum l";
        expectedCompletions = {"lala", "lol"};
        expectedContextLength = 1;
    }


    SECTION("set mod:contA/leafInCont ")
    {
        input = "set mod:contA/leafInCont ";
        expectedCompletions = {"abc", "def"};
        expectedContextLength = 0;
    }

    SECTION("set mod:list[number=42]/leafInList ")
    {
        input = "set mod:list[number=42]/leafInList ";
        expectedCompletions = {"ano", "anoda", "ne", "katoda"};
        expectedContextLength = 0;
    }

    REQUIRE(parser.completeCommand(input, errorStream) == (Completions{expectedCompletions, expectedContextLength}));
}
