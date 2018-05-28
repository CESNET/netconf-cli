/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/

#include "interpreter.hpp"

void Interpreter::operator()(const cd_& cd) const
{
    m_parser.changeNode(cd.m_path);
}

void Interpreter::operator()(const create_&) const
{

}

void Interpreter::operator()(const delete_&) const
{

}

Interpreter::Interpreter(Parser& parser, Schema& schema)
    : m_parser(parser), m_schema(schema)
{
}
