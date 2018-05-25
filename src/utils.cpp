/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/
#include "utils.hpp"

std::string joinPaths(const std::string& prefix, const std::string& suffix)
{
    if (prefix.empty() || suffix.empty())
        return prefix + suffix;
    else
        return prefix + '/' + suffix;
}

std::string stripLastNodeFromPath(const std::string& path)
{
    std::string res = path;
    auto pos = res.find_last_of('/');
    if (pos == res.npos)
        res.clear();
    else
        res.erase(pos);
    return res;
}
