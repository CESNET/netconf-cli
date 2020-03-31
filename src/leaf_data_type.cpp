#include "leaf_data_type.hpp"
namespace yang {
Enum::Enum(const std::initializer_list<const char*>& list)
{
    std::transform(list.begin(), list.end(), std::inserter(m_allowedValues, m_allowedValues.end()), [] (const auto& value) {
        return enum_{value};
    });
}
Enum::Enum(const std::set<enum_>& values)
    : m_allowedValues(values)
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
IdentityRef::IdentityRef(const std::set<identityRef_>& values)
    : m_allowedValues(std::move(values))
{
}
IdentityRef::IdentityRef(std::set<identityRef_>&& values)
    : m_allowedValues(std::move(values))
{
}
bool IdentityRef::operator==(const IdentityRef& other) const
{
    return this->m_allowedValues == other.m_allowedValues;
}
bool LeafRef::operator==(const LeafRef& other) const
{
    return this->m_pointsTo == other.m_pointsTo;
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
}
