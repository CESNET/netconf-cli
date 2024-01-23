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
#pragma once

#include <string>
#include "ast_path.hpp"
#include "ast_values.hpp"
#include "schema.hpp"

std::string joinPaths(const std::string& prefix, const std::string& suffix);
std::string stripLastNodeFromPath(const std::string& path);
schemaPath_ pathWithoutLastNode(const schemaPath_& path);
dataPath_ pathWithoutLastNode(const dataPath_& path);
ModuleNodePair splitModuleNode(const std::string& input);
std::string leafDataTypeToString(const yang::LeafDataType& type);
std::string fullNodeName(const schemaPath_& location, const ModuleNodePair& pair);
std::string fullNodeName(const dataPath_& location, const ModuleNodePair& pair);
std::string leafDataToString(const leaf_data_ value);
schemaPath_ anyPathToSchemaPath(const boost::variant<dataPath_, schemaPath_, module_>& path);
std::string stripLeafListValueFromPath(const std::string& path);
std::string stripLastListInstanceFromPath(const std::string& path);
// The second argument controls whether module prefixes should be added to the keys.
std::string instanceToString(const ListInstance& instance, const std::optional<std::string>& modName = std::nullopt);

template <typename... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template <typename... Ts> overloaded(Ts...) -> overloaded<Ts...>;
