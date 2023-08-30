/*
 * Copyright (C) 2020 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
*/

#pragma once
#include <memory>
#include <optional>
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
struct Empty {
    bool operator==(const Empty&) const;
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
struct Bits {
    bool operator==(const Bits& other) const;
    std::set<std::string> m_allowedValues;
};
struct LeafRef;
struct InstanceIdentifier {
    InstanceIdentifier();
    bool operator==(const InstanceIdentifier& other) const;
};
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
    yang::Empty,
    yang::IdentityRef,
    yang::Bits,
    yang::LeafRef,
    yang::InstanceIdentifier,
    yang::Union
>;
struct TypeInfo;
struct LeafRef {
    LeafRef(const LeafRef& src);
    LeafRef(const std::string& xpath, std::unique_ptr<TypeInfo>&& type);
    bool operator==(const LeafRef& other) const;
    std::string m_targetXPath;
    std::unique_ptr<TypeInfo> m_targetType;
};

struct Union {
    bool operator==(const Union& other) const;
    std::vector<TypeInfo> m_unionTypes;
};
struct TypeInfo {
    TypeInfo(const yang::LeafDataType& type,
            const std::optional<std::string> units = std::nullopt,
            const std::optional<std::string> description = std::nullopt);
    bool operator==(const TypeInfo& other) const;
    yang::LeafDataType m_type;
    std::optional<std::string> m_units;
    std::optional<std::string> m_description;
};
}
