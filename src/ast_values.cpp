/*
 * Copyright (C) 2019 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/
#include "ast_values.hpp"

enum_::enum_() = default;

enum_::enum_(const std::string& value)
    : m_value(value)
{
}

identityRef_::identityRef_() = default;

identityRef_::identityRef_(const std::string& value)
    : m_value(value)
{
}

identityRef_::identityRef_(const std::string& module, const std::string& value)
    : m_prefix(module_{module})
    , m_value(value)
{
}

binary_::binary_() = default;

binary_::binary_(const std::string& value)
    : m_value(value)
{
}

bool module_::operator<(const module_& b) const
{
    return this->m_name < b.m_name;
}

bool identityRef_::operator==(const identityRef_& b) const
{
    return this->m_prefix == b.m_prefix && this->m_value == b.m_value;
}

bool identityRef_::operator<(const identityRef_& b) const
{
    return std::tie(this->m_prefix, this->m_value) < std::tie(b.m_prefix, b.m_value);
}

bool binary_::operator==(const binary_& b) const
{
    return this->m_value == b.m_value;
}

bool binary_::operator<(const binary_& b) const
{
    return this->m_value < b.m_value;
}

bool enum_::operator==(const enum_& b) const
{
    return this->m_value == b.m_value;
}

bool enum_::operator<(const enum_& b) const
{
    return this->m_value < b.m_value;
}

bool special_::operator==(const special_& b) const
{
    return this->m_value == b.m_value;
}

bool special_::operator<(const special_& b) const
{
    return this->m_value < b.m_value;
}

std::string specialValueToString(const special_& value)
{
    switch (value.m_value) {
    case SpecialValue::Container:
        return "(container)";
    case SpecialValue::PresenceContainer:
        return "(presence container)";
    case SpecialValue::List:
        return "(list)";
    }

    __builtin_unreachable();
}
