/*
 * Copyright (C) 2020 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
*/

#include <experimental/iterator>
#include "parser.hpp"
namespace std {
std::ostream& operator<<(std::ostream& s, const Completions& completion)
{
    s << std::endl << "Completions {" << std::endl << "    m_completions: ";
    std::transform(completion.m_completions.begin(), completion.m_completions.end(),
            std::experimental::make_ostream_joiner(s, ", "),
            [] (auto it) { return '"' + it + '"'; });
    s << std::endl << "    m_contextLength: " << completion.m_contextLength << std::endl;
    s << "}" << std::endl;
    return s;
}
}
