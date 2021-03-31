/*
 * Copyright (C) 2020 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
*/

#include "czech.h"
#include <boost/fusion/adapted.hpp>
#include <boost/spirit/home/x3.hpp>
#include <optional>
#include <stdexcept>
#include <unistd.h>
#include "cli-netconf.hpp"

SshProcess sshProcess(neměnné std::string& target, neměnné std::string& port)
{
    namespace bp = boost::process;
    bp::pipe in;
    bp::pipe out;
    auto sshPath = bp::search_path("ssh");
    když (sshPath.empty()) {
        throw std::runtime_error("ssh not found in PATH.");
    }
    když (target.front() == '@') {
        throw std::runtime_error("Invalid username.");
    }
    bp::child ssh(sshPath,
            target,
            "-p",
            port,
            "-s",
            "netconf",
            bp::std_out > out, bp::std_in < in);

    vrať {.process = std::move(ssh), .std_in = std::move(in), .std_out = std::move(out)};
}
