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

bool get_::operator==(const get_& b) const
{
    return this->m_path == b.m_path;
}

bool cd_::operator==(const cd_& b) const
{
    return this->m_path == b.m_path;
}

bool ls_::operator==(const ls_& b) const
{
    return this->m_path == b.m_path && this->m_options == b.m_options;
}

bool create_::operator==(const create_& b) const
{
    return this->m_path == b.m_path;
}

bool delete_::operator==(const delete_& b) const
{
    return this->m_path == b.m_path;
}

bool move_::operator==(const move_& other) const
{
    return this->m_source == other.m_source && this->m_destination == other.m_destination;
}

bool dump_::operator==(const dump_& other) const
{
    return this->m_format == other.m_format;
}

bool prepare_::operator==(const prepare_& other) const
{
    return this->m_path == other.m_path;
}

bool exec_::operator==(const exec_& other) const
{
    return this->m_path == other.m_path;
}

bool switch_::operator==(const switch_& other) const
{
    return this->m_mode == other.m_mode;
}
