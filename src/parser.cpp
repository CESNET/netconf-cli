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
#include "utils.hpp"


InvalidCommandException::~InvalidCommandException() = default;

Parser::Parser(const std::shared_ptr<const Schema> schema, WritableOps writableOps, const std::shared_ptr<const DataQuery> dataQuery)
    : m_schema(schema)
    , m_dataquery(dataQuery)
    , m_curDir({Scope::Absolute, {}})
    , m_writableOps(writableOps)
{
}

bool Completions::operator==(const Completions& b) const
{
    return this->m_completions == b.m_completions && this->m_contextLength == b.m_contextLength;
}

command_ Parser::parseCommand(const std::string& line, std::ostream& errorStream)
{
    command_ parsedCommand;
    ParserContext ctx(*m_schema, nullptr, m_curDir);
    auto it = line.begin();

    boost::spirit::x3::error_handler<std::string::const_iterator> errorHandler(it, line.end(), errorStream);

    auto grammar =
            x3::with<parser_context_tag>(ctx)[
            x3::with<x3::error_handler_tag>(std::ref(errorHandler))[
            x3::with<writableOps_tag>(m_writableOps)[command]]
    ];
    bool result = x3::parse(it, line.end(), grammar, parsedCommand);

    if (!result || it != line.end()) {
        throw InvalidCommandException(std::string(it, line.end()) + " this was left of input");
    }

    return parsedCommand;
}

Completions Parser::completeCommand(const std::string& line, std::ostream& errorStream) const
{
    std::set<std::string> completions;
    command_ parsedCommand;
    ParserContext ctx(*m_schema, m_dataquery, m_curDir);
    ctx.m_completing = true;
    auto it = line.begin();
    boost::spirit::x3::error_handler<std::string::const_iterator> errorHandler(it, line.end(), errorStream);

    auto grammar =
            x3::with<parser_context_tag>(ctx)[
            x3::with<x3::error_handler_tag>(std::ref(errorHandler))[
            x3::with<writableOps_tag>(m_writableOps)[command]]
    ];
    x3::parse(it, line.end(), grammar, parsedCommand);

    auto completionIterator = ctx.m_completionIterator ? *ctx.m_completionIterator : line.end();

    int completionContext = line.end() - completionIterator;

    auto filtered = filterByPrefix(ctx.m_suggestions, std::string(completionIterator, line.end()));
    if (filtered.size() == 1) {
        auto suffix = filtered.begin()->m_whenToAdd == Completion::WhenToAdd::Always
            || filtered.begin()->m_value == std::string{completionIterator, line.end()}
            ? filtered.begin()->m_suffix
            : "";
        return {.m_completions = {filtered.begin()->m_value + suffix}, .m_contextLength = completionContext};
    }

    std::set<std::string> res;
    std::transform(filtered.begin(), filtered.end(), std::inserter(res, res.end()), [](auto it) { return it.m_value; });
    return {.m_completions = res, .m_contextLength = completionContext};
}

void Parser::changeNode(const dataPath_& name)
{
    m_curDir = realPath(m_curDir, name);
}

std::string Parser::currentNode() const
{
    return pathToDataString(m_curDir, Prefixes::WhenNeeded);
}

dataPath_ Parser::currentPath()
{
    return m_curDir;
}
