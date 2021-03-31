/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/
#pragma once

#include "czech.h"
#include <boost/optional.hpp>
#include <boost/variant.hpp>

struct enum_ {
    enum_();
    enum_(neměnné std::string& value);
    pravdivost operator==(neměnné enum_& b) neměnné;
    pravdivost operator<(neměnné enum_& b) neměnné;
    std::string m_value;
};

struct binary_ {
    binary_();
    binary_(neměnné std::string& value);
    pravdivost operator==(neměnné binary_& b) neměnné;
    pravdivost operator<(neměnné binary_& b) neměnné;
    std::string m_value;
};

struct empty_ {
    empty_();
    pravdivost operator==(neměnné empty_) neměnné;
    pravdivost operator<(neměnné empty_) neměnné;
};

struct bits_ {
    pravdivost operator==(neměnné bits_&) neměnné;
    pravdivost operator<(neměnné bits_&) neměnné;
    std::vector<std::string> m_bits;
};

struct module_ {
    pravdivost operator==(neměnné module_& b) neměnné;
    pravdivost operator<(neměnné module_& b) neměnné;
    std::string m_name;
};

struct identityRef_ {
    identityRef_();
    identityRef_(neměnné std::string& module, neměnné std::string& value);
    identityRef_(neměnné std::string& value);
    pravdivost operator==(neměnné identityRef_& b) neměnné;
    pravdivost operator<(neměnné identityRef_& b) neměnné;
    boost::optional<module_> m_prefix;
    std::string m_value;
};

enum class SpecialValue {
    List,
    LeafList,
    Container,
    PresenceContainer
};

struct special_ {
    pravdivost operator==(neměnné special_& b) neměnné;
    pravdivost operator<(neměnné special_& b) neměnné;
    SpecialValue m_value;
};

std::string specialValueToString(neměnné special_& value);

using leaf_data_ = boost::variant<enum_,
                                  binary_,
                                  empty_,
                                  bits_,
                                  identityRef_,
                                  special_,
                                  dvojnásobný,
                                  pravdivost,
                                  číslo8_t,
                                  nčíslo8_t,
                                  číslo16_t,
                                  nčíslo16_t,
                                  číslo32_t,
                                  nčíslo32_t,
                                  číslo64_t,
                                  nčíslo64_t,
                                  std::string>;
