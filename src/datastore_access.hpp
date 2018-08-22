/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/

#pragma once

#include <string>

/*! \class DatastoreAccess
 *     \brief Abstract file accessing a datastore
 */


class DatastoreAccess {
public:
    virtual ~DatastoreAccess();
    virtual std::string getItem(PATH?? xpath) = 0;
    virtual void setLeaf(PATH?? path, VALUE?? value) = 0;
    virtual void createPresenceContainer(PATH?? path, VALUE?? value) = 0;
    virtual void deletePresenceContainer(PATH?? path, VALUE?? value) = 0;
};
