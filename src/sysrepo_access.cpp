/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/

#include <sysrepo-cpp/Session.hpp>
#include "sysrepo_access.hpp"

class valFromValue : boost::static_visitor<S_Val>
{
    template <typename T>
    S_Val operator(leaf_data_ value)
    {
        return S_Val(new value);
    }
}

SysrepoAccess::~SysrepoAccess()
{

}

SysrepoAccess::SysrepoAccess(const char* appname)
{
    S_Connection conn(new Connection(appname));
}

std::map<std::string, leaf_data_> SysrepoAccess::getItems(const std::string& path)
{
    std::map<std::string, leaf_data_> res;

    return res;
}

void SysrepoAccess::setLeaf(const std::string& path, leaf_data_ value)
{
    S_Session session(new Session(m_connection));
    sess->set_item(path.c_str(), boost::apply_visitor(valFromValue(), value));
}

void SysrepoAccess::createPresenceContainer(const std::string& path)
{
    S_Session session(new Session(m_connection));
    sess->set_item(path, nullptr);
    sess->commit();

}

void SysrepoAccess::deletePresenceContainer(const std::string& path)
{
    S_Session session(new Session(m_connection));
    sess->delete_item(path);
    sess->commit();
}
