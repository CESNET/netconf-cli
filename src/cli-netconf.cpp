/*
 * Copyright (C) 2020 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
*/

// #include <boost/spirit/home/x3.hpp>
// #include <iostream>
// #include <libssh/callbacks.h>
// #include <libssh/libssh.h>
// #include <pwd.h>
// #include <termios.h>
#include <stdexcept>
#include <unistd.h>
#include "cli-netconf.hpp"
// #include "netconf_access.hpp"
// #include "UniqueResource.hpp"
// #include "utils.hpp"
// BOOST_FUSION_ADAPT_STRUCT(SshOptions, m_user, m_hostname, m_port)


// void SshKeyDeleter::operator()(ssh_key_struct* key) const
// {
//     ssh_key_free(key);
// }

// void SshSessionDeleter::operator()(ssh_session_struct* session) const
// {
//     ssh_free(session);
// }

// void SshStringDeleter::operator()(char* str) const
// {
//     ssh_string_free_char(str);
// }

// namespace {
// namespace cliParsers {

// namespace x3 = boost::spirit::x3;
// using x3::alnum;
// using x3::char_;
// using x3::uint_;
// auto atSignIfNotLast = &('@' >> *(char_-'@') >> '@') >> char_("@");
// auto userAllowedChar = x3::rule<class UserAllowedChar, char>{"UserAllowedChar"} = ((char_-'@') | atSignIfNotLast);
// auto user = x3::rule<class User, std::string>{"User"} = +userAllowedChar >> '@';
// auto host = x3::rule<class Host, std::string>{"Host"} = (alnum >> *(-char_('.') >> (alnum | '-')));
// auto port = x3::rule<class Port, unsigned int>{"Port"} = ":" >> uint_;

// auto sshOptions = x3::rule<class Host, SshOptions>{"Hostname"} = -user >> host >> -port;
// }

// std::string getKeyDir()
// {
//     auto homeEnv = getenv("HOME");
//     std::string homePath;
//     if (homeEnv) {
//         homePath = homeEnv;
//     } else {
//         homePath = getpwuid(getuid())->pw_dir;
//     }

//     return joinPaths(homePath, ".ssh");
// }

// struct unlockPrivKeyCallbackData {
//     const std::string& m_path;
//     bool m_userEnteredNothing;
// };

// // This function will get simpler if https://bugs.libssh.org/T217 gets added. I
// // will probably use the automatic pubkey auth function
// int unlockPrivKeyCallback([[maybe_unused]] const char* prompt, char* buf, size_t len, int echo, int verify, void* userdata)
// {
//     using namespace std::string_literals;
//     auto data = reinterpret_cast<unlockPrivKeyCallbackData*>(userdata);

//     auto status = ssh_getpass((("Enter passphrase for key '") + data->m_path + "': ").c_str(), buf, len, echo, verify);

//     if (buf == ""s) {
//         data->m_userEnteredNothing = true;
//     }

//     return status;
// }

// auto defaultPubKeyFilename = "id_rsa.pub";
// auto defaultPrivKeyFilename = "id_rsa";

// enum class KeyType {
//     Private,
//     Public
// };

// template <KeyType TYPE>
// auto importKey(const std::string& path)
// {
//     ssh_key_struct* keyPtr;
//     int status;
//     switch (TYPE) {
//     case KeyType::Public:
//         status = ssh_pki_import_pubkey_file(path.c_str(), &keyPtr);
//         break;
//     case KeyType::Private:
//         unlockPrivKeyCallbackData data{path, false};
//         do {
//             status = ssh_pki_import_privkey_file(path.c_str(),
//                                                  nullptr,
//                                                  unlockPrivKeyCallback,
//                                                  reinterpret_cast<void*>(&data),
//                                                  &keyPtr);
//         } while (status == SSH_ERROR && !data.m_userEnteredNothing); // If user entered nothing, we'll give up.
//         break;
//     }

//     return std::unique_ptr<ssh_key_struct, SshKeyDeleter>{status == SSH_OK ? keyPtr : nullptr};
// }

// bool authenticatePubKey(const SshSessionUniquePtr& session)
// {
//     // Use a default key dir.
//     auto keyDir = getKeyDir();

//     // Let's try to import the pubkey.
//     auto pubKeyPath = joinPaths(keyDir, defaultPubKeyFilename);
//     auto pubKey = importKey<KeyType::Public>(pubKeyPath);
//     if (!pubKey) {
//         return false;
//     }

//     // Okay, the pubkey was imported successfully, let's see if the server accepts it.
//     if (ssh_userauth_try_publickey(session.get(), nullptr, pubKey.get()) != SSH_OK) {
//         return false;
//     }

//     // Good, the server will accept our key, let's import the private key.
//     auto privKeyPath = joinPaths(keyDir, defaultPrivKeyFilename);
//     auto privKey = importKey<KeyType::Private>(privKeyPath);
//     if (!privKey) {
//         return false;
//     }

