/*
 * Copyright (C) 2020 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
*/

#include <boost/spirit/home/x3.hpp>
#include <boost/fusion/adapted.hpp>
#include <optional>
#include <stdexcept>
#include <unistd.h>
#include "cli-netconf.hpp"

struct SshOptions {
    std::string m_hostname;
    std::optional<unsigned int> m_port = std::nullopt;
    std::optional<std::string> m_user = std::nullopt;
};

BOOST_FUSION_ADAPT_STRUCT(SshOptions, m_user, m_hostname, m_port);

namespace {
namespace cliParsers {

namespace x3 = boost::spirit::x3;
using x3::alnum;
using x3::char_;
using x3::uint_;
auto atSignIfNotLast = &('@' >> *(char_-'@') >> '@') >> char_("@");
auto userAllowedChar = x3::rule<class UserAllowedChar, char>{"UserAllowedChar"} = ((char_-'@') | atSignIfNotLast);
auto user = x3::rule<class User, std::string>{"User"} = +userAllowedChar >> '@';
auto host = x3::rule<class Host, std::string>{"Host"} = (alnum >> *(-char_('.') >> (alnum | '-')));
auto port = x3::rule<class Port, unsigned int>{"Port"} = ":" >> uint_;

auto sshOptions = x3::rule<class Host, SshOptions>{"Hostname"} = -user >> host >> -port;
}
}


SshOptions parseHostname(const std::string& hostname)
{
    using namespace std::string_literals;
    SshOptions res;

    auto it = hostname.begin();
    auto parseResult = boost::spirit::x3::parse(it, hostname.end(), cliParsers::sshOptions, res);
    if (!parseResult || it != hostname.end()) {
        auto error = "Failed to parse hostname here:\n"s;
        error += hostname + "\n";
        error += std::string(it - hostname.begin(), ' ');
        error += '^';
        throw std::runtime_error{error};
    }

    return res;
}

namespace bp = boost::process;
// FIXME: name this function something like connectOpenSSH or something
OpenSshProcess getSsh(const std::string& hostname)
{
    auto options = parseHostname(hostname);
    bp::pipe in;
    bp::pipe out;
    bp::child ssh("ssh " + options.m_hostname + " -p " + std::to_string(*options.m_port) + " -s netconf", bp::std_out > out, bp::std_in < in);

    // Designated initializers would be best for this, but I don't know how to enable that
    return {std::move(ssh), std::move(in), std::move(out)};
}
