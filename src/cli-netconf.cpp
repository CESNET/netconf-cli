/*
 * Copyright (C) 2020 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
*/

#include <boost/fusion/adapted.hpp>
#include <boost/spirit/home/x3.hpp>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <unistd.h>
#include "cli-netconf.hpp"

SshProcess sshProcess(const std::string& target, const std::string& port)
{
    namespace bp = boost::process;
    bp::pipe in;
    bp::pipe out;
    auto sshPath = bp::search_path("ssh");
    if (sshPath.empty()) {
        throw std::runtime_error("ssh not found in PATH.");
    }
    if (target.front() == '@') {
        throw std::runtime_error("Invalid username.");
    }

    bp::child ssh(sshPath,
            target,
            "-p",
            port,
            "-s",
            "netconf",
            bp::std_out > out, bp::std_in < in);

    return {std::move(ssh), std::move(in), std::move(out)};
}
