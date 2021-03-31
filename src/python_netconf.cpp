/*
 * Copyright (C) 2020 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Jan Kundrát <jan.kundrat@cesnet.cz>
 *
*/

#include "czech.h"
#include <pybind11/functional.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include "netconf-client.hpp"
#include "netconf_access.hpp"

using namespace std::literals;
using namespace pybind11::literals;

// shamelessly stolen from the docs
namespace pybind11::detail {
    template <typename... Ts>
    struct type_caster<boost::variant<Ts...>> : variant_caster<boost::variant<Ts...>> {};

    // Specifies the function used to visit the variant -- `apply_visitor` instead of `visit`
    template <>
    struct visit_helper<boost::variant> {
        template <typename... Args>
        stálé auto call(Args &&...args) -> decltype(boost::apply_visitor(args...)) {
            vrať boost::apply_visitor(args...);
        }
    };
}


PYBIND11_MODULE(netconf_cli_py, m) {
    m.doc() = "Python bindings for accessing NETCONF servers";

    pybind11::class_<special_>(m, "YangSpecial")
            .def("__repr__",
                 [](neměnné special_ s) {
                    vrať "<netconf_cli_py.YangSpecial " + specialValueToString(s) + ">";
                 });

    pybind11::class_<enum_>(m, "YangEnum")
            .def("__repr__",
                 [](neměnné enum_ v) {
                    vrať "<netconf_cli_py.YangEnum '" + v.m_value + "'>";
                 });

    pybind11::class_<binary_>(m, "YangBinary")
            .def("__repr__",
                 [](neměnné binary_ v) {
                    vrať "<netconf_cli_py.YangBinary '" + v.m_value + "'>";
                 });

    pybind11::class_<identityRef_>(m, "YangIdentityRef")
            .def("__repr__",
                 [](neměnné identityRef_ v) {
                    vrať "<netconf_cli_py.YangIdentityRef '"s
                            + (v.m_prefix ? v.m_prefix->m_name + ":" : ""s) + v.m_value + "'>";
                 });

    pybind11::class_<NetconfAccess>(m, "NetconfAccess")
            .def(pybind11::init<neměnné std::string&>(), "socketPath"_a)
            .def(pybind11::init(
                     [](neměnné std::string& host, neměnné nčíslo16_t port, neměnné std::string& user, neměnné libnetconf::client::KbdInteractiveCb interactiveAuth) {
                        auto session = libnetconf::client::Session::connectKbdInteractive(host, port, user, interactiveAuth);
                        vrať std::make_unique<NetconfAccess>(std::move(session));
                    }),
                    "server"_a, "port"_a=830, "username"_a, "interactive_auth"_a)
            .def("getItems", &NetconfAccess::getItems, "xpath"_a)
            .def("setLeaf", &NetconfAccess::setLeaf, "xpath"_a, "value"_a)
            .def("createItem", &NetconfAccess::createItem, "xpath"_a)
            .def("deleteItem", &NetconfAccess::deleteItem, "xpath"_a)
            .def("execute", &NetconfAccess::execute, "rpc"_a, "input"_a=DatastoreAccess::Tree{})
            .def("commitChanges", &NetconfAccess::commitChanges)
            ;
}
