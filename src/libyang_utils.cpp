#include <boost/algorithm/string/predicate.hpp>
#include <cmath>
#include <libyang-cpp/Context.hpp>
#include "datastore_access.hpp"
#include "libyang_utils.hpp"
#include "utils.hpp"

struct impl_leafValueFromNode {
    leaf_data_ operator()(const libyang::Empty) const
    {
        return empty_{};
    }

    leaf_data_ operator()(const libyang::Binary& bin) const
    {
        return binary_{bin.base64};
    }

    leaf_data_ operator()(const std::vector<libyang::Bit>& bits) const
    {
        bits_ res;
        std::transform(bits.begin(), bits.end(), std::back_inserter(res.m_bits), [] (const libyang::Bit& bit) {
            return bit.name;
        });
        return res;
    }

    leaf_data_ operator()(const libyang::Enum& enumVal) const
    {
        return enum_{enumVal.name};
    }

    leaf_data_ operator()(const libyang::IdentityRef& identRef) const
    {
        return identityRef_{identRef.module, identRef.name};
    }

    leaf_data_ operator()(const libyang::Decimal64& dec) const
    {
        return dec.number * std::pow(10, -dec.digits);
    }

    leaf_data_ operator()(const libyang::InstanceIdentifier& node) const
    {
        return instanceIdentifier_{node.path};
    }

    template <typename Type>
    leaf_data_ operator()(const Type& val) const
    {
        return val;
    }
};

leaf_data_ leafValueFromNode(libyang::DataNodeTerm node)
{
    return std::visit(impl_leafValueFromNode{},node.value());
}

namespace {
template <typename CollectionType>
void impl_lyNodesToTree(DatastoreAccess::Tree& res, CollectionType items, std::optional<std::string> ignoredXPathPrefix)
{
    auto stripXPathPrefix = [&ignoredXPathPrefix](auto path) {
        return ignoredXPathPrefix && path.find(*ignoredXPathPrefix) != std::string::npos ? path.substr(ignoredXPathPrefix->size()) : path;
    };

    for (const auto& it : items) {
        if (it.schema().nodeType() == libyang::NodeType::Container) {
            if (it.schema().asContainer().isPresence()) {
                // The fact that the container is included in the data tree
                // means that it is present and I don't need to check any
                // value.
                res.emplace_back(stripXPathPrefix(it.path()), special_{SpecialValue::PresenceContainer});
            }
        }
        if (it.schema().nodeType() == libyang::NodeType::List) {
            res.emplace_back(stripXPathPrefix(it.path()), special_{SpecialValue::List});
        }
        if (it.schema().nodeType() == libyang::NodeType::Leaf || it.schema().nodeType() == libyang::NodeType::Leaflist) {
            auto term = it.asTerm();
            auto value = leafValueFromNode(term);
            res.emplace_back(stripXPathPrefix(it.path()), value);
        }
    }
}
}

template <typename CollectionType>
void lyNodesToTree(DatastoreAccess::Tree& res, CollectionType items, std::optional<std::string> ignoredXPathPrefix)
{
    for (auto it = items.begin(); it != items.end(); /* nothing */) {
        if ((*it).schema().nodeType() == libyang::NodeType::Leaflist) {
            auto leafListPath = stripLeafListValueFromPath(it->path());
            res.emplace_back(leafListPath, special_{SpecialValue::LeafList});
            while (it != items.end() && boost::starts_with(it->path(), leafListPath)) {
                impl_lyNodesToTree(res, it->childrenDfs(), ignoredXPathPrefix);
                it++;
            }
        } else {
            impl_lyNodesToTree(res, it->childrenDfs(), ignoredXPathPrefix);
            it++;
        }
    }
}

using SiblingColl = libyang::Collection<libyang::DataNode, libyang::IterationType::Sibling>;
using DfsColl = libyang::Collection<libyang::DataNode, libyang::IterationType::Dfs>;

template
void lyNodesToTree<SiblingColl>(DatastoreAccess::Tree& res, SiblingColl items, std::optional<std::string> ignoredXPathPrefix);
template
void lyNodesToTree<DfsColl>(DatastoreAccess::Tree& res, DfsColl items, std::optional<std::string> ignoredXPathPrefix);
template
void lyNodesToTree<libyang::Set<libyang::DataNode>>(DatastoreAccess::Tree& res, libyang::Set<libyang::DataNode> items, std::optional<std::string> ignoredXPathPrefix);

DatastoreAccess::Tree rpcOutputToTree(libyang::DataNode output)
{
    DatastoreAccess::Tree res;
    lyNodesToTree(res, output.siblings(), joinPaths(output.path(), "/"));
    return res;
}

libyang::DataNode treeToRpcInput(libyang::Context ctx, const std::string& path, DatastoreAccess::Tree in)
{
    auto root = ctx.newPath(path, std::nullopt, libyang::CreationOptions::Update);
    for (const auto& [k, v] : in) {
        root.newPath(k, leafDataToString(v), libyang::CreationOptions::Update);
    }

    return root;
}
