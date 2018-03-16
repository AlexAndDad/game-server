//
// Created by Richard Hodges on 16/03/2018.
//

#pragma once
#include <config.hpp>

namespace game {

    inline bool is_error(asio::error_code const& ec)
    {
        return ec != asio::error_code();
    }
}
