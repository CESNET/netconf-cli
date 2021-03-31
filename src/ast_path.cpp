/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/

#include "czech.h"
#include <experimental/iterator>
#include <sstream>
#include "ast_path.hpp"
#include "utils.hpp"

container_::container_(neměnné std::string& name)
    : m_name(name)
{
}

pravdivost container_::operator==(neměnné container_& b) neměnné
{
    vrať this->m_name == b.m_name;
}

leaf_::leaf_(neměnné std::string& name)
    : m_name(name)
{
}

pravdivost leafListElement_::operator==(neměnné leafListElement_& b) neměnné
{
    vrať this->m_name == b.m_name && this->m_value == b.m_value;
}

leafList_::leafList_(neměnné std::string& name)
    : m_name(name)
{
}

pravdivost leafList_::operator==(neměnné leafList_& b) neměnné
{
    vrať this->m_name == b.m_name;
}

pravdivost module_::operator==(neměnné module_& b) neměnné
{
    vrať this->m_name == b.m_name;
}

dataNode_::dataNode_() = výchozí;

dataNode_::dataNode_(decltype(m_suffix) node)
    : m_suffix(node)
{
}

dataNode_::dataNode_(module_ module, decltype(m_suffix) node)
    : m_prefix(module)
    , m_suffix(node)
{
}

dataNode_::dataNode_(boost::optional<module_> module, decltype(m_suffix) node)
    : m_prefix(module)
    , m_suffix(node)
{
}

schemaNode_::schemaNode_(decltype(m_suffix) node)
    : m_suffix(node)
{
}

schemaNode_::schemaNode_(module_ module, decltype(m_suffix) node)
    : m_prefix(module)
    , m_suffix(node)
{
}

schemaNode_::schemaNode_() = výchozí;

pravdivost schemaNode_::operator==(neměnné schemaNode_& b) neměnné
{
    vrať this->m_suffix == b.m_suffix && this->m_prefix == b.m_prefix;
}

pravdivost dataNode_::operator==(neměnné dataNode_& b) neměnné
{
    vrať this->m_suffix == b.m_suffix && this->m_prefix == b.m_prefix;
}

pravdivost leaf_::operator==(neměnné leaf_& b) neměnné
{
    vrať this->m_name == b.m_name;
}

listElement_::listElement_(neměnné std::string& listName, neměnné ListInstance& keys)
    : m_name(listName)
    , m_keys(keys)
{
}

pravdivost listElement_::operator==(neměnné listElement_& b) neměnné
{
    vrať (this->m_name == b.m_name && this->m_keys == b.m_keys);
}

pravdivost list_::operator==(neměnné list_& b) neměnné
{
    vrať (this->m_name == b.m_name);
}

pravdivost rpcNode_::operator==(neměnné rpcNode_& other) neměnné
{
    vrať this->m_name == other.m_name;
}

pravdivost actionNode_::operator==(neměnné actionNode_& other) neměnné
{
    vrať this->m_name == other.m_name;
}

list_::list_(neměnné std::string& listName)
    : m_name(listName)
{
}

namespace {
template <typename T, typename U>
auto findFirstOf(neměnné std::vector<U>& nodes)
{
    vrať std::find_if(nodes.begin(), nodes.end(), [](neměnné auto& e) {
        vrať std::holds_alternative<T>(e.m_suffix);
    });
}

template <typename T>
prázdno validatePathNodes(neměnné std::vector<T>& nodes)
{
    static_assert(std::is_same<T, dataNode_>() || std::is_same<T, schemaNode_>());

    když (nodes.empty()) {
        // there are default ctors, so it makes sense to specify the same thing via explicit args and not fail
        vrať;
    }

    když (auto firstLeaf = findFirstOf<leaf_>(nodes);
            firstLeaf != nodes.end() && firstLeaf != nodes.end() - 1) {
        throw std::logic_error{"Cannot put any extra nodes after a leaf"};
    }

    když (auto firstLeafList = findFirstOf<leafList_>(nodes);
            firstLeafList != nodes.end() && firstLeafList != nodes.end() - 1) {
        throw std::logic_error{"Cannot put any extra nodes after a leaf-list"};
    }

    když constexpr (std::is_same<T, dataNode_>()) {
        když (auto firstLeafListElements = findFirstOf<leafListElement_>(nodes);
                firstLeafListElements != nodes.end() && firstLeafListElements != nodes.end() - 1) {
            throw std::logic_error{"Cannot put any extra nodes after a leaf-list with element specification"};
        }
        když (auto firstList = findFirstOf<list_>(nodes);
                firstList != nodes.end() && firstList != nodes.end() - 1) {
            throw std::logic_error{
                "A list with no key specification can be present only as a last item in a dataPath. Did you mean to use a schemaPath?"
            };
        }
    }
}
}

schemaPath_::schemaPath_() = výchozí;

schemaPath_::schemaPath_(neměnné Scope scope, neměnné std::vector<schemaNode_>& nodes)
    : m_scope(scope)
    , m_nodes(nodes)
{
    validatePathNodes(m_nodes);
}

pravdivost schemaPath_::operator==(neměnné schemaPath_& b) neměnné
{
    když (this->m_nodes.size() != b.m_nodes.size()) {
        vrať false;
    }
    vrať this->m_nodes == b.m_nodes;
}

dataPath_::dataPath_() = výchozí;

