//
// Created by Richard Hodges on 15/03/2018.
//

#include "config.hpp"
#include "error_code.hpp"

#include <memory>
#include <istream>


namespace game {

    using protocol = asio::ip::tcp;

    struct connection
        : std::enable_shared_from_this<connection>
    {
        connection(asio::io_context& executor)
            : strand_(executor)
            , socket_(executor)
            , timer_(executor)
        {

        }

        auto get_socket() -> protocol::socket& { return socket_; }

        void poke_timer()
        {
            timer_.expires_from_now(boost::posix_time::seconds(3));
            auto handler = [self = shared_from_this()](asio::error_code ec)
            {
                self->handle_timeoout(ec);
            };

            timer_.async_wait(asio::bind_executor(strand_, handler));
        }

        void handle_timeoout(asio::error_code ec)
        {
            if (is_error(ec)) {
                if (ec == asio::error::operation_aborted) {
                    // do nothing - this is because we extended the timer
                }
                else {
                    BOOST_LOG_TRIVIAL(error) << "fatal timer error: " << ec.message();
                    socket_.cancel();
                }
            }
            else {
                BOOST_LOG_TRIVIAL(info) << "connection timeout";
                socket_.cancel();
            }
        }

        void handle_read(asio::error_code ec, std::size_t bytes)
        {
            if (is_error(ec)) {
                BOOST_LOG_TRIVIAL(error) << "read error: " << ec.message();
            }
            else {
                poke_timer();
                std::istream is(&this->rxbuf_);
                std::string  s;
                std::getline(is, s);
                BOOST_LOG_TRIVIAL(info) << "received: " << s;
                initate_read();
            }
        }

        void initate_read()
        {
            auto read_handler = [self = shared_from_this()](asio::error_code ec, std::size_t size)
            {
                self->handle_read(ec, size);
            };

            asio::async_read_until(socket_, rxbuf_, '\n', asio::bind_executor(strand_, read_handler));
        }

        void run()
        {
            BOOST_LOG_TRIVIAL(info) << "connection start";
            poke_timer();
            initate_read();
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
    };

    void start_accepting(protocol::acceptor& acceptor)
    {
        auto& executor = acceptor.get_io_context();
        auto connection_candidate = std::make_shared<connection>(executor);
        auto accept_handler       = [connection_candidate](asio::error_code ec)
        {
            if (is_error(ec)) {
                BOOST_LOG_TRIVIAL(info) << "acceptor error: " << ec.message();
            }
            else {
                connection_candidate->run();
            }

        };

        acceptor.async_accept(connection_candidate->get_socket(), accept_handler);
    }
}

int main(int argc, const char **argv)
{
    using namespace game;
    BOOST_LOG_TRIVIAL(info) << "program start";

    asio::io_context   executor;
    protocol::acceptor acceptor(executor);
    acceptor.open(protocol::v4());
    auto server_endpoint = protocol::endpoint(asio::ip::address_v4(0), 4000);
    acceptor.bind(server_endpoint);
    acceptor.listen();
    start_accepting(acceptor);

    executor.run();


    return 0;
}