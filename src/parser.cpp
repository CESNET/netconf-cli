/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/
#include <experimental/iterator>
#include <ostream>
#include "parser.hpp"

TooManyArgumentsException::~TooManyArgumentsException() = default;

InvalidCommandException::~InvalidCommandException() = default;


Parser::Parser(const Schema& schema)
    : m_schema(schema)
{
}

struct nodeToDataString : public boost::static_visitor<std::string> {
    std::string operator()(const listElement_& node) const
    {
        std::ostringstream res;
        res << node.m_name + "[";
        std::transform(node.m_keys.begin(), node.m_keys.end(),
                       std::experimental::make_ostream_joiner(res, ' '),
                       [] (const auto& it) { return it.first + "=" + it.second; });
        res << "]";
        return res.str();
    }
    std::string operator()(const nodeup_&) const
    {
        return "..";
    }
    template <class T>
    std::string operator()(const T& node) const
    {
        return node.m_name;
    }
};

command_ Parser::parseCommand(const std::string& line, std::ostream& errorStream)
{
    command_ parsedCommand;
    ParserContext ctx(m_schema, m_curDir);
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
    for (const auto& it : name.m_nodes) {
        if (it.type() == typeid(nodeup_))
            m_curDir.m_nodes.pop_back();
        else
            m_curDir.m_nodes.push_back(it);
    }
}

std::string Parser::currentNode() const
{
    std::string res;
    for (const auto& it : m_curDir.m_nodes) {
        res = joinPaths(res, boost::apply_visitor(nodeToDataString(), it));
    }

    return res;
}
