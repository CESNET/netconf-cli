/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/

#include "datastore_access.hpp"

DatastoreAccess::~DatastoreAccess() = default;

DatastoreException::DatastoreException(const std::vector<DatastoreError>& errors)
{
    m_what = "The following errors occured:\n";
    for (auto it : errors) {
        m_what += " Message: ";
        m_what += it.m_message;
        m_what += "\n";
        if (it.m_xpath) {
            m_what += " Xpath: ";
            m_what += it.m_xpath.value();
            m_what += "\n";
        }
    }
}

const char* DatastoreException::what() const noexcept
{
    return m_what.c_str();
}
