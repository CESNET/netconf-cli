/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/
#pragma once

#include <boost/optional.hpp>
#include <boost/variant.hpp>

struct enum_ {
    enum_();
    enum_(const std::string& value);
    bool operator==(const enum_& b) const;
    std::string m_value;
};

struct binary_ {
    binary_();
    binary_(const std::string& value);
    bool operator==(const binary_& b) const;
    std::string m_value;
};

struct module_ {
    bool operator==(const module_& b) const;
    std::string m_name;
};

struct identityRef_ {
    identityRef_();
    identityRef_(const std::string& module, const std::string& value);
    identityRef_(const std::string& value);
    bool operator==(const identityRef_& b) const;
    boost::optional<module_> m_prefix;
    std::string m_value;
};

struct special_ {
    bool operator==(const special_& b) const;
    std::string m_value;
};

using leaf_data_ = boost::variant<enum_,
                                  binary_,
                                  identityRef_,
                                  special_,
                                  double,
                                  bool,
                                  int8_t,
                                  uint8_t,
                                  int16_t,
                                  uint16_t,
                                  int32_t,
                                  uint32_t,
                                  int64_t,
                                  uint64_t,
                                  std::string>;
