/*
 * Copyright (C) 2020 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
*/

#include "leaf_data.hpp"
template <>
std::set<std::string> createSetSuggestions_class<yang::Enum>::getSuggestions(ParserContext& parserContext) const
{
    auto enums = std::get<yang::Enum>(parserContext.m_leafType).m_allowedValues;
    std::set<std::string> res;
    std::transform(enums.begin(), enums.end(), std::inserter(res, res.end()), [](const auto& value) { return value.m_value; });
    return res;
}

template <>
std::set<std::string> createSetSuggestions_class<yang::IdentityRef>::getSuggestions(ParserContext& parserContext) const
{
    auto identities = std::get<yang::IdentityRef>(parserContext.m_leafType).m_allowedValues;
    std::set<std::string> res;
    auto topLevelModule = parserContext.currentSchemaPath().m_nodes.front().m_prefix.get().m_name;

    std::transform(identities.begin(), identities.end(), std::inserter(res, res.end()), [topLevelModule](const auto& identity) {
        std::string res;
        if (topLevelModule != identity.m_prefix.get().m_name) {
            res += identity.m_prefix.get().m_name + ":";
        }
        res += identity.m_value;
        return res;
    });
    return res;
}
