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
    ParserContext ctx(*m_schema, dataPathToSchemaPath(m_curDir));
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
#include <iostream>
std::vector<std::string> Parser::completeCommand(const std::string& line, std::ostream& errorStream) const
{
    std::vector<std::string> completions;
    command_ parsedCommand;
    ParserContext ctx(*m_schema, dataPathToSchemaPath(m_curDir));
    auto it = line.begin();
    boost::spirit::x3::error_handler<std::string::const_iterator> errorHandler(it, line.end(), errorStream);

    auto grammar =
            x3::with<parser_context_tag>(ctx)[
            x3::with<x3::error_handler_tag>(std::ref(errorHandler))[command]
    ];
    bool result = x3::phrase_parse(it, line.end(), grammar, space, parsedCommand);

    if (result && it == line.end()) {
        std::cout << "[DBG]complete parse, no completions";
    } else if (!result || it != line.end()) {
        std::cout << "[DBG]incomplete parse \"" << std::string(line.begin(), it) << "⏐" << std::string(it, line.end()) << "\"";
    }

    if (ctx.m_parsingPath)
        std::cout << " (path rule was used at some point)";
    std::cout << std::endl;

    completions.push_back(line + "");
    return completions;
}

void Parser::changeNode(const dataPath_& name)
{
    if (name.m_scope == Scope::Absolute) {
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

struct getSchemaPathVisitor : boost::static_visitor<schemaPath_> {
    schemaPath_ operator()(const dataPath_& path) const
    {
        return dataPathToSchemaPath(path);
    }

    schemaPath_ operator()(const schemaPath_& path) const
    {
        return path;
    }
};


std::set<std::string> Parser::availableNodes(const boost::optional<boost::variant<dataPath_, schemaPath_>>& path, const Recursion& option) const
{
    auto pathArg = dataPathToSchemaPath(m_curDir);
    if (path) {
        auto schemaPath = boost::apply_visitor(getSchemaPathVisitor(), *path);
        pathArg.m_nodes.insert(pathArg.m_nodes.end(), schemaPath.m_nodes.begin(), schemaPath.m_nodes.end());
    }
    return m_schema->childNodes(pathArg, option);
}
