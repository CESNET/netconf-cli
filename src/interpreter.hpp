/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/

#pragma once

#include <boost/variant/static_visitor.hpp>
#include "parser.hpp"

struct Interpreter : boost::static_visitor<void> {
    Interpreter();

    Interpreter(Parser &m_parser);

    void operator()(const cd_&) const;
private:
    Parser& m_parser;
};
