#include "CParser.hpp"
CParser::CParser(const CTree& tree)
    : m_tree(tree)
{
}

Cmd CParser::parseInput(const std::string& line)
{
    auto const node_up =
        lexeme[x3::string("..")];
    auto const nodename =
        lexeme[+alpha];
    auto const cd_args =
        (
         (+(node_up >> +(char_('/'))) >> -node_up) | //for only going up (../../..)
         (*(node_up >> +(char_('/'))) >> nodename[checkNode(m_tree)] >> *("/" >> nodename[checkNode(m_tree)]))
        )
        >> x3::eoi;
    auto const cd =
        x3::string("cd")
        >> cd_args
        >> x3::eoi;
    auto const keywords =
        cd;

    /*auto const checkNode =
        [&](auto& ctx) {
            std::cout << "trying to enter " << _attr(ctx) << std::endl;
            m_tree.checkNode(m_tree.currentNode(), _attr(ctx));
        };*/
    Cmd args;
    auto it = line.begin();
    bool result = x3::phrase_parse(it, line.end(), keywords, space, args);
    std::cout << "success: " << result << std::endl;
    if (!result) {
        throw TooManyArgumentsException(std::string(it, line.end()));
    }


    return args;
}
