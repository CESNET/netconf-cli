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
        return binary_{std::string{bin.base64}};
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

    leaf_data_ operator()(const std::optional<libyang::DataNode>&) const
    {
        throw std::runtime_error("instance-identifier is not supported");
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

template <typename CollectionType>
void lyNodesToTree(DatastoreAccess::Tree& res, CollectionType items, std::optional<std::string> ignoredXPathPrefix)
{
    auto stripXPathPrefix = [&ignoredXPathPrefix](auto path) {
        return ignoredXPathPrefix && path.find(*ignoredXPathPrefix) != std::string::npos ? path.substr(ignoredXPathPrefix->size()) : path;
    };

    auto itemsIt = items.begin();
    if (itemsIt == items.end()) {
        // No items, just return.
        return;
    }
    auto dfsColl = itemsIt->childrenDfs();
    auto dfsIt = dfsColl.begin();
    auto advanceIt = [&] {
        dfsIt++;
        if (dfsIt == dfsColl.end()) {
            itemsIt++;
            if (itemsIt != items.end()) {
                dfsColl = itemsIt->childrenDfs();
                dfsIt = dfsColl.begin();
            }
        }
    };

    while (itemsIt != items.end()) {
        std::cerr << "dfsIt->path() = " << dfsIt->path() << "\n";
        if (dfsIt->schema().nodeType() == libyang::NodeType::Leaflist) {
            auto leafListPath = stripLeafListValueFromPath(dfsIt->path());
            res.emplace_back(leafListPath, special_{SpecialValue::LeafList});
            while (itemsIt != items.end() && boost::starts_with(dfsIt->path(), leafListPath)) {
                auto term = dfsIt->asTerm();
                auto value = leafValueFromNode(term);
                res.emplace_back(stripXPathPrefix(term.path()), value);
                advanceIt();
            }

            continue;
        }

        if (dfsIt->schema().nodeType() == libyang::NodeType::Container) {
            if (dfsIt->schema().asContainer().isPresence()) {
                // The fact that the container is included in the data tree
                // means that it is present and I don't need to check any
                // value.
                res.emplace_back(stripXPathPrefix(dfsIt->path()), special_{SpecialValue::PresenceContainer});
            }
        }
        if (dfsIt->schema().nodeType() == libyang::NodeType::List) {
            res.emplace_back(stripXPathPrefix(dfsIt->path()), special_{SpecialValue::List});
        }
        if (dfsIt->schema().nodeType() == libyang::NodeType::Leaf) {
            auto term = dfsIt->asTerm();
            auto value = leafValueFromNode(term);
            res.emplace_back(stripXPathPrefix(dfsIt->path()), value);
        }
        advanceIt();
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
    lyNodesToTree(res, output.siblings(), joinPaths(std::string{output.path()}, "/"));
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
