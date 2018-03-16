//
// Created by rhodges on 16/03/18.
//

#include "error_code.hpp"
#include "connection.hpp"
#include <chrono>
#include <cassert>

namespace game {

    struct connection_manager
    {
        using protocol = client_connection_protocol;

        connection_manager(asio::io_context &executor)
            : strand_(executor)
            , acceptor_(executor)
            , comms_timeout_(0ms)
        {

        }

        void start()
        {
            acceptor_.open(protocol::v4());
            acceptor_.set_option(protocol::socket::reuse_address());
            auto server_endpoint = protocol::endpoint(asio::ip::address_v4(0), 4000);
            acceptor_.bind(server_endpoint);
            acceptor_.listen();
            start_accepting();
        }

        void cancel()
        {
            acceptor_.cancel();
            strand_.dispatch([this]{ this->cancel_connections(); });
        }

        template<class Handler>
        auto wrap(Handler &&handler)
        {
            return asio::bind_executor(strand_, std::forward<Handler>(handler));
        }

        struct connection_deleter
        {
            connection_deleter(connection_manager& manager) : manager_(manager) {}

            void operator()(connection* pconn) const noexcept
            {
                manager_.notify_connection_deleted(pconn);
                delete pconn;
            }

            connection_manager& manager_;
        };

        void start_accepting()
        {
            auto &executor = get_executor();
            auto unique_connection = std::unique_ptr<connection, connection_deleter>(nullptr, connection_deleter(*this));
            unique_connection.reset(new connection(executor));
            auto connection_candidate = std::shared_ptr<connection>(std::move(unique_connection));
            auto accept_handler = [this, connection_candidate](asio::error_code ec)
            {
                if (is_error(ec)) {
                    if (ec == asio::error::operation_aborted) {
                        BOOST_LOG_TRIVIAL(info) << "aborting accept";
                    } else {
                        BOOST_LOG_TRIVIAL(error) << "acceptor error: " << ec.message();
                    }
                } else {
                    this->notify_connected(connection_candidate);
                    connection_candidate->run(make_connection_settings());
                    this->start_accepting();
                }
            };

            acceptor_.async_accept(connection_candidate->get_socket(), wrap(accept_handler));
        }

        auto get_executor() -> asio::io_context &
        {
            return strand_.get_io_context();
        }

        using connection_handle = connection*;

        void notify_connection_deleted(connection_handle handle)
        {
            strand_.dispatch([this, handle] { this->erase_connection(handle); });
        }

    private:

        void notify_connected(std::shared_ptr<connection> const& ptr)
        {
            assert(strand_.running_in_this_thread());
            connections_.emplace(ptr.get(), ptr);
        }

        auto make_connection_settings() const -> connection_settings
        {
            return connection_settings(comms_timeout_);
        }


        void erase_connection(connection_handle handle)
        {
            assert(strand_.running_in_this_thread());
            connections_.erase(handle);
        }

        void cancel_connections()
        {
            assert(strand_.running_in_this_thread());
            std::vector<std::shared_ptr<connection>> active_connections;
            active_connections.reserve(connections_.size());
            for (auto&& cache_entry : connections_)
            {
                if (auto p = cache_entry.second.lock())
                {
                    active_connections.push_back(std::move(p));
                }
            }

            for (auto && p : active_connections)
            {
                p->notify_cancel();
            }
        }



    private:

        asio::io_context::strand strand_;
        protocol::acceptor acceptor_;
        std::chrono::milliseconds comms_timeout_;

        using weak_connection_ptr = std::weak_ptr<connection>;
        using connection_cache = std::unordered_map<connection_handle, weak_connection_ptr>;
        connection_cache connections_;
    };
}