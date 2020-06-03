/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/
#include <docopt.h>
#include <iostream>
#include <optional>
#include <replxx.hxx>
#include <sstream>
#include "NETCONF_CLI_VERSION.h"
#include "interpreter.hpp"
#if defined(SYSREPO_CLI)
#include "sysrepo_access.hpp"
#define PROGRAM_NAME "sysrepo-cli"
static const auto usage = R"(CLI interface to sysrepo

Usage:
  sysrepo-cli [-d <datastore>]
  sysrepo-cli (-h | --help)
  sysrepo-cli --version

Options:
  -d <datastore>   can be "running" or "startup" [default: running])";
#elif defined(YANG_CLI)
#include "yang_access.hpp"
#define PROGRAM_NAME "yang-cli"
static const auto usage = R"(CLI interface for creating local YANG data instances

Usage:
  yang-cli
  yang-cli (-h | --help)
  yang-cli --version)";
#else
#error "Unknown CLI backend"
#endif

const auto HISTORY_FILE_NAME = PROGRAM_NAME "_history";

int main(int argc, char* argv[])
{
    auto args = docopt::docopt(usage,
                               {argv + 1, argv + argc},
                               true,
                               PROGRAM_NAME " " NETCONF_CLI_VERSION,
                               true);

#if defined(SYSREPO_CLI)
    auto datastoreType = Datastore::Running;
    if (const auto& ds = args["-d"]) {
        if (ds.asString() == "startup") {
            datastoreType = Datastore::Startup;
        } else if (ds.asString() == "running") {
            datastoreType = Datastore::Running;
        } else {
            std::cerr << PROGRAM_NAME <<  ": unknown datastore: " << ds.asString() << "\n";
            return 1;
        }
    }
    SysrepoAccess datastore(PROGRAM_NAME, datastoreType);
    std::cout << "Connected to sysrepo [datastore: " << (datastoreType == Datastore::Startup ? "startup" : "running") << "]" << std::endl;
#elif defined(YANG_CLI)
    YangAccess datastore;
#else
#error "Unknown CLI backend"
#endif

    auto dataQuery = std::make_shared<DataQuery>(datastore);
    Parser parser(datastore.schema(), dataQuery);

    using replxx::Replxx;

    Replxx lineEditor;

    lineEditor.bind_key(Replxx::KEY::meta(Replxx::KEY::BACKSPACE), std::bind(&Replxx::invoke, &lineEditor, Replxx::ACTION::KILL_TO_BEGINING_OF_WORD, std::placeholders::_1));
    lineEditor.bind_key(Replxx::KEY::control('W'), std::bind(&Replxx::invoke, &lineEditor, Replxx::ACTION::KILL_TO_WHITESPACE_ON_LEFT, std::placeholders::_1));

    lineEditor.set_word_break_characters("\t _[]/:'\"=-%");

    lineEditor.set_completion_callback([&parser](const std::string& input, int& context) {
        std::stringstream stream;
        auto completions = parser.completeCommand(input, stream);

        std::vector<replxx::Replxx::Completion> res;
        std::copy(completions.m_completions.begin(), completions.m_completions.end(), std::back_inserter(res));
        context = completions.m_contextLength;
        return res;
    });

    std::optional<std::string> historyFile;
    if (auto xdgHome = getenv("XDG_DATA_HOME")) {
        historyFile = std::string(xdgHome) + "/" + HISTORY_FILE_NAME;
    } else if (auto home = getenv("HOME")) {
        historyFile = std::string(home) + "/.local/share/" + HISTORY_FILE_NAME;
    }

    if (historyFile)
        lineEditor.history_load(historyFile.value());

    while (true) {
        auto line = lineEditor.input(parser.currentNode() + "> ");
        if (!line) {
            // If user pressed CTRL-C to abort the line, errno gets set to EAGAIN.
            // If user pressed CTRL-D (for EOF), errno doesn't get set to EAGAIN, so we exit the program.
            // I have no idea why replxx uses errno for this.
            if (errno == EAGAIN) {
                continue;
            } else {
                break;
            }
        }

        std::locale C_locale("C");
        std::string_view view{line};
        if (std::all_of(view.begin(), view.end(),
                        [C_locale](const auto c) { return std::isspace(c, C_locale);})) {
            continue;
        }

        try {
            command_ cmd = parser.parseCommand(line, std::cout);
            boost::apply_visitor(Interpreter(parser, datastore), cmd);
        } catch (InvalidCommandException& ex) {
            std::cerr << ex.what() << std::endl;
        } catch (DatastoreException& ex) {
            std::cerr << ex.what() << std::endl;
        }

        lineEditor.history_add(line);
    }

    if (historyFile)
        lineEditor.history_save(historyFile.value());

    return 0;
}
