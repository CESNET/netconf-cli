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
#if defined(sysrepo_CLI)
#include "sysrepo_access.hpp"
#define PROGRAM_NAME "sysrepo-cli"
static const char usage[] = R"(CLI interface to sysrepo

Usage:
  sysrepo-cli \
  sysrepo-cli (-h | --help)
  sysrepo-cli --version
)";
#elif defined(netconf_CLI)
#include <libssh/libsshpp.hpp>
#include "netconf_access.hpp"
#define PROGRAM_NAME "netconf-cli"
static const char usage[] = R"(CLI interface to remote NETCONF hosts

Usage:
  netconf-cli [USER@]<hostname>[:PORT]
  netconf-cli (-h | --help)
  netconf-cli --version
)";
#else
#error "Unknown CLI backend"
#endif

struct SshOptions {
    std::string m_hostname;
    std::optional<std::string> m_port = std::nullopt;
    std::optional<std::string> m_user = std::nullopt;
};

#if defined (netconf_CLI)
SshOptions parseHostname(const std::string& hostname)
{
    SshOptions res;

    auto it = 0;
    if (auto userEnd = hostname.find('@'); userEnd != std::string::npos) {
        res.m_user = hostname.substr(0, userEnd);
        it = userEnd;
    }

    auto hostnameEnd = hostname.find(':');
    res.m_hostname = hostname.substr(it, hostnameEnd);

    if (hostnameEnd != std::string::npos) {
        res.m_port = hostname.substr(hostnameEnd + 1);
    }

    return res;
}

std::unique_ptr<ssh::Session> createSshSession(const SshOptions& options)
{
    auto session = std::make_unique<ssh::Session>();
    session->setOption(SSH_OPTIONS_HOST, "127.0.0.1");
    if (options.m_port) {
        session->setOption(SSH_OPTIONS_PORT_STR, options.m_port->c_str());
    }
    session->setOption(SSH_OPTIONS_LOG_VERBOSITY, 9999);
    if (options.m_user) {
        session->setOption(SSH_OPTIONS_USER, options.m_user->c_str());
    }
    try {
        session->connect();
        session->userauthPublickeyAuto();
    } catch (ssh::SshException& ex) {
        std::cout << "ex.getCode() = " << ex.getCode() << std::endl;
        std::cout << "ex.getError()  = " << ex.getError() << std::endl;
    }

    return session;
}
#endif

const auto HISTORY_FILE_NAME = PROGRAM_NAME "_history";

int main(int argc, char* argv[])
{
    auto args = docopt::docopt(usage,
                               {argv + 1, argv + argc},
                               true,
                               PROGRAM_NAME " " NETCONF_CLI_VERSION,
                               true);
    std::cout << "Welcome to " PROGRAM_NAME << std::endl;

#if defined(sysrepo_CLI)
    SysrepoAccess datastore(PROGRAM_NAME);
#elif defined(netconf_CLI)
    ssh_set_log_level(9999);
    auto options = parseHostname(args.at("<hostname>").asString());
    NetconfAccess datastore(createSshSession(options));
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
