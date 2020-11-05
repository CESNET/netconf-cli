#include <boost/algorithm/string/predicate.hpp>
#include <cmath>
#include "datastore_access.hpp"
#include "libyang_utils.hpp"
#include "utils.hpp"

leaf_data_ leafValueFromNode(libyang::S_Data_Node_Leaf_List node)
{
    std::function<leaf_data_(libyang::S_Data_Node_Leaf_List)> impl = [&impl] (libyang::S_Data_Node_Leaf_List node) -> leaf_data_ {
        // value_type() is what's ACTUALLY stored inside `node`
        // Leafrefs sometimes don't hold a reference to another, but they have the actual pointed-to value.
        switch (node->value_type()) {
        case LY_TYPE_ENUM:
            return enum_{node->value()->enm()->name()};
        case LY_TYPE_UINT8:
            return node->value()->uint8();
        case LY_TYPE_UINT16:
            return node->value()->uint16();
        case LY_TYPE_UINT32:
            return node->value()->uint32();
        case LY_TYPE_UINT64:
            return node->value()->uint64();
        case LY_TYPE_INT8:
            return node->value()->int8();
        case LY_TYPE_INT16:
            return node->value()->int16();
        case LY_TYPE_INT32:
            return node->value()->int32();
        case LY_TYPE_INT64:
            return node->value()->int64();
        case LY_TYPE_DEC64:
        {
            auto v = node->value()->dec64();
            return v.value * std::pow(10, -v.digits);
        }
        case LY_TYPE_BOOL:
            return node->value()->bln();
        case LY_TYPE_STRING:
            return std::string{node->value()->string()};
        case LY_TYPE_BINARY:
            return binary_{node->value()->binary()};
        case LY_TYPE_IDENT:
            return identityRef_{node->value()->ident()->module()->name(), node->value()->ident()->name()};
        case LY_TYPE_EMPTY:
            return empty_{};
        case LY_TYPE_LEAFREF:
        {
            auto refsTo = node->value()->leafref();
            assert(refsTo);
            return impl(std::make_shared<libyang::Data_Node_Leaf_List>(node->value()->leafref()));
        }
        case LY_TYPE_BITS:
        {
            auto bits = node->value()->bit();
            std::vector<libyang::S_Type_Bit> filterNull;
            std::copy_if(bits.begin(), bits.end(), std::back_inserter(filterNull), [] (auto bit) { return bit; });
            bits_ res;
            std::transform(filterNull.begin(), filterNull.end(), std::inserter(res.m_bits, res.m_bits.end()), [] (const auto& bit) { return bit->name(); });
            return bits_{res};
        }
        default:
            return std::string{"(can't print)"};
    }
    };
    return impl(node);
}

namespace {
void impl_lyNodesToTree(DatastoreAccess::Tree& res, const std::vector<std::shared_ptr<libyang::Data_Node>> items, std::optional<std::string> ignoredXPathPrefix)
{
    auto stripXPathPrefix = [&ignoredXPathPrefix] (auto path) {
        return ignoredXPathPrefix && path.find(*ignoredXPathPrefix) != std::string::npos ? path.substr(ignoredXPathPrefix->size()) : path;
    };

    for (const auto& it : items) {
        if (it->schema()->nodetype() == LYS_CONTAINER) {
            if (libyang::Schema_Node_Container{it->schema()}.presence()) {
                // The fact that the container is included in the data tree
                // means that it is present and I don't need to check any
                // value.
                res.emplace_back(stripXPathPrefix(it->path()), special_{SpecialValue::PresenceContainer});
            }
        }
        if (it->schema()->nodetype() == LYS_LIST) {
            res.emplace_back(stripXPathPrefix(it->path()), special_{SpecialValue::List});
        }
        if (it->schema()->nodetype() == LYS_LEAF || it->schema()->nodetype() == LYS_LEAFLIST) {
            auto leaf = std::make_shared<libyang::Data_Node_Leaf_List>(it);
            auto value = leafValueFromNode(leaf);
            res.emplace_back(stripXPathPrefix(it->path()), value);
        }
    }
}
}

// This is very similar to the fillMap lambda in SysrepoAccess, however,
// Sysrepo returns a weird array-like structure, while libnetconf
// returns libyang::Data_Node
void lyNodesToTree(DatastoreAccess::Tree& res, const std::vector<std::shared_ptr<libyang::Data_Node>> items, std::optional<std::string> ignoredXPathPrefix)
{
    for (auto it = items.begin(); it < items.end(); it++) {
        if ((*it)->schema()->nodetype() == LYS_LEAFLIST) {
            auto leafListPath = stripLeafListValueFromPath((*it)->path());
            res.emplace_back(leafListPath, special_{SpecialValue::LeafList});
            while (it != items.end() && boost::starts_with((*it)->path(), leafListPath)) {
                impl_lyNodesToTree(res, (*it)->tree_dfs(), ignoredXPathPrefix);
                it++;
            }
        } else {
            impl_lyNodesToTree(res, (*it)->tree_dfs(), ignoredXPathPrefix);
        }
    }
}
