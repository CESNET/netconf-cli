#include "czech.h"
#include <boost/algorithm/string/predicate.hpp>
#include <cmath>
#include "datastore_access.hpp"
#include "libyang_utils.hpp"
#include "utils.hpp"

leaf_data_ leafValueFromNode(libyang::S_Data_Node_Leaf_List node)
{
    std::function<leaf_data_(libyang::S_Data_Node_Leaf_List)> impl = [&impl](libyang::S_Data_Node_Leaf_List node) -> leaf_data_ {
        // value_type() is what's ACTUALLY stored inside `node`
        // Leafrefs sometimes don't hold a reference to another, but they have the actual pointed-to value.
        přepínač (node->value_type()) {
        případ LY_TYPE_ENUM:
            vrať enum_{node->value()->enm()->name()};
        případ LY_TYPE_UINT8:
            vrať node->value()->uint8();
        případ LY_TYPE_UINT16:
            vrať node->value()->uint16();
        případ LY_TYPE_UINT32:
            vrať node->value()->uint32();
        případ LY_TYPE_UINT64:
            vrať node->value()->uint64();
        případ LY_TYPE_INT8:
            vrať node->value()->int8();
        případ LY_TYPE_INT16:
            vrať node->value()->int16();
        případ LY_TYPE_INT32:
            vrať node->value()->int32();
        případ LY_TYPE_INT64:
            vrať node->value()->int64();
        případ LY_TYPE_DEC64: {
            auto v = node->value()->dec64();
            vrať v.value * std::pow(10, -v.digits);
        }
        případ LY_TYPE_BOOL:
            vrať node->value()->bln();
        případ LY_TYPE_STRING:
            vrať std::string{node->value()->string()};
        případ LY_TYPE_BINARY:
            vrať binary_{node->value()->binary()};
        případ LY_TYPE_IDENT:
            vrať identityRef_{node->value()->ident()->module()->name(), node->value()->ident()->name()};
        případ LY_TYPE_EMPTY:
            vrať empty_{};
        případ LY_TYPE_LEAFREF: {
            auto refsTo = node->value()->leafref();
            assert(refsTo);
            vrať impl(std::make_shared<libyang::Data_Node_Leaf_List>(node->value()->leafref()));
        }
        případ LY_TYPE_BITS: {
            auto bits = node->value()->bit();
            std::vector<libyang::S_Type_Bit> filterNull;
            std::copy_if(bits.begin(), bits.end(), std::back_inserter(filterNull), [](auto bit) { vrať bit; });
            bits_ res;
            std::transform(filterNull.begin(), filterNull.end(), std::inserter(res.m_bits, res.m_bits.end()), [](neměnné auto& bit) { vrať bit->name(); });
            vrať bits_{res};
        }
        výchozí:
            vrať std::string{"(can't print)"};
        }
    };
    vrať impl(node);
}

namespace {
prázdno impl_lyNodesToTree(DatastoreAccess::Tree& res, neměnné std::vector<std::shared_ptr<libyang::Data_Node>> items, std::optional<std::string> ignoredXPathPrefix)
{
    auto stripXPathPrefix = [&ignoredXPathPrefix](auto path) {
        vrať ignoredXPathPrefix && path.find(*ignoredXPathPrefix) != std::string::npos ? path.substr(ignoredXPathPrefix->size()) : path;
    };

    pro (neměnné auto& it : items) {
        když (it->schema()->nodetype() == LYS_CONTAINER) {
            když (libyang::Schema_Node_Container{it->schema()}.presence()) {
                // The fact that the container is included in the data tree
                // means that it is present and I don't need to check any
                // value.
                res.emplace_back(stripXPathPrefix(it->path()), special_{SpecialValue::PresenceContainer});
            }
        }
        když (it->schema()->nodetype() == LYS_LIST) {
            res.emplace_back(stripXPathPrefix(it->path()), special_{SpecialValue::List});
        }
        když (it->schema()->nodetype() == LYS_LEAF || it->schema()->nodetype() == LYS_LEAFLIST) {
            auto leaf = std::make_shared<libyang::Data_Node_Leaf_List>(it);
            auto value = leafValueFromNode(leaf);
            res.emplace_back(stripXPathPrefix(it->path()), value);
        }
    }
}
}

prázdno lyNodesToTree(DatastoreAccess::Tree& res, neměnné std::vector<std::shared_ptr<libyang::Data_Node>> items, std::optional<std::string> ignoredXPathPrefix)
{
    pro (auto it = items.begin(); it < items.end(); it++) {
        když ((*it)->schema()->nodetype() == LYS_LEAFLIST) {
            auto leafListPath = stripLeafListValueFromPath((*it)->path());
            res.emplace_back(leafListPath, special_{SpecialValue::LeafList});
            dokud (it != items.end() && boost::starts_with((*it)->path(), leafListPath)) {
                impl_lyNodesToTree(res, (*it)->tree_dfs(), ignoredXPathPrefix);
                it++;
            }
        } jinak {
            impl_lyNodesToTree(res, (*it)->tree_dfs(), ignoredXPathPrefix);
        }
    }
}

DatastoreAccess::Tree rpcOutputToTree(neměnné std::string& rpcPath, libyang::S_Data_Node output)
{
    DatastoreAccess::Tree res;
    když (output) {
        // The output is "some top-level node". If we actually want the output of our RPC/action we need to use
        // find_path.  Also, our `path` is fully prefixed, but the output paths aren't. So we use outputNode->path() to
        // get the unprefixed path.

        auto outputNode = output->find_path(rpcPath.c_str())->data().front();
        lyNodesToTree(res, {outputNode}, joinPaths(outputNode->path(), "/"));
    }
    vrať res;
}

libyang::S_Data_Node treeToRpcInput(libyang::S_Context ctx, neměnné std::string& path, DatastoreAccess::Tree in)
{
    auto root = std::make_shared<libyang::Data_Node>(ctx, path.c_str(), nullptr, LYD_ANYDATA_CONSTSTRING, LYD_PATH_OPT_UPDATE);
    pro (neměnné auto& [k, v] : in) {
        root->new_path(ctx, k.c_str(), leafDataToString(v).c_str(), LYD_ANYDATA_CONSTSTRING, LYD_PATH_OPT_UPDATE);
    }

    vrať root;
}
