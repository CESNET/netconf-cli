/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/

#pragma once

#include <boost/variant.hpp>
#include <set>
#include <stdexcept>
#include <unordered_map>
#include "ast_path.hpp"

namespace yang {
enum class ContainerTraits {
    Presence,
    None,
};

enum class LeafDataTypes {
    String,
    Decimal,
    Bool,
    Int,
    Uint,
    Enum,
};

struct container {
    yang::ContainerTraits m_presence;
};
struct list {
    std::set<std::string> m_keys;
};
struct leaf {
    yang::LeafDataTypes m_type;
    std::set<std::string> m_enumValues;
};
}


using NodeType = boost::variant<yang::container, yang::list, yang::leaf>;


class InvalidNodeException : public std::invalid_argument {
public:
    using std::invalid_argument::invalid_argument;
    ~InvalidNodeException() override;
};

/*! \class Schema
 *     \brief The class representing the schema, that the user traverses.
 *         */
class Schema {
public:
    Schema();
    bool nodeExists(const std::string& location, const std::string& name) const;

    bool isContainer(const path_& location, const std::string& name) const;
    void addContainer(const std::string& location, const std::string& name, yang::ContainerTraits isPresence = yang::ContainerTraits::None);
    const std::set<std::string>& listKeys(const path_& location, const std::string& name) const;
    bool listHasKey(const path_& location, const std::string& name, const std::string& key) const;
    bool isList(const path_& location, const std::string& name) const;
    void addList(const std::string& location, const std::string& name, const std::set<std::string>& keys);
    bool isPresenceContainer(const path_& location, const std::string& name) const;
    void addLeaf(const std::string& location, const std::string& name, const yang::LeafDataTypes& type);
    void addLeafEnum(const std::string& location, const std::string& name, std::set<std::string> enumValues);
    bool leafEnumHasValue(const path_& location, const std::string& name, const std::string& value) const;
    bool isLeaf(const path_& location, const std::string& name) const;
    bool leafIsEnum(const path_& location, const std::string& name) const;
    bool leafIsDecimal(const path_& location, const std::string& name) const;
    bool leafIsBool(const path_& location, const std::string& name) const;
    bool leafIsInt(const path_& location, const std::string& name) const;
    bool leafIsUint(const path_& location, const std::string& name) const;
    bool leafIsString(const path_& location, const std::string& name) const;


private:
    const std::unordered_map<std::string, NodeType>& children(const std::string& name) const;

    std::unordered_map<std::string, std::unordered_map<std::string, NodeType>> m_nodes;
};