//     // Great, the privkey was imported successfully, let's try to authenticate.
//     return (ssh_userauth_publickey(session.get(), nullptr, privKey.get()) == SSH_AUTH_SUCCESS);
// }

// enum class Echo {
//     On,
//     Off
// };

// void terminalEcho(Echo x)
// {
//     struct termios tty;
//     tcgetattr(STDIN_FILENO, &tty);
//     if (x == Echo::On)
//         tty.c_lflag |= ECHO;
//     else
//         tty.c_lflag &= ~ECHO;

//     tcsetattr(STDIN_FILENO, TCSANOW, &tty);
// }

// std::string getPass(Echo echo)
// {
//     auto resource = make_unique_resource([echo]() {
//         if (echo == Echo::Off) {
//             terminalEcho(Echo::Off);
//         }
//     }, [echo]() {
//         if (echo == Echo::Off) {
//         terminalEcho(Echo::On);
//         // User presses enter to accept, but no newline will be printed, so let's do that ourselves instead.
//         std::cout << std::endl;
//         }
//     });

//     std::string pass;
//     std::getline(std::cin, pass);

//     return pass;
// }

// bool authenticateKbdInteractive(const SshSessionUniquePtr& session)
// {
//     int status;
//     while ((status = ssh_userauth_kbdint(session.get(), nullptr, nullptr)) == SSH_AUTH_INFO) {
//         std::string_view name{ssh_userauth_kbdint_getname(session.get())};
//         std::string_view instruction{ssh_userauth_kbdint_getinstruction(session.get())};
//         auto n{ssh_userauth_kbdint_getnprompts(session.get())};

//         if (!name.empty()) {
//             std::cout << name << std::endl;
//         }

//         if (!instruction.empty()) {
//             std::cout << instruction << std::endl;
//         }

//         for (int i = 0; i < n; i++) {
//             char echo;
//             auto prompt = ssh_userauth_kbdint_getprompt(session.get(), i, &echo);
//             std::cout << prompt;

//             auto answer = getPass(echo ? Echo::On : Echo::Off);
//             if (ssh_userauth_kbdint_setanswer(session.get(), i, answer.c_str()) != 0) {
//                 return false;
//             }
//         }
//     }

//     if (status == SSH_AUTH_SUCCESS) {
//         return true;
//     }

//     return false;
// }

// bool authenticatePassword(const SshSessionUniquePtr& session)
// {
//     std::cout << "Password: ";
//     auto password = getPass(Echo::Off);
//     return ssh_userauth_password(session.get(), nullptr, password.c_str()) == SSH_AUTH_SUCCESS;
// }

// auto getSshOption(const SshSessionUniquePtr& session, ssh_options_e option)
// {
//     char* value;
//     auto status = ssh_options_get(session.get(), option, &value);
//     return std::unique_ptr<char, SshStringDeleter>{status == SSH_OK ? value : nullptr};
// }

// struct KeyFingerprint {
//     std::string m_fingerprint;
//     std::string m_keyType;
// };



// KeyFingerprint getHostKeyFingerPrint(const SshSessionUniquePtr& session)
// {
//     ssh_key_struct* key;
//     ssh_get_server_publickey(session.get(), &key);
//     std::unique_ptr<ssh_key_struct, SshKeyDeleter> keyPtr(key);
//     unsigned char* keyHash;
//     size_t keyHashLen;
//     ssh_get_publickey_hash(keyPtr.get(), SSH_PUBLICKEY_HASH_SHA256, &keyHash, &keyHashLen);
//     auto keyHashStr = ssh_get_hexa(keyHash, keyHashLen);
//     std::unique_ptr<char, SshStringDeleter> keyHashStrPtr(keyHashStr);
//     ssh_clean_pubkey_hash(&keyHash); // I would love to use a unique_ptr wrapper like I use everywhere, but libssh needs a double pointer here for some reason...

//     auto keyType = ssh_key_type_to_char(ssh_key_type(key));

//     return KeyFingerprint{keyHashStrPtr.get(), keyType};
// }

// bool verifyHost(const SshSessionUniquePtr& session)
// {
//     switch (ssh_session_is_known_server(session.get())) {
//     case SSH_KNOWN_HOSTS_OK:
//         return true;
//     case SSH_KNOWN_HOSTS_NOT_FOUND: // The known hosts file doesn't exist
//         [[fallthrough]];
//     case SSH_KNOWN_HOSTS_UNKNOWN: // Known hosts file exists, but the host is not present
//     {
//         unsigned int port;
//         ssh_options_get_port(session.get(), &port);
//         std::cout << "The authenticity of host '" << getSshOption(session, SSH_OPTIONS_HOST).get() << ":" << port << "' can't be established." << std::endl;
//         auto fingerprint = getHostKeyFingerPrint(session);
//         std::cout << fingerprint.m_keyType << " key fingerprint is " << fingerprint.m_fingerprint << "." << std::endl;
//         std::cout << "Are you sure you want to continue connecting (yes/no)? ";
//         while (true) {
//             std::string answer;
//             std::getline(std::cin, answer);
//             if (answer == "yes") {
//                 ssh_session_update_known_hosts(session.get());
//                 std::cout << "Warning: Permanently added '" << getSshOption(session, SSH_OPTIONS_HOST).get() << "' (ECDSA) to the list of known hosts.  " << std::endl;
//                 return true;
//             }
//             if (answer == "no") {
//                 std::cout << "Host key verification failed." << std::endl;
//                 return false;
//             }

