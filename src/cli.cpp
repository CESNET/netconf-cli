/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/
#include <atomic>
#include <boost/algorithm/string.hpp>
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
  sysrepo-cli [-d <datastore_target>]
  sysrepo-cli (-h | --help)
  sysrepo-cli --version

Options:
  -d <datastore_target>   can be "running", "startup" or "operational" [default: operational])";
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
  yang-cli [--configonly] [--ignore-unknown-data] [-s <search_dir>]... [-e enable_features]... [-i data_file]... <schema_file_or_module_name>...
  yang-cli (-h | --help)
  yang-cli --version

Options:
  -s <search_dir>        Search in these directories for YANG schema files. This option can be supplied more than once.
  -e <enable_features>   Feature to enable after modules are loaded. This option can be supplied more than once. Format: <module_name>:<feature>
  -i <data_file>         File to import data from
  --configonly           Disable editing of operational data
  --ignore-unknown-data  Silently ignore data with no available schema file)";
#elif defined(NETCONF_CLI)
// FIXME: improve usage
static const auto usage = R"(CLI interface for NETCONF

Usage:
  netconf-cli [-v] [-d <datastore_target>] [-p <port>] <host>
  netconf-cli [-v] [-d <datastore_target>] --socket <path>
  netconf-cli (-h | --help)
  netconf-cli --version

Options:
  -v         enable verbose mode
  -p <port>  port number [default: 830]
  -d <datastore_target>   can be "running", "startup" or "operational" [default: operational])";
#include "cli-netconf.hpp"
#include "netconf_access.hpp"
#define PROGRAM_NAME "netconf-cli"
// FIXME: this should be replaced by C++20 std::jthread at some point
struct PoorMansJThread {
    ~PoorMansJThread()
    {
        if (thread.joinable()) {
            thread.join();
        }
    }
    std::thread thread;
};
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

    using replxx::Replxx;
    Replxx lineEditor;
    std::atomic<int> backendReturnCode = 0;

    // For some reason, GCC10 still needs [[maybe_unused]] because of conditional compilation...
    [[maybe_unused]] auto datastoreTarget = DatastoreTarget::Operational;
    if (const auto& ds = args["-d"]) {
        if (ds.asString() == "startup") {
            datastoreTarget = DatastoreTarget::Startup;
        } else if (ds.asString() == "running") {
            datastoreTarget = DatastoreTarget::Running;
        } else if (ds.asString() == "operational") {
            datastoreTarget = DatastoreTarget::Operational;
        } else {
            std::cerr << PROGRAM_NAME << ": unknown datastore target: " << ds.asString() << "\n";
            return 1;
        }
    }

    auto datastoreTargetString = args["-d"] ? args["-d"].asString() : std::string("operational");

#if defined(SYSREPO_CLI)
    auto datastore = std::make_shared<SysrepoAccess>();
    std::cout << "Connected to sysrepo [datastore target: " << datastoreTargetString << "]" << std::endl;
#elif defined(YANG_CLI)
    auto datastore = std::make_shared<YangAccess>();
    if (args["--configonly"].asBool()) {
        writableOps = WritableOps::No;
    } else {
        writableOps = WritableOps::Yes;
        std::cout << "ops is writable" << std::endl;
    }
    for (const auto& dir : args["-s"].asStringList()) {
        datastore->addSchemaDir(dir);
    }
    for (const auto& schemaFile : args["<schema_file_or_module_name>"].asStringList()) {
        std::filesystem::path path{schemaFile};
        if (std::filesystem::exists(path)) {
            if (std::filesystem::is_regular_file(path)) {
                datastore->addSchemaDir(path.parent_path());
            }
            datastore->addSchemaFile(schemaFile);
        } else if (schemaFile.find('/') == std::string::npos) { // Module names cannot have a slash
            datastore->loadModule(schemaFile);
        } else {
            std::cerr << "Cannot load YANG module " << schemaFile << "\n";
        }
    }
    if (const auto& enableFeatures = args["-e"]) {
        namespace x3 = boost::spirit::x3;
        std::map<std::string, std::vector<std::string>> toEnable;
        auto grammar = +(x3::char_-":") >> ":" >> +(x3::char_-":");
        for (const auto& enableFeature : enableFeatures.asStringList()) {
            std::pair<std::string, std::string> parsed;
            auto it = enableFeature.begin();
            auto res = x3::parse(it, enableFeature.cend(), grammar, parsed);
            if (!res || it != enableFeature.cend()) {
                std::cerr << "Error parsing feature enable flags: " << enableFeature << "\n";
                return 1;
            }
            toEnable[parsed.first].emplace_back(parsed.second);
        }
        try {
            for (const auto& [moduleName, features] : toEnable) {
                datastore->setEnabledFeatures(moduleName, features);
            }
        } catch (std::runtime_error& ex) {
            std::cerr << ex.what() << "\n";
            return 1;
        }
    }

    auto strict = args.at("--ignore-unknown-data").asBool() ? StrictDataParsing::No : StrictDataParsing::Yes;

    if (const auto& dataFiles = args["-i"]) {
        for (const auto& dataFile : dataFiles.asStringList()) {
            datastore->addDataFile(dataFile, strict);
        }
    }
