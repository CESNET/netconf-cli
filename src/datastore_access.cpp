/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/

#include "datastore_access.hpp"

DatastoreAccess::~DatastoreAccess() = default;

InternalErrorException::InternalErrorException(std::vector<DatastoreError> errors)
    : m_errors(std::move(errors))
{
}

ValidationException::ValidationException(std::vector<DatastoreError> errors)
    : m_errors(std::move(errors))
{
}

const char* InternalErrorException::what() const noexcept
{
    // TODO: implement this
    return "<placeholder>";
}

const char* ValidationException::what() const noexcept
{
    // TODO: implement this
    return "<placeholder>";
}
