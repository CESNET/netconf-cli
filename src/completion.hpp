/*
 * Copyright (C) 2020 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
*/
#include <set>
#include <string>
struct Completion {
    enum class WhenToAdd {
        Always,
        IfFullMatch
    };
    bool operator<(const Completion& b) const;
    bool operator==(const Completion& b) const;
    std::string m_value;
    // If the parser determines that suggestions are unambiguous (after
    // filtering by prefix), this suffix gets added to the completion (for
    // example a left bracket after a list). If m_whenToAdd is IfFullMatch, the
    // suffix only gets added if the prefix matches fully.
    std::string m_suffix = "";
    WhenToAdd m_whenToAdd = WhenToAdd::Always;
};

/** Returns a subset of the original set with only the strings starting with prefix */
std::set<Completion> filterByPrefix(const std::set<Completion>& set, const std::string_view prefix);

