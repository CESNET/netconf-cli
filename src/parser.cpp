/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/
#include <ostream>
#include "parser.hpp"

TooManyArgumentsException::~TooManyArgumentsException() = default;

InvalidCommandException::~InvalidCommandException() = default;


Parser::Parser(const std::shared_ptr<const Schema> schema)
    : m_schema(schema)
{
}

command_ Parser::parseCommand(const std::string& line, std::ostream& errorStream)
{
    command_ parsedCommand;
    ParserContext ctx(*m_schema, m_curDir);
    auto it = line.begin();

    boost::spirit::x3::error_handler<std::string::const_iterator> errorHandler(it, line.end(), errorStream);

    auto grammar =
            x3::with<parser_context_tag>(ctx)[
            x3::with<x3::error_handler_tag>(std::ref(errorHandler))[command]
    ];
    bool result = x3::phrase_parse(it, line.end(), grammar, space, parsedCommand);

    if (!result || it != line.end()) {
        throw InvalidCommandException(std::string(it, line.end()) + " this was left of input");
    }

    return parsedCommand;
}

void Parser::changeNode(const path_& name)
{
    if (name.m_absolute) {
        m_curDir = name;
    } else {
        for (const auto& it : name.m_nodes) {
            if (it.m_suffix.type() == typeid(nodeup_))
                m_curDir.m_nodes.pop_back();
            else
                m_curDir.m_nodes.push_back(it);
        }
    }
}

std::string Parser::currentNode() const
{
    return "/" + pathToDataString(m_curDir);
}

std::set<std::string> Parser::availableNodes(const boost::optional<path_>& path) const
{
    auto pathArg = m_curDir;
    if (path)
        pathArg.m_nodes.insert(pathArg.m_nodes.end(), path->m_nodes.begin(), path->m_nodes.end());
    return m_schema->childNodes(pathArg);
}
