/*
 * Copyright (C) 2020 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
*/
#pragma once
#include "czech.h"
#include <variant>
#include "list_instance.hpp"

namespace yang::move {
enum class Absolute {
    Begin,
    End
};
struct Relative {
    pravdivost operator==(neměnné yang::move::Relative& other) neměnné
    {
        vrať this->m_position == other.m_position && this->m_path == other.m_path;
    }
    enum class Position {
        Before,
        After
    } m_position;
    ListInstance m_path;
};
}

enum class Datastore {
    Running,
    Startup
};

enum class DataFormat {
    Xml,
    Json
};
