/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/
#include <ostream>
#include "CParser.hpp"

TooManyArgumentsException::~TooManyArgumentsException() = default;

InvalidCommandException::~InvalidCommandException() = default;

struct nodeToString : public boost::static_visitor<std::string> {
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

CParser::CParser(const CTree& tree)
    : m_tree(tree)
{
}

cd_ CParser::parseCommand(const std::string& line, std::ostream& errorStream)
{
    cd_ parsedCommand;
    ParserContext ctx(m_tree, m_curDir);
    auto it = line.begin();

    boost::spirit::x3::error_handler<std::string::const_iterator> errorHandler(it, line.end(), errorStream);

    auto grammar =
            x3::with<parser_context_tag>(ctx)[
            x3::with<x3::error_handler_tag>(std::ref(errorHandler))[cd]
    ];
    bool result = x3::phrase_parse(it, line.end(), grammar, space, parsedCommand);

    if (!result || it != line.end()) {
        throw InvalidCommandException(std::string(it, line.end()) + " this was left of input");
    }

    return parsedCommand;
}

void CParser::changeNode(const path_& name)
{
    if (name.m_nodes.empty()) {
        m_curDir = "";
        return;
    }
    for (const auto& it : name.m_nodes) {
        const std::string node = boost::apply_visitor(nodeToString(), it);
        if (node == "..") {
            m_curDir = stripLastNodeFromPath(m_curDir);
        } else {
            m_curDir = joinPaths(m_curDir, boost::apply_visitor(nodeToString(), it));
        }

    }
}
std::string CParser::currentNode() const
{
    return m_curDir;
}

