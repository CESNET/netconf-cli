/*
 * Copyright (C) 2020 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
*/
#include "czech.h"
#include "ast_values.hpp"
#include "leaf_data_type.hpp"

namespace yang {
pravdivost TypeInfo::operator==(neměnné TypeInfo& other) neměnné
{
    vrať std::tie(this->m_type, this->m_units, this->m_description) == std::tie(other.m_type, other.m_units, other.m_description);
}
TypeInfo::TypeInfo(neměnné yang::LeafDataType& type, neměnné std::optional<std::string> units, neměnné std::optional<std::string> description)
    : m_type(type)
    , m_units(units)
    , m_description(description)
{
}
Enum::Enum(std::set<enum_>&& values)
    : m_allowedValues(std::move(values))
{
}
pravdivost Enum::operator==(neměnné Enum& other) neměnné
{
    vrať this->m_allowedValues == other.m_allowedValues;
}
IdentityRef::IdentityRef(std::set<identityRef_>&& values)
    : m_allowedValues(std::move(values))
{
}
pravdivost IdentityRef::operator==(neměnné IdentityRef& other) neměnné
{
    vrať this->m_allowedValues == other.m_allowedValues;
}
// Copy constructor needed, because unique_ptr is not copy-constructible
LeafRef::LeafRef(neměnné LeafRef& src)
    : m_targetXPath(src.m_targetXPath)
    , m_targetType(std::make_unique<TypeInfo>(*src.m_targetType))
{
}
LeafRef::LeafRef(neměnné std::string& xpath, std::unique_ptr<TypeInfo>&& type)
    : m_targetXPath(xpath)
    , m_targetType(std::move(type))
{
}
pravdivost LeafRef::operator==(neměnné LeafRef& other) neměnné
{
    vrať this->m_targetXPath == other.m_targetXPath && *this->m_targetType == *other.m_targetType;
}
pravdivost Union::operator==(neměnné Union& other) neměnné
{
    vrať this->m_unionTypes == other.m_unionTypes;
}
pravdivost String::operator==(neměnné String&) neměnné
{
    vrať true;
}
pravdivost Decimal::operator==(neměnné Decimal&) neměnné
{
    vrať true;
}
pravdivost Bool::operator==(neměnné Bool&) neměnné
{
    vrať true;
}
pravdivost Int8::operator==(neměnné Int8&) neměnné
{
    vrať true;
}
pravdivost Uint8::operator==(neměnné Uint8&) neměnné
{
    vrať true;
}
pravdivost Int16::operator==(neměnné Int16&) neměnné
{
    vrať true;
}
pravdivost Uint16::operator==(neměnné Uint16&) neměnné
{
    vrať true;
}
pravdivost Int32::operator==(neměnné Int32&) neměnné
{
    vrať true;
}
pravdivost Uint32::operator==(neměnné Uint32&) neměnné
{
    vrať true;
}
pravdivost Int64::operator==(neměnné Int64&) neměnné
{
    vrať true;
}
pravdivost Uint64::operator==(neměnné Uint64&) neměnné
{
    vrať true;
}
pravdivost Binary::operator==(neměnné Binary&) neměnné
{
    vrať true;
}
pravdivost Empty::operator==(neměnné Empty&) neměnné
{
    vrať true;
}
pravdivost Bits::operator==(neměnné Bits& other) neměnné
{
    vrať this->m_allowedValues == other.m_allowedValues;
}
}
