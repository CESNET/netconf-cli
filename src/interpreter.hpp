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
    void operator()(const get_&) const;
    void operator()(const cd_&) const;
    void operator()(const create_&) const;
    void operator()(const delete_&) const;
    void operator()(const ls_&) const;
    void operator()(const describe_&) const;
    void operator()(const discard_&) const;
    void operator()(const help_&) const;
    void operator()(const copy_& copy) const;

private:
    template <typename T>
    std::string absolutePathFromCommand(const T& command) const;
    std::string absolutePathFromCommand(const get_& command) const;
    std::string absolutePathFromCommand(const describe_& describe) const;
    std::string buildTypeInfo(const std::string& path) const;

    Parser& m_parser;
    DatastoreAccess& m_datastore;
};
