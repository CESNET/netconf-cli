/*
 * Copyright (C) 2020 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
*/

#include <pybind11/pybind11.h>
#include "sysrepo_subscription.hpp"

using namespace pybind11::literals;

PYBIND11_MODULE(sysrepo_subscription_py, m)
{
    m.doc() = "SysrepoSubscription Python interface";

    pybind11::class_<SysrepoSubscription>(m, "SysrepoSubscription")
        .def(pybind11::init<const std::string&>(), "moduleName"_a);
}
