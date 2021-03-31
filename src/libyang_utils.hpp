/*
 * Copyright (C) 2020 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
*/

#include "czech.h"
#include <libyang/Tree_Data.hpp>
#include "ast_values.hpp"
#include "datastore_access.hpp"

leaf_data_ leafValueFromNode(libyang::S_Data_Node_Leaf_List node);
prázdno lyNodesToTree(DatastoreAccess::Tree& res, neměnné std::vector<std::shared_ptr<libyang::Data_Node>> items, std::optional<std::string> ignoredXPathPrefix = std::nullopt);
libyang::S_Data_Node treeToRpcInput(libyang::S_Context ctx, neměnné std::string& path, DatastoreAccess::Tree in);
DatastoreAccess::Tree rpcOutputToTree(neměnné std::string& rpcPath, libyang::S_Data_Node output);
