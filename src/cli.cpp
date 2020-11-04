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
#include "proxy_datastore.hpp"
#include "yang_schema.hpp"
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
#include <boost/spirit/home/x3.hpp>
#include <filesystem>
#include "yang_access.hpp"
#define PROGRAM_NAME "yang-cli"
static const auto usage = R"(CLI interface for creating local YANG data instances

  The <schema_file_or_module_name> argument is treated as a file name if a file
  with such a path exists, otherwise it's treated as a module name. Search dirs
  will be used to find a schema for that module.

Usage:
  yang-cli [--configonly] [-s <search_dir>] [-e enable_features]... [-i data_file]... <schema_file_or_module_name>...
  yang-cli (-h | --help)
  yang-cli --version

Options:
  -s <search_dir>       Set search for schema lookup
  -e <enable_features>  Feature to enable after modules are loaded. This option can be supplied more than once. Format: <module_name>:<feature>
  -i <data_file>        File to import data from
  --configonly          Disable editing of operational data)";
#elif defined(NETCONF_CLI)
// FIXME: improve usage
static const auto usage = R"(CLI interface for NETCONF

Usage:
  netconf-cli [-v] [-p <port>] <host>
  netconf-cli (-h | --help)
  netconf-cli --version

Options:
  -v         enable verbose mode
  -p <port>  port number [default: 830]
)";
#include "netconf_access.hpp"
#include "cli-netconf.hpp"
#define PROGRAM_NAME "netconf-access"
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
    WritableOps writableOps = WritableOps::No;

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
    auto datastore = std::make_shared<SysrepoAccess>(datastoreType);
    std::cout << "Connected to sysrepo [datastore: " << (datastoreType == Datastore::Startup ? "startup" : "running") << "]" << std::endl;
#elif defined(YANG_CLI)
    auto datastore = std::make_shared<YangAccess>();
    if (args["--configonly"].asBool()) {
        writableOps = WritableOps::No;
    } else {
        writableOps = WritableOps::Yes;
        std::cout << "ops is writable" << std::endl;
    }
    if (const auto& search_dir = args["-s"]) {
        datastore->addSchemaDir(search_dir.asString());
    }
    for (const auto& schemaFile : args["<schema_file_or_module_name>"].asStringList()) {
        if (std::filesystem::exists(schemaFile)) {
            datastore->addSchemaFile(schemaFile);
        } else if (schemaFile.find('/') == std::string::npos) { // Module names cannot have a slash
            datastore->loadModule(schemaFile);
        } else {
            std::cerr << "Cannot load YANG module " << schemaFile << "\n";
        }
    }
    if (const auto& enableFeatures = args["-e"]) {
        namespace x3 = boost::spirit::x3;
        auto grammar = +(x3::char_-":") >> ":" >> +(x3::char_-":");
        for (const auto& enableFeature : enableFeatures.asStringList()) {
            std::pair<std::string, std::string> parsed;
            auto it = enableFeature.begin();
            auto res = x3::parse(it, enableFeature.cend(), grammar, parsed);
            if (!res || it != enableFeature.cend()) {
                std::cerr << "Error parsing feature enable flags: " << enableFeature << "\n";
                return 1;
            }
            try {
                datastore->enableFeature(parsed.first, parsed.second);
            } catch (std::runtime_error& ex) {
                std::cerr << ex.what() << "\n";
                return 1;
            }

        }
    }
    if (const auto& dataFiles = args["-i"]) {
        for (const auto& dataFile : dataFiles.asStringList()) {
            datastore->addDataFile(dataFile);
        }
    }
#elif defined(NETCONF_CLI)
    auto verbose = args.at("-v").asBool();
    if (verbose) {
        NetconfAccess::setNcLogLevel(NC_VERB_DEBUG);
    }

    SshProcess process;
    std::shared_ptr<NetconfAccess> datastore;

    try {
        process = sshProcess(args.at("<host>").asString(), args.at("-p").asString());
        datastore = std::make_shared<NetconfAccess>(process.std_out.native_source(), process.std_in.native_sink());
    } catch (std::runtime_error& ex) {
        std::cerr << "SSH connection failed: " << ex.what() << "\n";
        return 1;
    }
#else
#error "Unknown CLI backend"
#endif

#if defined(SYSREPO_CLI) || defined(NETCONF_CLI)
    auto createTemporaryDatastore = [](const std::shared_ptr<DatastoreAccess>& datastore) {
        return std::make_shared<YangAccess>(std::static_pointer_cast<YangSchema>(datastore->schema()));
    };
#elif defined(YANG_CLI)
    auto createTemporaryDatastore = [](const std::shared_ptr<DatastoreAccess>&) {
        return nullptr;
    };
#endif

    ProxyDatastore proxyDatastore(datastore, createTemporaryDatastore);
    auto dataQuery = std::make_shared<DataQuery>(*datastore);
    Parser parser(datastore->schema(), writableOps, dataQuery);

    using replxx::Replxx;

    Replxx lineEditor;

    lineEditor.bind_key(Replxx::KEY::meta(Replxx::KEY::BACKSPACE), [&lineEditor] (const auto& code) {
        return lineEditor.invoke(Replxx::ACTION::KILL_TO_BEGINING_OF_WORD, code);
    });
    lineEditor.bind_key(Replxx::KEY::control('W'), [&lineEditor] (const auto& code) {
        return lineEditor.invoke(Replxx::ACTION::KILL_TO_WHITESPACE_ON_LEFT, code);
    });

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

    if (historyFile) {
        lineEditor.history_load(historyFile.value());
    }

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
            boost::apply_visitor(Interpreter(parser, proxyDatastore), cmd);
        } catch (InvalidCommandException& ex) {
            std::cerr << ex.what() << std::endl;
        } catch (DatastoreException& ex) {
            std::cerr << ex.what() << std::endl;
        } catch (std::runtime_error& ex) {
            std::cerr << ex.what() << std::endl;
        }

        lineEditor.history_add(line);
    }

    if (historyFile) {
        lineEditor.history_save(historyFile.value());
    }

    return 0;
}
