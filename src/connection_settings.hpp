//
// Created by rhodges on 16/03/18.
//

#pragma once

#include "config.hpp"

namespace game
{
    struct connection_settings
    {
        constexpr connection_settings(std::chrono::milliseconds inactivity_timeout = 0ms)
            : inactivity_timeout_(inactivity_timeout)
        {

        }

        constexpr bool should_timeout() const { return inactivity_timeout() > 0ms; }

        constexpr auto inactivity_timeout() const -> std::chrono::milliseconds
        {
            return inactivity_timeout_;
        }

    private:
        std::chrono::milliseconds inactivity_timeout_;
    };


    auto operator<<(std::ostream& os, connection_settings const& arg) -> std::ostream&;
}