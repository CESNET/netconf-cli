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

namespace {
template <typename OutputType, typename CollectionType>
void impl_lyNodesToTree(OutputType& res, CollectionType items, std::optional<std::string> ignoredXPathPrefix)
{
    auto stripXPathPrefix = [&ignoredXPathPrefix](auto path) {
        return ignoredXPathPrefix && path.find(*ignoredXPathPrefix) != std::string::npos ? path.substr(ignoredXPathPrefix->size()) : path;
    };

    auto emplaceItem = [&] (auto item, auto path, auto value) {
        if constexpr (std::is_same<OutputType, DatastoreAccess::Tree>()) {
            res.emplace_back(stripXPathPrefix(path), value);
        } else {
            auto meta = item.meta();
            auto op = DatastoreAccess::Operation::Merge;

            if (std::find_if(meta.begin(), meta.end(),
                [] (const libyang::Meta& meta) { return meta.name() == "operation" && (meta.valueStr() == "remove" || meta.valueStr() == "delete"); }) != meta.end()) {
                op = DatastoreAccess::Operation::Remove;
            }
            res.emplace_back(DatastoreAccess::Change{
                    .xpath = path,
                    .value = value,
                    .operation = op
            });
        }
    };

    for (const auto& it : items) {
        if (it.schema().nodeType() == libyang::NodeType::Container) {
            if (it.schema().asContainer().isPresence()) {
                // The fact that the container is included in the data tree
                // means that it is present and I don't need to check any
                // value.
                emplaceItem(it, it.path(), special_{SpecialValue::PresenceContainer});
            }
        }
        if (it.schema().nodeType() == libyang::NodeType::List) {
            emplaceItem(it, it.path(), special_{SpecialValue::List});
        }
        if (it.schema().nodeType() == libyang::NodeType::Leaf || it.schema().nodeType() == libyang::NodeType::Leaflist) {
            auto term = it.asTerm();
            auto value = leafValueFromNode(term);
            emplaceItem(it, it.path(), value);
        }
    }
}
}

template <typename OutputType, typename CollectionType>
void lyNodesToTree(OutputType& res, CollectionType items, std::optional<std::string> ignoredXPathPrefix)
{
    for (auto it = items.begin(); it != items.end(); /* nothing */) {
        if ((*it).schema().nodeType() == libyang::NodeType::Leaflist) {
            auto leafListPath = stripLeafListValueFromPath(std::string{(*it).path()});
            if constexpr (std::is_same<OutputType, DatastoreAccess::Tree>()) {
                res.emplace_back(leafListPath, special_{SpecialValue::LeafList});
            } else {
                res.emplace_back(DatastoreAccess::Change{
                    .xpath = leafListPath,
                    .value = special_{SpecialValue::LeafList},
                    .operation = DatastoreAccess::Operation::Merge
                });
            }
            while (it != items.end() && boost::starts_with(std::string{(*it).path()}, leafListPath)) {
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
void lyNodesToTree<DatastoreAccess::Tree, SiblingColl>(DatastoreAccess::Tree& res, SiblingColl items, std::optional<std::string> ignoredXPathPrefix);
template
void lyNodesToTree<DatastoreAccess::ChangeTree, SiblingColl>(DatastoreAccess::ChangeTree& res, SiblingColl items, std::optional<std::string> ignoredXPathPrefix);
template
void lyNodesToTree<DatastoreAccess::Tree, DfsColl>(DatastoreAccess::Tree& res, DfsColl items, std::optional<std::string> ignoredXPathPrefix);
template
void lyNodesToTree<DatastoreAccess::Tree, libyang::Set<libyang::DataNode>>(DatastoreAccess::Tree& res, libyang::Set<libyang::DataNode> items, std::optional<std::string> ignoredXPathPrefix);

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
