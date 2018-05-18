#include "parser_context.hpp"
ParserContext::ParserContext(const CTree& tree)
        : m_tree(tree)
{
    m_curPath = m_tree.currentNode();
}