//             std::cout << "Please type 'yes' or 'no': ";
//         }
//     }
//     case SSH_KNOWN_HOSTS_ERROR:
//         return false;
//     case SSH_KNOWN_HOSTS_OTHER: // The server gave a different key type, that's almost as if it gave a wrong one. TODO: Maybe it should give a different message anyway?
//         [[fallthrough]];
//     case SSH_KNOWN_HOSTS_CHANGED:
//         std::cout << "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@" << std::endl;
//         std::cout << "@    WARNING: REMOTE HOST IDENTIFICATION HAS CHANGED!     @" << std::endl;
//         std::cout << "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@" << std::endl;
//         std::cout << "IT IS POSSIBLE THAT SOMEONE IS DOING SOMETHING NASTY!" << std::endl;
//         std::cout << "Someone could be eavesdropping on you right now (man-in-the-middle attack)!" << std::endl;
//         std::cout << "It is also possible that a host key has just been changed." << std::endl;
//         auto fingerprint = getHostKeyFingerPrint(session);
//         std::cout << "The fingerprint for the " << fingerprint.m_keyType << "key sent by the remote host is" << std::endl;
//         std::cout << fingerprint.m_fingerprint << std::endl;
//         std::cout << "Please contact your system administrator." << std::endl;
//         std::cout << "Add correct host key in " << joinPaths(getKeyDir(), "known_hosts") << " to get rid of this message." << std::endl;
//         std::cout << fingerprint.m_keyType << " host key for localhost has changed and you have requested strict checking." << std::endl;
//         std::cout << "Host key verification failed." << std::endl;
//         return false;
//     }

//     __builtin_unreachable();
// }
// }

// SshOptions parseHostname(const std::string& hostname)
// {
//     using namespace std::string_literals;
//     SshOptions res;

//     auto it = hostname.begin();
//     auto parseResult = boost::spirit::x3::parse(it, hostname.end(), cliParsers::sshOptions, res);
//     if (!parseResult || it != hostname.end()) {
//         auto error = "Failed to parse hostname here:\n"s;
//         error += hostname + "\n";
//         error += std::string(it - hostname.begin(), ' ');
//         error += '^';
//         throw std::runtime_error{error};
//     }

//     return res;
// }

// // I would love to use ssh::Session wrapper libssh has, but libnetconf2 wants
// // to manage the session by itself and while it is possible to get the session
// // pointer from the wrapper, I can't stop the wrapper from freeing it. Also,
// // I'm not using the `ssh_session` typedef, it's very misleading.
// SshSessionUniquePtr createSshSession(const SshOptions& options)
// {
//     auto session{std::unique_ptr<ssh_session_struct, SshSessionDeleter>(ssh_new())};
//     ssh_options_set(session.get(), SSH_OPTIONS_HOST, options.m_hostname.c_str());
//     if (options.m_port) {
//         ssh_options_set(session.get(), SSH_OPTIONS_PORT, &(*options.m_port));
//     }
//     if (options.m_user) {
//         ssh_options_set(session.get(), SSH_OPTIONS_USER, options.m_user->c_str());
//     }

//     if (ssh_connect(session.get()) != SSH_OK) {
//         std::ostringstream ss;
//         ss << "ssh: " << ssh_get_error(session.get()) << " (" << ssh_get_error_code(session.get()) << ")";
//         throw std::runtime_error{ss.str()};
//     }

//     if (!verifyHost(session)) {
//         throw std::runtime_error{"ssh: Host key verification failed."};
//     }

//     // Try ssh-agent
//     if (ssh_userauth_agent(session.get(), nullptr) == SSH_AUTH_SUCCESS) {
//         return session;
//     }

//     // Try pubkey authentication
//     if (authenticatePubKey(session)) {
//         return session;
//     }

//     // Try keyboard-interactive authentication
//     if (authenticateKbdInteractive(session)) {
//         return session;
//     }

//     // Try password authentication
//     if (authenticatePassword(session)) {
//         return session;
//     }

//     throw std::runtime_error{"ssh: Permission denied."};
// }


InOutFd getSshFds(char* args[])
{
    int inpipefd[2];
    int outpipefd[2];

    pipe(inpipefd);
    pipe(outpipefd);
    if (fork() == 0) {
        execvp("ssh", args);
        throw std::runtime_error("openssh connection failed");
    }

    return {-1, -1};
}
