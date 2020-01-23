/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/
#include <ostream>
#include "grammars.hpp"
#include "parser.hpp"
#include "parser_context.hpp"

TooManyArgumentsException::~TooManyArgumentsException() = default;

InvalidCommandException::~InvalidCommandException() = default;


Parser::Parser(const std::shared_ptr<const Schema> schema)
    : m_schema(schema)
{
    m_curDir.m_scope = Scope::Absolute;
}

bool Completions::operator==(const Completions& b) const
{
    return this->m_completions == b.m_completions && this->m_contextLength == b.m_contextLength;
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

Completions Parser::completeCommand(const std::string& line, std::ostream& errorStream) const
{
    std::set<std::string> completions;
    command_ parsedCommand;
    ParserContext ctx(*m_schema, m_curDir);
    ctx.m_completing = true;
    auto it = line.begin();
    boost::spirit::x3::error_handler<std::string::const_iterator> errorHandler(it, line.end(), errorStream);

    auto grammar =
            x3::with<parser_context_tag>(ctx)[
            x3::with<x3::error_handler_tag>(std::ref(errorHandler))[command]
    ];
    x3::phrase_parse(it, line.end(), grammar, space, parsedCommand);

    int completionLocation = line.end() - ctx.m_completionIterator;

    auto set = filterByPrefix(ctx.m_suggestions, std::string(ctx.m_completionIterator, line.end()));
    if (set.size() == 1) {
        return {{(*set.begin()) + ctx.m_completionSuffix}, completionLocation};
    }
    return {set, completionLocation};
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
    return pathToDataString(m_curDir, Prefixes::WhenNeeded);
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


std::set<std::string> Parser::availableNodes(const boost::optional<boost::variant<boost::variant<dataPath_, schemaPath_>, module_>>& path, const Recursion& option) const
{
    auto pathArg = dataPathToSchemaPath(m_curDir);
    if (path) {
        if (path->type() == typeid(module_)) {
            return m_schema->moduleNodes(boost::get<module_>(*path), option);
        }

        auto schemaPath = boost::apply_visitor(getSchemaPathVisitor(), boost::get<boost::variant<dataPath_, schemaPath_>>(*path));
        if (schemaPath.m_scope == Scope::Absolute) {
            pathArg = schemaPath;
        } else {
            pathArg.m_nodes.insert(pathArg.m_nodes.end(), schemaPath.m_nodes.begin(), schemaPath.m_nodes.end());
        }
    }
    return m_schema->childNodes(pathArg, option);
}