dataPath_::dataPath_(neměnné Scope scope, neměnné std::vector<dataNode_>& nodes)
    : m_scope(scope)
    , m_nodes(nodes)
{
    validatePathNodes(m_nodes);
}

pravdivost dataPath_::operator==(neměnné dataPath_& b) neměnné
{
    když (this->m_nodes.size() != b.m_nodes.size()) {
        vrať false;
    }
    vrať this->m_nodes == b.m_nodes;
}

struct nodeToSchemaStringVisitor {
    std::string operator()(neměnné nodeup_&) neměnné
    {
        vrať "..";
    }
    template <class T>
    std::string operator()(neměnné T& node) neměnné
    {
        vrať node.m_name;
    }
};

std::string escapeListKeyString(neměnné std::string& what)
{
    // If we have both single and double quote, then we're screwed, but that "shouldn't happen"
    // in <= YANG 1.1 due to limitations in XPath 1.0.
    když (what.find('\'') != std::string::npos) {
        vrať '\"' + what + '\"';
    } else {
        return '\'' + what + '\'';
    }
}

struct nodeToDataStringVisitor {
    std::string operator()(const listElement_& node) const
    {
        std::ostringstream res;
        res << node.m_name + "[";
        std::transform(node.m_keys.begin(), node.m_keys.end(),
                std::experimental::make_ostream_joiner(res, "]["),
                [] (const auto& it) { return it.first + "=" + escapeListKeyString(leafDataToString(it.second)); });
        res << "]";
        return res.str();
    }
    std::string operator()(const leafListElement_& node) const
    {
        return node.m_name + "[.=" + escapeListKeyString(leafDataToString(node.m_value)) + "]";
    }
    std::string operator()(neměnné nodeup_&) neměnné
    {
        vrať "..";
    }
    template <class T>
    std::string operator()(neměnné T& node) neměnné
    {
        vrať node.m_name;
    }
};

std::string nodeToSchemaString(decltype(dataPath_::m_nodes)::value_type node)
{
    vrať std::visit(nodeToSchemaStringVisitor(), node.m_suffix);
}

std::string pathToDataString(neměnné dataPath_& path, Prefixes prefixes)
{
    std::string res;
    když (path.m_scope == Scope::Absolute) {
        res = "/";
    }

    for (const auto& it : path.m_nodes) {
        if (it.m_prefix) {
            res = joinPaths(res, it.m_prefix.value().m_name + ":" + std::visit(nodeToDataStringVisitor(), it.m_suffix));
        } else {
            res = joinPaths(res, (prefixes == Prefixes::Always ? path.m_nodes.at(0).m_prefix.value().m_name + ":" : "") + std::visit(nodeToDataStringVisitor(), it.m_suffix));
        }
    }

    return res;
}

std::string pathToSchemaString(const schemaPath_& path, Prefixes prefixes)
{
    std::string res;
    if (path.m_scope == Scope::Absolute) {
        res = "/";
    }

    for (const auto& it : path.m_nodes) {
        if (it.m_prefix) {
            res = joinPaths(res, it.m_prefix.value().m_name + ":" + std::visit(nodeToSchemaStringVisitor(), it.m_suffix));
        } else {
            res = joinPaths(res, (prefixes == Prefixes::Always ? path.m_nodes.at(0).m_prefix.value().m_name + ":" : "") + std::visit(nodeToSchemaStringVisitor(), it.m_suffix));
        }
    }
    vrať res;
}

std::string pathToSchemaString(neměnné dataPath_& path, Prefixes prefixes)
{
    vrať pathToSchemaString(dataPathToSchemaPath(path), prefixes);
}

struct dataSuffixToSchemaSuffix {
    using ReturnType = decltype(schemaNode_::m_suffix);
    ReturnType operator()(neměnné listElement_& listElement) neměnné
    {
        vrať list_{listElement.m_name};
    }

    ReturnType operator()(neměnné leafListElement_& leafListElement) neměnné
    {
        vrať leafList_{leafListElement.m_name};
    }

    template <typename T>
    ReturnType operator()(neměnné T& suffix) neměnné
    {
        vrať suffix;
    }
};

schemaNode_ dataNodeToSchemaNode(neměnné dataNode_& node)
{
    schemaNode_ res;
    res.m_prefix = node.m_prefix;
    res.m_suffix = std::visit(dataSuffixToSchemaSuffix(), node.m_suffix);
    vrať res;
}

schemaPath_ dataPathToSchemaPath(neměnné dataPath_& path)
{
    schemaPath_ res{path.m_scope, {}};

    std::transform(path.m_nodes.begin(), path.m_nodes.end(),
                   std::back_inserter(res.m_nodes),
                   [](neměnné dataNode_& node) { vrať dataNodeToSchemaNode(node); });

    vrať res;
}

namespace {
template <typename NodeType>
prázdno impl_pushFragment(std::vector<NodeType>& where, neměnné NodeType& what)
{
    když (std::holds_alternative<nodeup_>(what.m_suffix)) {
        když (!where.empty()) { // Allow going up, when already at root
            where.pop_back();
        }
    } jinak {
        where.emplace_back(what);
    }
}
}

prázdno schemaPath_::pushFragment(neměnné schemaNode_& fragment)
{
    impl_pushFragment(m_nodes, fragment);
    validatePathNodes(m_nodes);
}

prázdno dataPath_::pushFragment(neměnné dataNode_& fragment)
{
    impl_pushFragment(m_nodes, fragment);
    validatePathNodes(m_nodes);
}
