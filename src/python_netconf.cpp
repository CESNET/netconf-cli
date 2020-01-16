/*
 * Copyright (C) 2020 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Jan Kundrát <jan.kundrat@cesnet.cz>
 *
*/

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
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
        static auto call(Args &&...args) -> decltype(boost::apply_visitor(args...)) {
            return boost::apply_visitor(args...);
        }
    };
}


PYBIND11_MODULE(netconf_cli_py, m) {
    m.doc() = "Python bindings for accessing NETCONF servers";

    pybind11::class_<special_>(m, "YangSpecial")
            .def("__repr__",
                 [](const special_ s) {
                    return "<netconf_cli_py.YangSpecial " + specialValueToString(s) + ">";
                 });

    pybind11::class_<enum_>(m, "YangEnum")
            .def("__repr__",
                 [](const enum_ v) {
                    return "<netconf_cli_py.YangEnum '" + v.m_value + "'>";
                 });

    pybind11::class_<binary_>(m, "YangBinary")
            .def("__repr__",
                 [](const binary_ v) {
                    return "<netconf_cli_py.YangBinary '" + v.m_value + "'>";
                 });

    pybind11::class_<identityRef_>(m, "YangIdentityRef")
            .def("__repr__",
                 [](const identityRef_ v) {
                    return "<netconf_cli_py.YangIdentityRef '"s
                            + (v.m_prefix ? v.m_prefix->m_name + ":" : ""s) + v.m_value + "'>";
                 });

    pybind11::class_<NetconfAccess>(m, "NetconfAccess")
            .def(pybind11::init<const std::string&>(), "socketPath"_a)
            .def("getItems", &NetconfAccess::getItems, "xpath"_a)
            .def("setLeaf", &NetconfAccess::setLeaf, "xpath"_a, "value"_a)
            .def("commitChanges", &NetconfAccess::commitChanges)
            ;
}
