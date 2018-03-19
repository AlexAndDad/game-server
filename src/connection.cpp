//
// Created by rhodges on 19/03/18.
//

#include "connection.hpp"

namespace game {
    connection::connection(asio::io_context &executor)
        : strand_(executor)
        , socket_(executor)
        , timer_(executor)
    {

    }

    void connection::poke_timer()
    {
        assert(strand_.running_in_this_thread());

        if (settings_.should_timeout()) {
            timer_.expires_from_now(boost::posix_time::seconds(3));
            auto handler = [self = shared_from_this()](asio::error_code ec)
            {
                self->handle_timeoout(ec);
            };

            timer_.async_wait(asio::bind_executor(strand_, handler));
        }
    }

    void connection::handle_timeoout(asio::error_code ec)
    {
        assert(strand_.running_in_this_thread());

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

    void connection::handle_read(asio::error_code ec, std::size_t bytes)
    {
        assert(strand_.running_in_this_thread());

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

    void connection::initate_read()
    {
        assert(strand_.running_in_this_thread());

        auto read_handler = [self = shared_from_this()](asio::error_code ec, std::size_t size)
        {
            self->handle_read(ec, size);
        };

        asio::async_read_until(socket_, rxbuf_, '\n', asio::bind_executor(strand_, read_handler));
    }

    void connection::start(connection_settings settings)
    {
        settings_ = settings;
        BOOST_LOG_TRIVIAL(info) << "connection start with settings: " << settings_;

        strand_.dispatch(wrap([this]
                              {
                                  this->poke_timer();
                                  this->initate_read();
                              }));

    }

    void connection::notify_cancel()
    {
        strand_.dispatch(wrap([this]
                              {
                                  this->handle_cancel();
                              }));
    }

    void connection::handle_cancel()
    {
        assert(strand_.running_in_this_thread());

        BOOST_LOG_TRIVIAL(info) << "connection cancelled - shutting down";
        asio::error_code sink;
        timer_.cancel(sink);
        socket_.cancel(sink);
        socket_.shutdown(protocol::socket::shutdown_both, sink);
        socket_.close(sink);
    }


}