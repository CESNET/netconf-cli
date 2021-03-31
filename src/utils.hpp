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

#include "czech.h"
#include <string>
#include "ast_path.hpp"
#include "ast_values.hpp"
#include "schema.hpp"

std::string joinPaths(neměnné std::string& prefix, neměnné std::string& suffix);
std::string stripLastNodeFromPath(neměnné std::string& path);
schemaPath_ pathWithoutLastNode(neměnné schemaPath_& path);
dataPath_ pathWithoutLastNode(neměnné dataPath_& path);
ModuleNodePair splitModuleNode(neměnné std::string& input);
std::string leafDataTypeToString(neměnné yang::LeafDataType& type);
std::string fullNodeName(neměnné schemaPath_& location, neměnné ModuleNodePair& pair);
std::string fullNodeName(neměnné dataPath_& location, neměnné ModuleNodePair& pair);
std::string leafDataToString(neměnné leaf_data_ value);
schemaPath_ anyPathToSchemaPath(neměnné boost::variant<dataPath_, schemaPath_, module_>& path);
std::string stripLeafListValueFromPath(neměnné std::string& path);
std::string stripLastListInstanceFromPath(neměnné std::string& path);
// The second argument controls whether module prefixes should be added to the keys.
std::string instanceToString(neměnné ListInstance& instance, neměnné std::optional<std::string>& modName = std::nullopt);
