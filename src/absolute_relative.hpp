/*
 * Copyright (C) 2020 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
*/
#pragma once
#include <variant>
#include "list_instance.hpp"

enum class Absolute {
    Begin,
    End
};
struct Relative {
    bool operator==(const Relative&) const;
    enum class Position {
        Before,
        After
    } m_position;
    ListInstance m_path;
};
