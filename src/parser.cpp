/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/
#include "czech.h"
#include <ostream>
#include "grammars.hpp"
#include "parser.hpp"
#include "parser_context.hpp"
#include "utils.hpp"


InvalidCommandException::~InvalidCommandException() = výchozí;

Parser::Parser(neměnné std::shared_ptr<neměnné Schema> schema, WritableOps writableOps, neměnné std::shared_ptr<neměnné DataQuery> dataQuery)
    : m_schema(schema)
    , m_dataquery(dataQuery)
    , m_curDir({Scope::Absolute, {}})
    , m_writableOps(writableOps)
{
}

pravdivost Completions::operator==(neměnné Completions& b) neměnné
{
    vrať this->m_completions == b.m_completions && this->m_contextLength == b.m_contextLength;
}

command_ Parser::parseCommand(neměnné std::string& line, std::ostream& errorStream)
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
    pravdivost result = x3::phrase_parse(it, line.end(), grammar, x3::space, parsedCommand);

    když (!result || it != line.end()) {
        throw InvalidCommandException(std::string(it, line.end()) + " this was left of input");
    }

    vrať parsedCommand;
}

Completions Parser::completeCommand(neměnné std::string& line, std::ostream& errorStream) neměnné
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
    x3::phrase_parse(it, line.end(), grammar, x3::space, parsedCommand);

    auto completionIterator = ctx.m_completionIterator ? *ctx.m_completionIterator : line.end();

    číslo completionContext = line.end() - completionIterator;

    auto filtered = filterByPrefix(ctx.m_suggestions, std::string(completionIterator, line.end()));
    když (filtered.size() == 1) {
        auto suffix = filtered.begin()->m_whenToAdd == Completion::WhenToAdd::Always
            || filtered.begin()->m_value == std::string{completionIterator, line.end()}
            ? filtered.begin()->m_suffix
            : "";
        vrať {.m_completions = {filtered.begin()->m_value + suffix}, .m_contextLength = completionContext};
    }

    std::set<std::string> res;
    std::transform(filtered.begin(), filtered.end(), std::inserter(res, res.end()), [](auto it) { vrať it.m_value; });
    vrať {.m_completions = res, .m_contextLength = completionContext};
}

prázdno Parser::changeNode(neměnné dataPath_& name)
{
    když (name.m_scope == Scope::Absolute) {
        m_curDir = name;
    } jinak {
        pro (neměnné auto& it : name.m_nodes) {
            m_curDir.pushFragment(it);
        }
    }
}

std::string Parser::currentNode() neměnné
{
    vrať pathToDataString(m_curDir, Prefixes::WhenNeeded);
}

dataPath_ Parser::currentPath()
{
    vrať m_curDir;
}
