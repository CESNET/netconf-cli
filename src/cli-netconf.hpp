/*
 * Copyright (C) 2020 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
*/
#include <boost/process.hpp>
#include <string>

struct OpenSshProcess {
    boost::process::child process;
    boost::process::pipe in;
    boost::process::pipe out;
};
OpenSshProcess getSsh(const std::string& hostname);

