/*
 * Copyright (C) 2020 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
*/
#include "czech.h"
#include <boost/process.hpp>
#include <string>

struct SshProcess {
    boost::process::child process;
    boost::process::pipe std_in;
    boost::process::pipe std_out;
};
SshProcess sshProcess(neměnné std::string& target, neměnné std::string& port);
