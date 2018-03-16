//
// Created by Richard Hodges on 15/03/2018.
//

// General configuration for the entire program

#pragma once

#include <boost/asio.hpp>
#include <boost/system/error_code.hpp>
#include <boost/log/trivial.hpp>
#include <chrono>

namespace game
{
    namespace asio
    {
        using namespace boost::asio;
        using error_code = boost::system::error_code;
    }

    using namespace std::literals;
    using client_connection_protocol = asio::ip::tcp;

}


