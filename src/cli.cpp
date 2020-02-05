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
  sysrepo-cli
  sysrepo-cli (-h | --help)
  sysrepo-cli --version
)";
#elif defined(netconf_CLI)
#include <boost/spirit/home/x3.hpp>
#include <libssh/callbacks.h>
#include <libssh/libssh.h>
#include <termios.h>
#include "netconf_access.hpp"
#include "utils.hpp"
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


#if defined(netconf_CLI)
struct SshOptions {
    std::string m_hostname;
    std::optional<unsigned int> m_port = std::nullopt;
    std::optional<std::string> m_user = std::nullopt;
};
BOOST_FUSION_ADAPT_STRUCT(SshOptions, m_user, m_hostname, m_port)

namespace x3 = boost::spirit::x3;
namespace cliParsers {
using x3::alnum;
using x3::char_;
using x3::uint_;
auto atSignIfNotLast = &('@' >> *(char_-'@') >> '@') >> char_("@");
// TODO: check which characters should be allowed
auto userAllowedChar = x3::rule<class UserAllowedChar, char>{"UserAllowedChar"} = (alnum | char_('.') | char_('-') | atSignIfNotLast);
auto user = x3::rule<class User, std::string>{"User"} = alnum >> *userAllowedChar >> '@';
auto host = x3::rule<class Host, std::string>{"Host"} = (alnum >> *(-char_('.') >> (alnum | '-')));
auto port = x3::rule<class Port, unsigned int>{"Port"} = ":" >> uint_;

auto sshOptions = x3::rule<class Host, SshOptions>{"Hostname"} = -user >> host >> -port;
}

