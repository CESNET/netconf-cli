/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/

#pragma once
#include "ast_commands.hpp"
#include "ast_path.hpp"
#include "data_query.hpp"
#include "schema.hpp"


class InvalidCommandException : public std::invalid_argument {
public:
    using std::invalid_argument::invalid_argument;
    ~InvalidCommandException() override;
};

class TooManyArgumentsException : public std::invalid_argument {
public:
    using std::invalid_argument::invalid_argument;
    ~TooManyArgumentsException() override;
};

class Parser {
public:
    Parser(const std::shared_ptr<const Schema> schema, const std::shared_ptr<const DataQuery> dataQuery);
    command_ parseCommand(const std::string& line, std::ostream& errorStream);
    void changeNode(const dataPath_& name);
    std::string currentNode() const;
    std::set<std::string> availableNodes(const boost::optional<boost::variant<boost::variant<dataPath_, schemaPath_>, module_>>& path, const Recursion& option) const;
    std::set<std::string> completeCommand(const std::string& line, std::ostream& errorStream) const;

private:
    const std::shared_ptr<const Schema> m_schema;
    const std::shared_ptr<const DataQuery> m_dataquery;
    dataPath_ m_curDir;
};
