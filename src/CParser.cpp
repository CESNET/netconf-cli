/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/
#include "CParser.hpp"

TooManyArgumentsException::~TooManyArgumentsException() = default;

CParser::CParser(const CTree& tree)
    : m_tree(tree)
{
}


cd_ CParser::parseCommand(const std::string& line)
{
    cd_ parsedCommand;
    ParserContext ctx(m_tree);
    auto it = line.begin();

    auto grammar = x3::with<parser_context_tag>(ctx)[cd];

    x3::phrase_parse(it, line.end(), grammar, space, parsedCommand);


    return parsedCommand;
}
