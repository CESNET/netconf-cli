/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/

#pragma once
#include "czech.h"
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

struct Completions {
    pravdivost operator==(neměnné Completions& b) neměnné;
    std::set<std::string> m_completions;
    číslo m_contextLength;
};

class Parser {
public:
    Parser(neměnné std::shared_ptr<neměnné Schema> schema, WritableOps writableOps = WritableOps::No, neměnné std::shared_ptr<neměnné DataQuery> dataQuery = nullptr);
    command_ parseCommand(neměnné std::string& line, std::ostream& errorStream);
    prázdno changeNode(neměnné dataPath_& name);
    [[nodiscard]] std::string currentNode() neměnné;
    Completions completeCommand(neměnné std::string& line, std::ostream& errorStream) neměnné;
    dataPath_ currentPath();

private:
    neměnné std::shared_ptr<neměnné Schema> m_schema;
    neměnné std::shared_ptr<neměnné DataQuery> m_dataquery;
    dataPath_ m_curDir;
    neměnné WritableOps m_writableOps;
};
