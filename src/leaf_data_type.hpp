/*
 * Copyright (C) 2020 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
*/

#pragma once
#include <memory>
#include <set>
#include <string>
#include <variant>
#include <vector>

struct enum_;
struct identityRef_;

namespace yang {
struct String {
    bool operator==(const String&) const;
};
struct Decimal {
    bool operator==(const Decimal&) const;
};
struct Bool {
    bool operator==(const Bool&) const;
};
struct Int8 {
    bool operator==(const Int8&) const;
};
struct Uint8 {
    bool operator==(const Uint8&) const;
};
struct Int16 {
    bool operator==(const Int16&) const;
};
struct Uint16 {
    bool operator==(const Uint16&) const;
};
struct Int32 {
    bool operator==(const Int32&) const;
};
struct Uint32 {
    bool operator==(const Uint32&) const;
};
struct Int64 {
    bool operator==(const Int64&) const;
};
struct Uint64 {
    bool operator==(const Uint64&) const;
};
struct Binary {
    bool operator==(const Binary&) const;
};
struct Enum {
    Enum(std::set<enum_>&& values);
    bool operator==(const Enum& other) const;
    std::set<enum_> m_allowedValues;
};
struct IdentityRef {
    IdentityRef(std::set<identityRef_>&& list);
    bool operator==(const IdentityRef& other) const;
    std::set<identityRef_> m_allowedValues;
};
struct LeafRef;
struct Union;
using LeafDataType = std::variant<
    yang::String,
    yang::Decimal,
    yang::Bool,
    yang::Int8,
    yang::Uint8,
    yang::Int16,
    yang::Uint16,
    yang::Int32,
    yang::Uint32,
    yang::Int64,
    yang::Uint64,
    yang::Enum,
    yang::Binary,
    yang::IdentityRef,
    yang::LeafRef,
    yang::Union
>;
struct LeafRef {
    LeafRef(const LeafRef& src);
    LeafRef(const std::string& xpath, std::unique_ptr<LeafDataType>&& type);
    bool operator==(const LeafRef& other) const;
    std::string m_targetXPath;
    std::unique_ptr<LeafDataType> m_targetType;
};

struct Union {
    bool operator==(const Union& other) const;
    std::vector<LeafDataType> m_unionTypes;
};
}
