//
// Created by rhodges on 19/03/18.
//

#include "connection_manager.hpp"

namespace game {

    game::connection_manager::connection_manager(asio::io_context &executor)
        : strand_(executor)
        , acceptor_(executor)
        , comms_timeout_(0ms)
    {

    }

    void connection_manager::start()
    {
        acceptor_.open(protocol::v4());
        acceptor_.set_option(protocol::socket::reuse_address());
        auto server_endpoint = protocol::endpoint(asio::ip::address_v4(0), 4000);
        acceptor_.bind(server_endpoint);
        acceptor_.listen();
        start_accepting();
    }

}