#pragma once


class CTree
{
    public:
        bool checkNode(const std::string&) const;
        void changeNode(const std::string&);
        void initDefault();

    private:
        std::vector< std::string > getChildren(const std::string&) const;
        std::map< std::string, std::vector<std::string > m_nodes;
        std::string m_cur_dir;
};