namespace {
struct SshKeyDeleter {
    void operator()(ssh_key_struct* key) const
    {
        ssh_key_free(key);
    }
};

struct SshSessionDeleter {
    void operator()(ssh_session_struct* session) const
    {
        ssh_free(session);
    }
};

struct SshStringDeleter {
    void operator()(char* str) const
    {
        ssh_string_free_char(str);
    }
};

using SshSessionUniquePtr = std::unique_ptr<ssh_session_struct, SshSessionDeleter>;

SshOptions parseHostname(const std::string& hostname)
{
    using namespace std::string_literals;
    SshOptions res;

    auto it = hostname.begin();
    auto parseResult = x3::parse(it, hostname.end(), cliParsers::sshOptions, res);
    if (!parseResult || it != hostname.end()) {
        auto error = "Failed to parse hostname here:\n"s;
        error += hostname + "\n";
        error += std::string(it - hostname.begin(), ' ');
        error += '^';
        throw std::runtime_error{error};
    }

    return res;
}

std::string getKeyDir(const SshSessionUniquePtr& session)
{
    auto homeEnv = getenv("HOME");
    std::string homePath;
    if (homeEnv) {
        homePath = homeEnv;
    } else {
        char* user;
        ssh_options_get(session.get(), SSH_OPTIONS_USER, &user);
        homePath = joinPaths("/home", user);
    }

    return joinPaths(homePath, ".ssh");
}

struct unlockPrivKeyCallbackData {
    const std::string& m_path;
    bool m_askAgain;
    bool m_userEnteredNothing;
};

int unlockPrivKeyCallback([[maybe_unused]] const char* prompt, char* buf, size_t len, int echo, int verify, void* userdata)
{
    using namespace std::string_literals;
    auto data = reinterpret_cast<unlockPrivKeyCallbackData*>(userdata);
    auto keyPrompt = ("Enter passphrase for key '") + data->m_path + "': ";

    auto status = ssh_getpass(keyPrompt.c_str(), buf, len, echo, verify);

    if (buf == ""s) {
        data->m_userEnteredNothing = true;
    }

    return status;
}

auto defaultPubKeyFilename = "id_rsa.pub";
auto defaultPrivKeyFilename = "id_rsa";

enum class KeyType {
    Private,
    Public
};

template <KeyType TYPE>
auto importKey(const std::string& path)
{
    ssh_key_struct* keyPtr;
    int status;
    switch (TYPE) {
    case KeyType::Public:
        status = ssh_pki_import_pubkey_file(path.c_str(), &keyPtr);
        break;
    case KeyType::Private:
        unlockPrivKeyCallbackData data{path, false, false};
        do {
            status = ssh_pki_import_privkey_file(path.c_str(),
                                                 nullptr,
                                                 unlockPrivKeyCallback,
                                                 static_cast<void*>(&data),
                                                 &keyPtr);
        } while (status == SSH_ERROR && !data.m_userEnteredNothing);
        break;
    }

    return std::unique_ptr<ssh_key_struct, SshKeyDeleter>{status == SSH_OK ? keyPtr : nullptr};
}

bool authenticatePubKey(const SshSessionUniquePtr& session)
{
    // Use a default key dir.
    auto keyDir = getKeyDir(session);

    // Let's try to import the pubkey.
    auto pubKeyPath = joinPaths(keyDir, defaultPubKeyFilename);
    auto pubKey = importKey<KeyType::Public>(pubKeyPath);
    if (!pubKey) {
        return false;
    }

    // Okay, the pubkey was imported successfully, let's see if the server accepts it.
    if (ssh_userauth_try_publickey(session.get(), nullptr, pubKey.get()) != SSH_OK) {
        return false;
    }

    // Good, the server will accept our key, let's import the private key.
    auto privKeyPath = joinPaths(keyDir, defaultPrivKeyFilename);
    auto privKey = importKey<KeyType::Private>(privKeyPath);
    if (!privKey) {
        return false;
    }

    // Great, the privkey was imported successfully, let's try to authenticate.
    if (ssh_userauth_publickey(session.get(), nullptr, privKey.get()) != SSH_AUTH_SUCCESS) {
        return false;
    }

    // Wonderful, pubkey authentication was successful!
    return true;
}

enum class Echo {
    On,
    Off
};

void terminalEcho(Echo x)
{
    struct termios tty;
    tcgetattr(STDIN_FILENO, &tty);
    if (x == Echo::On)
        tty.c_lflag |= ECHO;
    else
        tty.c_lflag &= ~ECHO;

    tcsetattr(STDIN_FILENO, TCSANOW, &tty);
}

std::string getPass(Echo echo)
{
    if (echo == Echo::Off) {
        terminalEcho(Echo::Off);
    }
    std::string pass;
    std::getline(std::cin, pass);
    if (echo == Echo::Off) {
        terminalEcho(Echo::On);
        // User presses enter to accept, but no newline will be printed, so let's do that ourselves instead.
        std::cout << std::endl;
    }

    return pass;
}

bool authenticateKbdInteractive(const SshSessionUniquePtr& session)
{
    int status;
    while ((status = ssh_userauth_kbdint(session.get(), nullptr, nullptr)) == SSH_AUTH_INFO) {
        std::string_view name{ssh_userauth_kbdint_getname(session.get())};
        std::string_view instruction{ssh_userauth_kbdint_getinstruction(session.get())};
        auto n{ssh_userauth_kbdint_getnprompts(session.get())};

        if (!name.empty()) {
            std::cout << name << std::endl;
        }

        if (!instruction.empty()) {
            std::cout << instruction << std::endl;
        }

        for (int i = 0; i < n; i++) {
            char echo;
            auto prompt = ssh_userauth_kbdint_getprompt(session.get(), i, &echo);
            std::cout << prompt;

            auto answer = getPass(echo ? Echo::On : Echo::Off);
            if (ssh_userauth_kbdint_setanswer(session.get(), i, answer.c_str()) != 0) {
                return false;
            }
        }
    }

    if (status == SSH_AUTH_SUCCESS) {
        return true;
    }

    return false;
}

bool authenticatePassword(const SshSessionUniquePtr& session)
{
    std::cout << "Password SSH Authentication" << std::endl;
    std::cout << "Password: ";
    auto password = getPass(Echo::Off);
    if (ssh_userauth_password(session.get(), nullptr, password.c_str()) == SSH_AUTH_SUCCESS) {
        return true;
    }

    return false;
}

auto getSshOption(const SshSessionUniquePtr& session, ssh_options_e option)
{
    char* value;
    ssh_options_get(session.get(), option, &value);
    return std::unique_ptr<char, SshStringDeleter>{value};
}

std::string getHostKey(const SshSessionUniquePtr& session)
{
    char* key;
    ssh_session_export_known_hosts_entry(session.get(), &key);
    std::unique_ptr<char, SshStringDeleter> keyPtr(key);
    std::string_view keySv = keyPtr.get();

    return std::string{keySv.substr(keySv.find(' ') + 1, std::string_view::npos)};
}

bool verifyHost(const SshSessionUniquePtr& session)
{
    switch (ssh_session_is_known_server(session.get())) {
    case SSH_KNOWN_HOSTS_OK:
        return true;
    case SSH_KNOWN_HOSTS_NOT_FOUND: // The known hosts file doesn't exist
        [[fallthrough]];
    case SSH_KNOWN_HOSTS_UNKNOWN: // Known hosts file exists, but the host is not present
    {
        unsigned int port;
        ssh_options_get_port(session.get(), &port);
        std::cout << "The authenticity of host '" << getSshOption(session, SSH_OPTIONS_HOST).get() << ":" << port << "' can't be established." << std::endl;
        std::cout << "ECDSA key fingerprint is " << getHostKey(session) << "." << std::endl;
        std::cout << "Are you sure you want to continue connecting (yes/no)? ";
        while (true) {
            std::string answer;
            std::getline(std::cin, answer);
            if (answer == "yes") {
                ssh_session_update_known_hosts(session.get());
                std::cout << "Warning: Permanently added '" << getSshOption(session, SSH_OPTIONS_HOST).get() << "' (ECDSA) to the list of known hosts.  " << std::endl;
                return true;
            }
            if (answer == "no") {
                std::cout << "Host key verification failed." << std::endl;
                return false;
            }

            std::cout << "Please type 'yes' or 'no': ";
        }
    }
    case SSH_KNOWN_HOSTS_ERROR:
        return false;
    case SSH_KNOWN_HOSTS_OTHER: // The server gave a different key type, that's almost as if it gave a wrong one. TODO: Maybe it should give a different message anyway?
        [[fallthrough]];
    case SSH_KNOWN_HOSTS_CHANGED:
        std::cout << "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@" << std::endl;
        std::cout << "@    WARNING: REMOTE HOST IDENTIFICATION HAS CHANGED!     @" << std::endl;
        std::cout << "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@" << std::endl;
        std::cout << "IT IS POSSIBLE THAT SOMEONE IS DOING SOMETHING NASTY!" << std::endl;
        std::cout << "Someone could be eavesdropping on you right now (man-in-the-middle attack)!" << std::endl;
        std::cout << "It is also possible that a host key has just been changed." << std::endl;
        std::cout << "The fingerprint for the ECDSA key sent by the remote host is" << std::endl;
        std::cout << getHostKey(session);
        std::cout << "Please contact your system administrator." << std::endl;
        std::cout << "Add correct host key in " << joinPaths(getKeyDir(session), "known_hosts") << " to get rid of this message." << std::endl;
        std::cout << "ECDSA host key for localhost has changed and you have requested strict checking." << std::endl;
        std::cout << "Host key verification failed." << std::endl;
        return false;
    }

    __builtin_unreachable();
}



// I would love to use ssh::Session wrapper libssh has, but libnetconf2 wants
// to manage the session by itself and while it is possible to get the session
// pointer from the wrapper, I can't stop the wrapper from freeing it. Also,
// I'm not using the `ssh_session` typedef, it's very misleading.
ssh_session_struct* createSshSession(const SshOptions& options)
{
    auto session{std::unique_ptr<ssh_session_struct, SshSessionDeleter>(ssh_new())};
    ssh_options_set(session.get(), SSH_OPTIONS_HOST, options.m_hostname.c_str());
    if (options.m_port) {
        ssh_options_set(session.get(), SSH_OPTIONS_PORT, &(*options.m_port));
    }
    if (options.m_user) {
        ssh_options_set(session.get(), SSH_OPTIONS_USER, options.m_user->c_str());
    }

    if (ssh_connect(session.get()) != SSH_OK) {
        std::ostringstream ss;
        ss << "ssh: " << ssh_get_error(session.get()) << " (" << ssh_get_error_code(session.get()) << ")";
        throw std::runtime_error{ss.str()};
    }

    if (!verifyHost(session)) {
        throw std::runtime_error{"ssh: Host key verification failed."};
    }

    // Try ssh-agent
    if (ssh_userauth_agent(session.get(), nullptr) == SSH_AUTH_SUCCESS) {
        return session.release();
    }

    // Try pubkey authentication
    if (authenticatePubKey(session)) {
        return session.release();
    }

    // Try keyboard-interactive authentication
    if (authenticateKbdInteractive(session)) {
        return session.release();
    }

    // Try password authentication
    if (authenticatePassword(session)) {
        return session.release();
    }

    throw std::runtime_error{"ssh: Permission denied."};
}
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
    } catch (std::runtime_error& ex) {
        std::cerr << ex.what() << std::endl;
        return 1;
    }

    if (verbose) {
        NetconfAccess::setNcLogLevel(NC_VERB_DEBUG);
    }
    NetconfAccess::setNcLogCallback([] (NC_VERB_LEVEL level, const char* message) {
        std::cerr << "libnetconf[" << level << "]" << ": " << message << std::endl;
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
