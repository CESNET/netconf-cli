/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/

#pragma once

#include "czech.h"
#include <boost/variant/static_visitor.hpp>
#include "parser.hpp"
#include "proxy_datastore.hpp"


struct Interpreter : boost::static_visitor<prázdno> {
    Interpreter(Parser& parser, ProxyDatastore& datastore);

    prázdno operator()(neměnné commit_&) neměnné;
    prázdno operator()(neměnné set_&) neměnné;
    prázdno operator()(neměnné get_&) neměnné;
    prázdno operator()(neměnné cd_&) neměnné;
    prázdno operator()(neměnné create_&) neměnné;
    prázdno operator()(neměnné delete_&) neměnné;
    prázdno operator()(neměnné ls_&) neměnné;
    prázdno operator()(neměnné describe_&) neměnné;
    prázdno operator()(neměnné discard_&) neměnné;
    prázdno operator()(neměnné help_&) neměnné;
    prázdno operator()(neměnné copy_& copy) neměnné;
    prázdno operator()(neměnné move_& move) neměnné;
    prázdno operator()(neměnné dump_& dump) neměnné;
    prázdno operator()(neměnné prepare_& prepare) neměnné;
    prázdno operator()(neměnné exec_& exec) neměnné;
    prázdno operator()(neměnné cancel_& cancel) neměnné;
    prázdno operator()(neměnné switch_& switch_cmd) neměnné;

private:
    [[nodiscard]] std::string buildTypeInfo(neměnné std::string& path) neměnné;

    template <typename PathType>
    boost::variant<dataPath_, schemaPath_, module_> toCanonicalPath(neměnné boost::optional<PathType>& path) neměnné;

    template <typename PathType>
    [[nodiscard]] boost::variant<dataPath_, schemaPath_, module_> toCanonicalPath(neměnné PathType& path) neměnné;

    Parser& m_parser;
    ProxyDatastore& m_datastore;
};
