/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/

#pragma once

#include <boost/variant/static_visitor.hpp>
#include "proxy_datastore.hpp"
#include "parser.hpp"


struct Interpreter : boost::static_visitor<void> {
    Interpreter(Parser& parser, ProxyDatastore& datastore);

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
    void operator()(const move_& move) const;
    void operator()(const dump_& dump) const;

private:
    [[nodiscard]] std::string buildTypeInfo(const std::string& path) const;

    template <typename PathType>
    boost::variant<dataPath_, schemaPath_, module_> toCanonicalPath(const boost::optional<PathType>& path) const;

    template <typename PathType>
    [[nodiscard]] boost::variant<dataPath_, schemaPath_, module_> toCanonicalPath(const PathType& path) const;

    Parser& m_parser;
    ProxyDatastore& m_datastore;
};
