/*
 * Copyright (C) 2020 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
*/
#pragma once
#include <variant>
#include "list_instance.hpp"

namespace yang::move {
enum class Absolute {
    Begin,
    End
};
struct Relative {
    bool operator==(const yang::move::Relative& other) const
    {
        return this->m_position == other.m_position && this->m_path == other.m_path;
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
