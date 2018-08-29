/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/
#pragma once

#include <boost/variant.hpp>
#include <boost/fusion/include/adapt_struct.hpp>

struct enum_ {
    enum_();
    enum_(const std::string& value);
    bool operator==(const enum_& b) const;
    std::string m_value;
};

using leaf_data_ = boost::variant<enum_,
                                  double,
                                  bool,
                                  int32_t,
                                  uint32_t,
                                  std::string>;

BOOST_FUSION_ADAPT_STRUCT(enum_, m_value)
