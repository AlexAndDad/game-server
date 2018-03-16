//
// Created by Richard Hodges on 15/03/2018.
//

// General configuration for the entire program

#pragma once

#include <boost/asio.hpp>
#include <boost/system/error_code.hpp>
#include <boost/log/trivial.hpp>

namespace game
{
    namespace asio
    {
        using namespace boost::asio;
        using error_code = boost::system::error_code;
    }
}


