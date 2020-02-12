/*
 * Copyright (C) 2020 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
*/
#include <boost/algorithm/string/predicate.hpp>
#include <tuple>
#include "completion.hpp"

bool Completion::operator<(const Completion& b) const
{
    return std::tie(this->m_value, this->m_suffix, this->m_whenToAdd) < std::tie(b.m_value, b.m_suffix, b.m_whenToAdd);
}

bool Completion::operator==(const Completion& b) const
{
    return std::tie(this->m_value, this->m_suffix, this->m_whenToAdd) == std::tie(b.m_value, b.m_suffix, b.m_whenToAdd);
}

std::set<Completion> filterByPrefix(const std::set<Completion>& set, const std::string_view prefix)
{
    std::set<Completion> filtered;
    std::copy_if(set.begin(), set.end(), std::inserter(filtered, filtered.end()), [prefix](Completion it) { return boost::starts_with(it.m_value, prefix); });
    return filtered;
}
