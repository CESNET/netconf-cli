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
void impl_lyNodesToTree(DatastoreAccess::Tree& res, const libyang::DfsCollection<libyang::DataNode>& items, std::optional<std::string> ignoredXPathPrefix)
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
                res.emplace_back(stripXPathPrefix(std::string{it.path()}), special_{SpecialValue::PresenceContainer});
            }
        }
        if (it.schema().nodeType() == libyang::NodeType::List) {
            res.emplace_back(stripXPathPrefix(std::string{it.path()}), special_{SpecialValue::List});
        }
        if (it.schema().nodeType() == libyang::NodeType::Leaf || it.schema().nodeType() == libyang::NodeType::Leaflist) {
            auto term = it.asTerm();
            auto value = leafValueFromNode(term);
            res.emplace_back(stripXPathPrefix(std::string{it.path()}), value);
        }
    }
}
}

void lyNodesToTree(DatastoreAccess::Tree& res, libyang::SiblingCollection items, std::optional<std::string> ignoredXPathPrefix)
{
    for (auto it = items.begin(); it != items.end(); /* nothing */) {
        if ((*it).schema().nodeType() == libyang::NodeType::Leaflist) {
            auto leafListPath = stripLeafListValueFromPath(std::string{(*it).path()});
            res.emplace_back(leafListPath, special_{SpecialValue::LeafList});
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

DatastoreAccess::Tree rpcOutputToTree(const std::string& rpcPath, libyang::DataNode output)
{
    DatastoreAccess::Tree res;
    // The output is "some top-level node". If we actually want the output of our RPC/action we need to use
    // find_path.  Also, our `path` is fully prefixed, but the output paths aren't. So we use outputNode->path() to
    // get the unprefixed path.

    auto outputNode = output.findPath(rpcPath.c_str());
    if (!outputNode) {
        // FIXME is this correct
        throw std::logic_error("rpcOutputToTree: output didn't have rpcPath");
    }

    lyNodesToTree(res, outputNode->siblings(), joinPaths(std::string{outputNode->path()}, "/"));
    return res;
}

libyang::DataNode treeToRpcInput(libyang::Context ctx, const std::string& path, DatastoreAccess::Tree in)
{
    auto root = ctx.newPath(path.c_str(), nullptr, libyang::CreationOptions::Update);
    for (const auto& [k, v] : in) {
        root.newPath(k.c_str(), leafDataToString(v).c_str(), libyang::CreationOptions::Output);
    }

    return root;
}
