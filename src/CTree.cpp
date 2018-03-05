#include "CTree.hpp"

class InvalidNodeException
{
    public:
        InvalidNodeException(const string& node)
        {
            m_dir = node;
        }
        void what()
        {
            std::cerr << "can't cd into: " << m_dir << std::endl;               
        }
    private:
        std::string m_dir;
};

std::vector< std::string >& CTree::getChildren(const std::string& node) const
{
    return m_nodes.at(node);
}

CTree::checkNode(const std::string& node) const
{
    if (node == "..") return;
    if (getChildren.find(node) == m_nodes.end()) {
        throw InvalidNodeException(node);
    }
}
void CTree::changeNode(const std::string& node)
{
    m_cur_dir += node;
}
void CTree::addNode(const string& location, const string& name)
{
    m_nodes.at(location).push_back(name);
    m_nodes.emplace(location + "/" + name, std::vector< std::string >());
}
void CTree::initDefault()
{
    m_nodes.emplace("", std::vector< std::string >());
    addNode("", "aa");
    addNode("", "aa");
}
