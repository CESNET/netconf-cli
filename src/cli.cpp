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
#include <boost/spirit/home/x3.hpp>
#include <libssh/libsshpp.hpp>
#include "netconf_access.hpp"
#define PROGRAM_NAME "netconf-cli"
static const char usage[] = R"(CLI interface to remote NETCONF hosts

Usage:
  netconf-cli [-v] [USER@]<hostname>[:PORT]
  netconf-cli (-h | --help)
  netconf-cli --version

Options:
  -v  enable verbose mode
)";
#else
#error "Unknown CLI backend"
#endif


#if defined (netconf_CLI)
struct SshOptions {
    std::string m_hostname;
    std::optional<unsigned int> m_port = std::nullopt;
    std::optional<std::string> m_user = std::nullopt;
};
BOOST_FUSION_ADAPT_STRUCT(SshOptions, m_user, m_hostname, m_port)

namespace x3 = boost::spirit::x3;
namespace cliParsers {
// TODO: allow more usernames
auto user = x3::rule<class User, std::string>{"user"} = x3::alnum >> *(x3::alnum | "-") >> "@";
auto host = x3::rule<class Host, std::string>{"name of the host"} = x3::alnum >> *(x3::alnum | "-");
auto port = ":" >> x3::uint_;

auto sshOptions = x3::rule<class Host, SshOptions>{"Hostname"} = -user >> host >> -port;
}


SshOptions parseHostname(const std::string& hostname)
{
    SshOptions res;

    auto it = hostname.begin();
    x3::parse(it, hostname.end(), cliParsers::sshOptions, res);

    return res;
}

// I would love to use ssh::Session wrapper libssh has, but libnetconf2 wants
// to manage the session by itself and while it is possible to get the session
// pointer from the wrapper, I can't stop the wrapper it from freeing it. Also,
// I'm not using the `ssh_session` typedef, it's very misleading.
ssh_session_struct* createSshSession(const SshOptions& options)
{
    auto session = ssh_new();
    ssh_options_set(session, SSH_OPTIONS_HOST, options.m_hostname.c_str());
    if (options.m_port) {
        ssh_options_set(session, SSH_OPTIONS_PORT, &(*options.m_port));
    }
    if (options.m_user) {
        ssh_options_set(session, SSH_OPTIONS_USER, options.m_user->c_str());
    }

    ssh_connect(session);
    ssh_userauth_agent(session, nullptr);

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
    auto options = parseHostname(args.at("<hostname>").asString());
    auto verbose = args.at("-v").asBool();

    ssh_session_struct* session;
    try {
        session = createSshSession(options);
    } catch (ssh::SshException& ex) {
        std::cerr << "SSH server connection failed." << std::endl;
        std::cerr << "Reason: " << ex.getError() << std::endl;
        std::cerr << "Error code: " << ex.getCode() << std::endl;
        return 1;
    }
    NetconfAccess::setNcLogCallback([verbose] (NC_VERB_LEVEL level, const char* message) {
        if (verbose) {
            std::cerr << "libnetconf[" << level << "]" << ": " << message << std::endl;
        }
    });
    NetconfAccess datastore(session);
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
