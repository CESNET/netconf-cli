/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/

#pragma once

#include <boost/variant/static_visitor.hpp>
#include "datastore_access.hpp"
#include "parser.hpp"

struct Interpreter : boost::static_visitor<void> {
    Interpreter(Parser& parser, DatastoreAccess& datastore);

    void operator()(const commit_&) const;
    void operator()(const set_&) const;
    void operator()(const cd_&) const;
    void operator()(const create_&) const;
    void operator()(const delete_&) const;
    void operator()(const ls_&) const;

private:
    template <typename T>
    std::string absolutePathFromCommand(const T& command) const;

    Parser& m_parser;
    DatastoreAccess& m_datastore;
};
