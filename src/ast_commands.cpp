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

bool Move::operator==(const Move& other) const
{
    return this->m_destination == other.m_destination && this->m_mode == other.m_mode;
}

bool move_::operator==(const move_& other) const
{
    return this->m_path == other.m_path && this->m_move == other.m_move;
}