#elif defined(NETCONF_CLI)
    auto verbose = args.at("-v").asBool();
    if (verbose) {
        NetconfAccess::setNcLogLevel(libnetconf::LogLevel::Debug);
    }

    SshProcess process;
    PoorMansJThread processWatcher;
    std::shared_ptr<NetconfAccess> datastore;

    if (args.at("--socket").asBool()) {
        try {
            datastore = std::make_shared<NetconfAccess>(args.at("<path>").asString());
        } catch (std::runtime_error& ex) {
            std::cerr << "UNIX socket connection failed: " << ex.what() << std::endl;
            return 1;
        }
    } else {
        try {
            process = sshProcess(args.at("<host>").asString(), args.at("-p").asString());
            processWatcher.thread = std::thread{std::thread{[&process, &lineEditor, &backendReturnCode] () {
                process.process.wait();
                backendReturnCode = process.process.exit_code();
                // CTRL-U clears from the cursor to the start of the line
                // CTRL-K clears from the cursor to the end of the line
                // CTRL-D send EOF
                lineEditor.emulate_key_press(replxx::Replxx::KEY::control('U'));
                lineEditor.emulate_key_press(replxx::Replxx::KEY::control('K'));
                lineEditor.emulate_key_press(replxx::Replxx::KEY::control('D'));
            }}};
            datastore = std::make_shared<NetconfAccess>(process.std_out.native_source(), process.std_in.native_sink());
        } catch (std::runtime_error& ex) {
            std::cerr << "SSH connection failed: " << ex.what() << std::endl;
            return 1;
        }
    }
    std::cout << "Connected via NETCONF [datastore target: " << datastoreTargetString << "]" << std::endl;
#else
#error "Unknown CLI backend"
#endif

    datastore->setTarget(datastoreTarget);

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

    lineEditor.bind_key(Replxx::KEY::meta(Replxx::KEY::BACKSPACE), [&lineEditor](const auto& code) {
        return lineEditor.invoke(Replxx::ACTION::KILL_TO_BEGINING_OF_WORD, code);
    });
    lineEditor.bind_key(Replxx::KEY::control('W'), [&lineEditor](const auto& code) {
        return lineEditor.invoke(Replxx::ACTION::KILL_TO_WHITESPACE_ON_LEFT, code);
    });

    lineEditor.set_word_break_characters("\t _[]/:'\"=-%");

    lineEditor.set_completion_callback([&parser](const std::string& input, int& context) {
        std::stringstream stream;
        Completions completions;
        try {
            completions = parser.completeCommand(input, stream);
        } catch (std::exception& ex) {
            std::cerr << "Error while completing: " << ex.what() << "\n";
        }

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

    while (backendReturnCode == 0) {
        auto fullContextPath = parser.currentNode();
        std::string prompt;
        if (auto activeRpcPath = proxyDatastore.inputDatastorePath()) {
            auto rpcPrefixLength = activeRpcPath->size();
            prompt = "(prepare: " + *activeRpcPath + ") " + fullContextPath.substr(rpcPrefixLength);
        } else {
            prompt = fullContextPath;
        }

        prompt += "> ";

        auto line = lineEditor.input(prompt);
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
            if (cmd.type() == typeid(quit_)) {
                break;
            }
        } catch (InvalidCommandException& ex) {
            std::cerr << ex.what() << std::endl;
        } catch (DatastoreException& ex) {
            std::cerr << ex.what() << std::endl;
        } catch (std::runtime_error& ex) {
            std::cerr << ex.what() << std::endl;
        } catch (std::logic_error& ex) {
            std::cerr << ex.what() << std::endl;
        }

        lineEditor.history_add(line);
    }

    if (historyFile) {
        lineEditor.history_save(historyFile.value());
    }

    return backendReturnCode;
}
