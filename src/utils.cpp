/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/
#include <boost/algorithm/string/erase.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include "utils.hpp"

std::string joinPaths(const std::string& prefix, const std::string& suffix)
{
    if (prefix.empty() || suffix.empty() || prefix == "/")
        return prefix + suffix;
    else
        return prefix + '/' + suffix;
}

std::string stripLastNodeFromPath(const std::string& path)
{
    std::string res = path;
    auto pos = res.find_last_of('/');
    if (pos == res.npos)
        res.clear();
    else
        res.erase(pos);
    return res;
}

schemaPath_ pathWithoutLastNode(const schemaPath_& path)
{
    return schemaPath_{path.m_scope, decltype(schemaPath_::m_nodes)(path.m_nodes.begin(), path.m_nodes.end() - 1)};
}

std::string leafDataTypeToString(yang::LeafDataTypes type)
{
    switch (type) {
    case yang::LeafDataTypes::String:
        return "a string";
    case yang::LeafDataTypes::Decimal:
        return "a decimal";
    case yang::LeafDataTypes::Bool:
        return "a boolean";
    case yang::LeafDataTypes::Int:
        return "an integer";
    case yang::LeafDataTypes::Uint:
        return "an unsigned integer";
    case yang::LeafDataTypes::Enum:
        return "an enum";
    default:
        return "";
    }
}

std::string fullNodeName(const schemaPath_& location, const ModuleNodePair& pair)
{
    if (!pair.first) {
        return location.m_nodes.at(0).m_prefix.value().m_name + ":" + pair.second;
    } else {
        return pair.first.value() + ":" + pair.second;
    }
}

std::string fullNodeName(const dataPath_& location, const ModuleNodePair& pair)
{
    return fullNodeName(dataPathToSchemaPath(location), pair);
}

/** Returns a subset of the original set with only the strings starting with prefix
 * and with the actual prefix deleted from the string
 */
std::set<std::string> filterAndErasePrefix(const std::set<std::string>& set, const std::string_view prefix)
{
    std::set<std::string> filtered;
    std::copy_if(set.begin(), set.end(),
            std::inserter(filtered, filtered.end()),
            [prefix] (auto it) { return boost::starts_with(it, prefix); });

    std::set<std::string> withoutPrefix;
    std::transform(filtered.begin(), filtered.end(),
            std::inserter(withoutPrefix, withoutPrefix.end()),
            [prefix] (auto it) { boost::erase_first(it, prefix); return it; });
    return withoutPrefix;
}

