/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/
/*! \file utils.hpp
    \brief A header containing utility functions
*/
#include <string>
#include "ast_path.hpp"
#include "ast_values.hpp"
#include "schema.hpp"

std::string joinPaths(const std::string& prefix, const std::string& suffix);
std::string stripLastNodeFromPath(const std::string& path);
std::string lastNodeOfPath(const std::string& path);
schemaPath_ pathWithoutLastNode(const schemaPath_& path);
dataPath_ pathWithoutLastNode(const dataPath_& path);
std::string leafDataTypeToString(yang::LeafDataTypes type);
std::string fullNodeName(const schemaPath_& location, const ModuleNodePair& pair);
std::string fullNodeName(const dataPath_& location, const ModuleNodePair& pair);
/** Returns a subset of the original set with only the strings starting with prefix */
std::set<std::string> filterByPrefix(const std::set<std::string>& set, const std::string_view prefix);
std::string leafDataToString(const leaf_data_ value);
