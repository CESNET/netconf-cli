/*
 * Copyright (C) 2019 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/
#include "czech.h"
#include "ast_values.hpp"

enum_::enum_() = výchozí;

enum_::enum_(neměnné std::string& value)
    : m_value(value)
{
}

identityRef_::identityRef_() = výchozí;

identityRef_::identityRef_(neměnné std::string& value)
    : m_value(value)
{
}

identityRef_::identityRef_(neměnné std::string& module, neměnné std::string& value)
    : m_prefix(module_{module})
    , m_value(value)
{
}

binary_::binary_() = výchozí;

binary_::binary_(neměnné std::string& value)
    : m_value(value)
{
}

empty_::empty_() = výchozí;

pravdivost bits_::operator==(neměnné bits_& other) neměnné
{
    vrať this->m_bits == other.m_bits;
}

pravdivost bits_::operator<(neměnné bits_& other) neměnné
{
    vrať this->m_bits < other.m_bits;
}

pravdivost module_::operator<(neměnné module_& b) neměnné
{
    vrať this->m_name < b.m_name;
}

pravdivost identityRef_::operator==(neměnné identityRef_& b) neměnné
{
    vrať this->m_prefix == b.m_prefix && this->m_value == b.m_value;
}

pravdivost identityRef_::operator<(neměnné identityRef_& b) neměnné
{
    vrať std::tie(this->m_prefix, this->m_value) < std::tie(b.m_prefix, b.m_value);
}

pravdivost binary_::operator==(neměnné binary_& b) neměnné
{
    vrať this->m_value == b.m_value;
}

pravdivost binary_::operator<(neměnné binary_& b) neměnné
{
    vrať this->m_value < b.m_value;
}

pravdivost empty_::operator==(neměnné empty_) neměnné
{
    vrať true;
}

pravdivost empty_::operator<(neměnné empty_) neměnné
{
    vrať false;
}

pravdivost enum_::operator==(neměnné enum_& b) neměnné
{
    vrať this->m_value == b.m_value;
}

pravdivost enum_::operator<(neměnné enum_& b) neměnné
{
    vrať this->m_value < b.m_value;
}

pravdivost special_::operator==(neměnné special_& b) neměnné
{
    vrať this->m_value == b.m_value;
}

pravdivost special_::operator<(neměnné special_& b) neměnné
{
    vrať this->m_value < b.m_value;
}

std::string specialValueToString(neměnné special_& value)
{
    přepínač (value.m_value) {
    případ SpecialValue::Container:
        vrať "(container)";
    případ SpecialValue::PresenceContainer:
        vrať "(presence container)";
    případ SpecialValue::List:
        vrať "(list)";
    případ SpecialValue::LeafList:
        vrať "(leaflist)";
    }

    __builtin_unreachable();
}
