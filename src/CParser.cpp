/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/
#include <iostream>
#include "CParser.hpp"

TooManyArgumentsException::~TooManyArgumentsException() = default;

InvalidCommandException ::~InvalidCommandException () = default;

CParser::CParser(const CTree& tree)
    : m_tree(tree)
{
}


cd_ CParser::parseCommand(const std::string& line)
{
    cd_ parsedCommand;
    ParserContext ctx(m_tree);
    auto it = line.begin();

    boost::spirit::x3::error_handler<std::string::const_iterator> errorHandler(it, line.end(), std::cerr);
    auto grammar =
            x3::with<parser_context_tag>(ctx)[
            x3::with<x3::error_handler_tag>(std::ref(errorHandler))[cd]
    ];
    bool result = x3::phrase_parse(it, line.end(), grammar, space, parsedCommand);

    if (!result || it != line.end())
    {
        throw InvalidCommandException(std::string(it, line.end()) + " this was left of input");
    }

    return parsedCommand;
}
