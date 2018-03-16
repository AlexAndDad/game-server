//
// Created by rhodges on 16/03/18.
//

#pragma once

#include "error_code.hpp"
#include "connection_settings.hpp"

namespace game
{

    struct connection
            : std::enable_shared_from_this<connection> {

        using protocol = client_connection_protocol;

        connection(asio::io_context &executor)
                : strand_(executor), socket_(executor), timer_(executor) {

        }

        auto get_socket() -> protocol::socket & { return socket_; }

        void poke_timer() {
            if (settings_.should_timeout()) {
                timer_.expires_from_now(boost::posix_time::seconds(3));
                auto handler = [self = shared_from_this()](asio::error_code ec)
                {
                    self->handle_timeoout(ec);
                };

                timer_.async_wait(asio::bind_executor(strand_, handler));
            }
        }

        void handle_timeoout(asio::error_code ec) {
            if (is_error(ec)) {
                if (ec == asio::error::operation_aborted) {
                    // do nothing - this is because we extended the timer
                } else {
                    BOOST_LOG_TRIVIAL(error) << "fatal timer error: " << ec.message();
                    socket_.cancel();
                }
            } else {
                BOOST_LOG_TRIVIAL(info) << "connection timeout";
                socket_.cancel();
            }
        }

        void handle_read(asio::error_code ec, std::size_t bytes) {
            if (is_error(ec)) {
                BOOST_LOG_TRIVIAL(error) << "read error: " << ec.message();
            } else {
                poke_timer();
                std::istream is(&this->rxbuf_);
                std::string s;
                std::getline(is, s);
                BOOST_LOG_TRIVIAL(info) << "received: " << s;
                initate_read();
            }
        }

        void initate_read() {
            auto read_handler = [self = shared_from_this()](asio::error_code ec, std::size_t size) {
                self->handle_read(ec, size);
            };

            asio::async_read_until(socket_, rxbuf_, '\n', asio::bind_executor(strand_, read_handler));
        }

        void run(connection_settings settings) {
            settings_ = settings;
            BOOST_LOG_TRIVIAL(info) << "connection start with settings: " << settings_;
            poke_timer();
            initate_read();
        }

        void notify_cancel()
        {
            strand_.post([self = shared_from_this()]
                         {
                             self->handle_cancel();
                         });
        }

    private:
        void handle_cancel()
        {
            BOOST_LOG_TRIVIAL(info) << "connection cancelled - shutting down";
            timer_.cancel();
            socket_.cancel();
        }

    private:

        // synchronises async access to this object
        asio::io_context::strand strand_;

        // the client socket
        protocol::socket socket_;

        // inter-message timer
        asio::deadline_timer timer_;

        // stream buffer for buffering text lines sent by the client
        asio::streambuf rxbuf_;

        connection_settings settings_;
    };

}