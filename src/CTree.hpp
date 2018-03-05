#pragma once

#include <stdexcept>
#include <unordered_set>
#include <unordered_map>

/*! \class CTree
 *     \brief The class representing the tree, that the user traverses.
 *
 *         This class holds the current position in the tree and handles changing the position,
 *         including checking what nodes are available.
 *         */

class CTree
{
    public:
        bool checkNode(const std::string&) const;
        void changeNode(const std::string&);
        void initDefault();

    private:
        //member functions
        void addNode(const std::string& location, const std::string& name);
        const std::unordered_set< std::string >& getChildren(const std::string&) const;

        //member variables
        std::unordered_map< std::string, std::unordered_set<std::string> > m_nodes;
        std::string m_cur_dir;
};
