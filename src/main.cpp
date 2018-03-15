//
// Created by Richard Hodges on 15/03/2018.
//

#include "config.hpp"
#include <memory>
#include <istream>


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
        timer_.async_wait(asio::bind_executor(strand_, [self = this->shared_from_this()](asio::error_code ec)
        {
            if (ec == asio::error_code())
            {
                BOOST_LOG_TRIVIAL(info) << "connection timeout";
                self->socket_.cancel();
            }
            else if (ec == asio::error::operation_aborted)
            {
                // nothing to do
            }
            else
            {
                BOOST_LOG_TRIVIAL(error) << "fatal timer error: " << ec.message();
                self->socket_.cancel();
            }
        }));

    }

    void handle_read(asio::error_code ec, std::size_t bytes)
    {
        if (ec == asio::error_code())
        {
            BOOST_LOG_TRIVIAL(info) << "got data";

            poke_timer();

            std::istream is(&this->rxbuf_);
            std::string  s;
            std::getline(is, s);
            BOOST_LOG_TRIVIAL(info) << "received: " << s;
            initate_read();
        }
        else
        {
            BOOST_LOG_TRIVIAL(error) << "read error: " << ec.message();
        }
    }

    void initate_read()
    {
        asio::async_read_until(socket_,
                               rxbuf_,
                               '\n',
                               asio::bind_executor(strand_,
                                                   [self = this->shared_from_this(), this](asio::error_code ec,
                                                                                           std::size_t size)
                                                   {
                                                       this->handle_read(ec, size);
                                                   }));
    }

    void run()
    {
        BOOST_LOG_TRIVIAL(info) << "connection start";

        poke_timer();

        initate_read();
    }

private:

    asio::io_context::strand strand_;
    protocol::socket         socket_;
    asio::deadline_timer     timer_;

    asio::streambuf rxbuf_;
};

void start_accepting(protocol::acceptor& acceptor)
{
    auto& executor = acceptor.get_io_context();
    auto connection_candidate = std::make_shared<connection>(executor);
    acceptor.async_accept(connection_candidate->get_socket(),
                          [connection_candidate](asio::error_code ec)
                          {
                              if (ec != asio::error_code())
                              {
                                  BOOST_LOG_TRIVIAL(info) << "acceptor error: " << ec.message();
                              }
                              else
                              {
                                  connection_candidate->run();
                              }

                          });
}

int main(int argc, const char **argv)
{
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