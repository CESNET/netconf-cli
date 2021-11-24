/*
 * Copyright (C) 2020 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
*/

#include <libyang-cpp/DataNode.hpp>
#include "ast_values.hpp"
#include "datastore_access.hpp"

leaf_data_ leafValueFromNode(libyang::DataNodeTerm node);
template <typename CollectionType>
void lyNodesToTree(DatastoreAccess::Tree& res, CollectionType items, std::optional<std::string> ignoredXPathPrefix = std::nullopt);
libyang::DataNode treeToRpcInput(libyang::Context ctx, const std::string& path, DatastoreAccess::Tree in);
DatastoreAccess::Tree rpcOutputToTree(libyang::DataNode output);
