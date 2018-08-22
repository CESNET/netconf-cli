/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/

#include <sysrepo-cpp/Session.h>
#include "sysrepo_access.hpp"


struct valFromValue : boost::static_visitor<S_Val> {
    S_Val operator()(const enum_& value) const
    {
        return std::make_shared<Val>(value.m_value.c_str(), SR_ENUM_T);
    }

    S_Val operator()(const std::string& value) const
    {
        return std::make_shared<Val>(value.c_str());
    }

    S_Val operator()(const uint32_t& value) const
    {
        return std::make_shared<Val>(value, SR_UINT32_T);
    }

    S_Val operator()(const int32_t& value) const
    {
        return std::make_shared<Val>(value, SR_INT32_T);
    }

    template <typename T>
    S_Val operator()(const T& value) const
    {
        return std::make_shared<Val>(value);
    }
};

SysrepoAccess::~SysrepoAccess()
{
    m_session->commit();
}

SysrepoAccess::SysrepoAccess(const std::string& appname)
    : m_connection(new Connection(appname.c_str()))
{
    m_session = std::make_shared<Session>(m_connection);
}

std::map<std::string, leaf_data_> SysrepoAccess::getItems(const std::string& path)
{
    std::map<std::string, leaf_data_> res;
    auto iterator = m_session->get_items_iter(path.c_str());

    return res;
}

void SysrepoAccess::setLeaf(const std::string& path, leaf_data_ value)
{
    m_session->set_item(path.c_str(), boost::apply_visitor(valFromValue(), value));
}

void SysrepoAccess::createPresenceContainer(const std::string& path)
{
    m_session->set_item(path.c_str());
}

void SysrepoAccess::deletePresenceContainer(const std::string& path)
{
    m_session->delete_item(path.c_str());
}
