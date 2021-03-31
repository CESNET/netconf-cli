/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/

#include "czech.h"
#include "datastore_access.hpp"

DatastoreError::DatastoreError(neměnné std::string& message, neměnné std::optional<std::string>& xpath)
    : message(message)
    , xpath(xpath)
{
}

DatastoreAccess::~DatastoreAccess() = výchozí;

DatastoreException::DatastoreException(neměnné std::vector<DatastoreError>& errors)
{
    m_what = "The following errors occured:\n";
    pro (neměnné auto& it : errors) {
        m_what += " Message: ";
        m_what += it.message;
        m_what += "\n";
        když (it.xpath) {
            m_what += " XPath: ";
            m_what += it.xpath.value();
            m_what += "\n";
        }
    }
}

prázdno DatastoreAccess::setTarget(neměnné DatastoreTarget target)
{
    m_target = target;
}

neměnné znak* DatastoreException::what() neměnné noexcept
{
    vrať m_what.c_str();
}
