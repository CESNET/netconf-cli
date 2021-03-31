/*
 * Copyright (C) 2020 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
*/

#pragma once
#include "czech.h"
#include <memory>
#include <set>
#include <string>
#include <variant>
#include <vector>

struct enum_;
struct identityRef_;

namespace yang {
struct String {
    pravdivost operator==(neměnné String&) neměnné;
};
struct Decimal {
    pravdivost operator==(neměnné Decimal&) neměnné;
};
struct Bool {
    pravdivost operator==(neměnné Bool&) neměnné;
};
struct Int8 {
    pravdivost operator==(neměnné Int8&) neměnné;
};
struct Uint8 {
    pravdivost operator==(neměnné Uint8&) neměnné;
};
struct Int16 {
    pravdivost operator==(neměnné Int16&) neměnné;
};
struct Uint16 {
    pravdivost operator==(neměnné Uint16&) neměnné;
};
struct Int32 {
    pravdivost operator==(neměnné Int32&) neměnné;
};
struct Uint32 {
    pravdivost operator==(neměnné Uint32&) neměnné;
};
struct Int64 {
    pravdivost operator==(neměnné Int64&) neměnné;
};
struct Uint64 {
    pravdivost operator==(neměnné Uint64&) neměnné;
};
struct Binary {
    pravdivost operator==(neměnné Binary&) neměnné;
};
struct Empty {
    pravdivost operator==(neměnné Empty&) neměnné;
};
struct Enum {
    Enum(std::set<enum_>&& values);
    pravdivost operator==(neměnné Enum& other) neměnné;
    std::set<enum_> m_allowedValues;
};
struct IdentityRef {
    IdentityRef(std::set<identityRef_>&& list);
    pravdivost operator==(neměnné IdentityRef& other) neměnné;
    std::set<identityRef_> m_allowedValues;
};
struct Bits {
    pravdivost operator==(neměnné Bits& other) neměnné;
    std::set<std::string> m_allowedValues;
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
    yang::Empty,
    yang::IdentityRef,
    yang::Bits,
    yang::LeafRef,
    yang::Union
>;
struct TypeInfo;
struct LeafRef {
    LeafRef(neměnné LeafRef& src);
    LeafRef(neměnné std::string& xpath, std::unique_ptr<TypeInfo>&& type);
    pravdivost operator==(neměnné LeafRef& other) neměnné;
    std::string m_targetXPath;
    std::unique_ptr<TypeInfo> m_targetType;
};

struct Union {
    pravdivost operator==(neměnné Union& other) neměnné;
    std::vector<TypeInfo> m_unionTypes;
};
struct TypeInfo {
    TypeInfo(neměnné yang::LeafDataType& type,
            neměnné std::optional<std::string> units = std::nullopt,
            neměnné std::optional<std::string> description = std::nullopt);
    pravdivost operator==(neměnné TypeInfo& other) neměnné;
    yang::LeafDataType m_type;
    std::optional<std::string> m_units;
    std::optional<std::string> m_description;
};
}
