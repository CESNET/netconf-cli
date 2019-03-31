/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/
#include "ast_commands.hpp"

enum_::enum_() = default                                                        ;

enum_::enum_(const std::string& value)
    : m_value(value)                                                            {}

identityRef_::identityRef_() = default                                          ;

identityRef_::identityRef_(const std::string& value)
    : m_value(value)                                                            {}

identityRef_::identityRef_(const std::string& module, const std::string& value)
    : m_prefix(module_{module})
    , m_value(value)                                                            {}

binary_::binary_() = default                                                    ;

binary_::binary_(const std::string& value)
    : m_value(value)                                                            {}

bool set_::operator==(const set_& b) const                                      {
    return this->m_path == b.m_path && this->m_data == b.m_data                 ;}

bool cd_::operator==(const cd_& b) const                                        {
    return this->m_path == b.m_path                                             ;}

bool ls_::operator==(const ls_& b) const                                        {
    return this->m_path == b.m_path && this->m_options == b.m_options           ;}

bool identityRef_::operator==(const identityRef_& b) const                      {
    return this->m_prefix == b.m_prefix && this->m_value == b.m_value           ;}

bool binary_::operator==(const binary_& b) const                                {
    return this->m_value == b.m_value                                           ;}

bool enum_::operator==(const enum_& b) const                                    {
    return this->m_value == b.m_value                                           ;}

bool create_::operator==(const create_& b) const                                {
    return this->m_path == b.m_path                                             ;}

bool delete_::operator==(const delete_& b) const                                {
    return this->m_path == b.m_path                                             ;}
