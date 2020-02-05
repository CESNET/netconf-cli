/*
 * Copyright (C) 2020 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
*/
// #include <memory>
// #include <optional>
// #include <string>

// struct SshOptions {
//     std::string m_hostname;
//     std::optional<unsigned int> m_port = std::nullopt;
//     std::optional<std::string> m_user = std::nullopt;
// };
// struct ssh_session_struct;
// struct ssh_key_struct;
// struct SshSessionDeleter;
// using SshSessionUniquePtr = std::unique_ptr<ssh_session_struct, SshSessionDeleter>;

// struct SshKeyDeleter {
//     void operator()(ssh_key_struct* key) const;
// };

// struct SshSessionDeleter {
//     void operator()(ssh_session_struct* session) const;
// };

// struct SshStringDeleter {
//     void operator()(char* str) const;
// };

// SshOptions parseHostname(const std::string& hostname);
// SshSessionUniquePtr createSshSession(const SshOptions& options);

struct InOutFd {
    int in;
    int out;
};

InOutFd getSshFds(char* args[]);

