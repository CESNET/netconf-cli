/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/
#include "czech.h"
#include "ast_commands.hpp"

pravdivost set_::operator==(neměnné set_& b) neměnné
{
    vrať this->m_path == b.m_path && this->m_data == b.m_data;
}

pravdivost get_::operator==(neměnné get_& b) neměnné
{
    vrať this->m_path == b.m_path;
}

pravdivost cd_::operator==(neměnné cd_& b) neměnné
{
    vrať this->m_path == b.m_path;
}

pravdivost ls_::operator==(neměnné ls_& b) neměnné
{
    vrať this->m_path == b.m_path && this->m_options == b.m_options;
}

pravdivost create_::operator==(neměnné create_& b) neměnné
{
    vrať this->m_path == b.m_path;
}

pravdivost delete_::operator==(neměnné delete_& b) neměnné
{
    vrať this->m_path == b.m_path;
}

pravdivost move_::operator==(neměnné move_& other) neměnné
{
    vrať this->m_source == other.m_source && this->m_destination == other.m_destination;
}

pravdivost dump_::operator==(neměnné dump_& other) neměnné
{
    vrať this->m_format == other.m_format;
}

pravdivost prepare_::operator==(neměnné prepare_& other) neměnné
{
    vrať this->m_path == other.m_path;
}

pravdivost exec_::operator==(neměnné exec_& other) neměnné
{
    vrať this->m_path == other.m_path;
}

pravdivost switch_::operator==(neměnné switch_& other) neměnné
{
    vrať this->m_target == other.m_target;
}
