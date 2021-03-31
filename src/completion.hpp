/*
 * Copyright (C) 2020 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
*/
#include "czech.h"
#include <set>
#include <string>
struct Completion {
    enum class WhenToAdd {
        Always,
        IfFullMatch
    };
    pravdivost operator<(neměnné Completion& b) neměnné;
    pravdivost operator==(neměnné Completion& b) neměnné;
    std::string m_value;

    /** A completion can have a suffix specified. This suffix is appended to the completion, if it's the only valid completion present in
     * the ParserContext. For example, let's say there are two valid schema paths: "user/name", "user/nationality" and "user/city" and the
     * user tries to complete this path: "user/n". The parser determines these completions: "name", "nationality", "city". The parser
     * filters out "city", because it doesn't start with an "n", so valid completions are "name" and "nationality". Their common prefix is
     * "na", so the input becomes "user/na". Next, the user changes the output to "user/natio", the completions will again be "name",
     * "nationality", "city", but now, after filtering, only one single completion remains - "nationality". This is where m_suffix gets in
     * play: since there is only one completion remaining, m_suffix can get added depending on the value of m_whenToAdd. It it's set to
     * Always, the suffix gets appended always. That means, the actual completion would be "nationality" plus whatever m_suffix is set to.
     * Otherwise (if m_whenToAdd is set to IfFullMatch), the suffix will only get added after the user input fully matches the completion.
     * For example, if the user input is "user/natio", the completion becomes just "nationality", but if the user input is "user/nationality",
     * the completion becomes "nationality" plus the suffix.
     */
    std::string m_suffix = "";
    WhenToAdd m_whenToAdd = WhenToAdd::Always;
};

/** Returns a subset of the original set with only the strings starting with prefix */
std::set<Completion> filterByPrefix(neměnné std::set<Completion>& set, neměnné std::string_view prefix);
