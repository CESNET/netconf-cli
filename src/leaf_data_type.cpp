/*
 * Copyright (C) 2020 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
*/
#include "ast_values.hpp"
#include "leaf_data_type.hpp"

namespace yang {
bool TypeInfo::operator==(const TypeInfo& other) const
{
    return std::tie(this->m_type, this->m_units, this->m_description) == std::tie(other.m_type, other.m_units, other.m_description);
}
TypeInfo::TypeInfo(const yang::LeafDataType& type, const std::optional<std::string> units, const std::optional<std::string> description)
    : m_type(type)
    , m_units(units)
    , m_description(description)
{
}
Enum::Enum(std::set<enum_>&& values)
    : m_allowedValues(std::move(values))
{
}
bool Enum::operator==(const Enum& other) const
{
    return this->m_allowedValues == other.m_allowedValues;
}
IdentityRef::IdentityRef(std::set<identityRef_>&& values)
    : m_allowedValues(std::move(values))
{
}
bool IdentityRef::operator==(const IdentityRef& other) const
{
    return this->m_allowedValues == other.m_allowedValues;
}
// Copy constructor needed, because unique_ptr is not copy-constructible
LeafRef::LeafRef(const LeafRef& src)
    : m_targetXPath(src.m_targetXPath)
    , m_targetType(std::make_unique<TypeInfo>(*src.m_targetType))
{
}
LeafRef::LeafRef(const std::string& xpath, std::unique_ptr<TypeInfo>&& type)
    : m_targetXPath(xpath)
    , m_targetType(std::move(type))
{
}
bool LeafRef::operator==(const LeafRef& other) const
{
    return this->m_targetXPath == other.m_targetXPath && *this->m_targetType == *other.m_targetType;
}
bool Union::operator==(const Union& other) const
{
    return this->m_unionTypes == other.m_unionTypes;
}
bool String::operator==(const String&) const
{
    return true;
}
bool Decimal::operator==(const Decimal&) const
{
    return true;
}
bool Bool::operator==(const Bool&) const
{
    return true;
}
bool Int8::operator==(const Int8&) const
{
    return true;
}
bool Uint8::operator==(const Uint8&) const
{
    return true;
}
bool Int16::operator==(const Int16&) const
{
    return true;
}
bool Uint16::operator==(const Uint16&) const
{
    return true;
}
bool Int32::operator==(const Int32&) const
{
    return true;
}
bool Uint32::operator==(const Uint32&) const
{
    return true;
}
bool Int64::operator==(const Int64&) const
{
    return true;
}
bool Uint64::operator==(const Uint64&) const
{
    return true;
}
bool Binary::operator==(const Binary&) const
{
    return true;
}
bool Empty::operator==(const Empty&) const
{
    return true;
}
bool Bits::operator==(const Bits& other) const
{
    return this->m_allowedValues == other.m_allowedValues;
}
InstanceIdentifier::InstanceIdentifier()
{
}
bool InstanceIdentifier::operator==(const InstanceIdentifier&) const
{
    return true;
}
}
