/*
 * Copyright (C) 2020 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
*/

#include "leaf_data.hpp"
template <>
std::set<std::string> createSetSuggestions_class<yang::LeafDataTypes::Enum>::getSuggestions(const ParserContext& parserContext, const Schema& schema) const
{
    return schema.enumValues(parserContext.m_tmpListKeyLeafPath.m_location, parserContext.m_tmpListKeyLeafPath.m_node);
}

template <>
std::set<std::string> createSetSuggestions_class<yang::LeafDataTypes::IdentityRef>::getSuggestions(const ParserContext& parserContext, const Schema& schema) const
{
    return schema.validIdentities(parserContext.m_tmpListKeyLeafPath.m_location, parserContext.m_tmpListKeyLeafPath.m_node, Prefixes::WhenNeeded);
}
