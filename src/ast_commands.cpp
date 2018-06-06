/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/
#include "ast_commands.hpp"


bool set_::operator==(const set_& b) const
{
    return this->m_path == b.m_path && this->m_data == b.m_data;
}

bool cd_::operator==(const cd_& b) const
{
    return this->m_path == b.m_path;
}

bool create_::operator==(const create_& b) const
{
    return this->m_path == b.m_path;
}

bool delete_::operator==(const delete_& b) const
{
    return this->m_path == b.m_path;
}
