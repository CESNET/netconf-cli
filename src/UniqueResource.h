/*
 * Copyright (C) 2017-2019 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Jan Kundrát <jan.kundrat@cesnet.cz>
 *
*/

#pragma once

#include <functional>

class UniqueResource {
public:
    using Func = std::function<void()>;
    UniqueResource(const UniqueResource& other) = delete;
    UniqueResource& operator=(const UniqueResource& other) = delete;
    UniqueResource(UniqueResource&& other) = default;
    ~UniqueResource()
    {
        cleanup();
    }

private:
    Func cleanup;
    UniqueResource(Func&& init, Func&& cleanup)
        : cleanup(std::forward<Func>(cleanup))
    {
        init();
    }
    friend auto make_unique_resource(Func&& init, Func&& cleanup);
};

inline auto make_unique_resource(UniqueResource::Func&& init, UniqueResource::Func&& cleanup)
{
    return UniqueResource(std::move(init), std::move(cleanup));
}
