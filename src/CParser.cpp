#include "CParser.hpp"

TooManyArgumentsException::~TooManyArgumentsException()

CParser::CParser(const CTree& tree)
    : m_tree(tree)
{
}


Cmd CParser::parseInput(const std::string& line)
{
    Cmd args;
    auto it = line.begin();
    bool result = x3::phrase_parse(it, line.end(), command, space, args);
    std::cout << "success: " << result << std::endl;
    if (it != line.end()) {
        throw TooManyArgumentsException(std::string(it, line.end()));
    }


    return args;
}
