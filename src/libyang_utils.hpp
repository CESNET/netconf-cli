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
void lyNodesToTree(DatastoreAccess::Tree& res, libyang::Collection<libyang::DataNode, libyang::IterationType::Sibling> items, std::optional<std::string> ignoredXPathPrefix = std::nullopt);
void lyNodesToTree(DatastoreAccess::Tree& res, libyang::DataNodeSet items, std::optional<std::string> ignoredXPathPrefix = std::nullopt);
libyang::DataNode treeToRpcInput(libyang::Context ctx, const std::string& path, DatastoreAccess::Tree in);
DatastoreAccess::Tree rpcOutputToTree(const std::string& rpcPath, libyang::DataNode output);
