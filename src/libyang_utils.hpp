/*
 * Copyright (C) 2020 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
*/

#include <libyang/Tree_Data.hpp>
#include "ast_values.hpp"
#include "datastore_access.hpp"

leaf_data_ leafValueFromNode(libyang::S_Data_Node_Leaf_List node);
void lyNodesToTree(DatastoreAccess::Tree& res, const std::vector<std::shared_ptr<libyang::Data_Node>> items, std::optional<std::string> ignoredXPathPrefix = std::nullopt);
