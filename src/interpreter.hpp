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

enum class ConvertToString {
    Yes,
    No
};

template <ConvertToString value> struct impl_GetReturnType;
template <> struct impl_GetReturnType<ConvertToString::Yes> {using type = std::string;};
template <> struct impl_GetReturnType<ConvertToString::No> {using type = boost::variant<dataPath_, schemaPath_, module_>;};

template <ConvertToString value> using GetReturnType = typename impl_GetReturnType<value>::type;

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
    void operator()(const move_& move) const;

private:
    std::string buildTypeInfo(const std::string& path) const;

    template <ConvertToString CONVERT_TO_STRING, typename PathType>
    auto resolveOptionalPath(const boost::optional<PathType>& optPath) const -> GetReturnType<CONVERT_TO_STRING>;

    template <ConvertToString CONVERT_TO_STRING, typename PathType>
    auto resolvePath(const PathType& path) const -> GetReturnType<CONVERT_TO_STRING>;

    Parser& m_parser;
    DatastoreAccess& m_datastore;
};
