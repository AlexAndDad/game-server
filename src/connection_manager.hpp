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

        connection_manager(asio::io_context &executor);

        void start();

        void cancel()
        {
            acceptor_.cancel();
            strand_.dispatch([this] { this->cancel_connections(); });
        }

        template<class Handler>
        auto wrap(Handler &&handler)
        {
            return asio::bind_executor(strand_, std::forward<Handler>(handler));
        }

        struct connection_deleter
        {
            connection_deleter(connection_manager &manager) : manager_(manager) {}

            void operator()(connection *pconn) const noexcept
            {
                manager_.notify_connection_deleted(pconn);
                delete pconn;
            }

            connection_manager &manager_;
        };

        void start_accepting()
        {
            auto &executor = get_executor();
            auto connection_candidate = connection::create_shared(executor, connection_deleter(*this));
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
                    connection_candidate->start(make_connection_settings());
                    this->start_accepting();
                }
            };

            acceptor_.async_accept(connection_candidate->get_socket(), wrap(accept_handler));
        }

        auto get_executor() -> asio::io_context &
        {
            return strand_.get_io_context();
        }

        using connection_handle = connection *;

        void notify_connection_deleted(connection_handle handle)
        {
            strand_.dispatch([this, handle] { this->erase_connection(handle); });
        }

    private:

        void notify_connected(std::shared_ptr<connection> const &ptr)
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
            for (auto &&cache_entry : connections_) {
                if (auto p = cache_entry.second.lock()) {
                    active_connections.push_back(std::move(p));
                }
            }

            for (auto &&p : active_connections) {
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


/*
#include <thread>

std::string read_line_with_timeout(boost::asio::ip::tcp::socket &sock, boost::asio::streambuf &buf)
{
    namespace asio =  boost::asio;

    // these statics could of course be encapsulated into a service object
    static asio::io_context executor;
    static asio::io_context::work work(executor);
    static std::thread mythread{[&] { executor.start(); }};

    auto temp_socket = asio::generic::stream_protocol::socket(executor,
                                                              sock.local_endpoint().protocol(),
                                                              dup(sock.native_handle()));
    auto timer = asio::deadline_timer(executor, boost::posix_time::milliseconds(3000));

    std::condition_variable cv;
    std::mutex m;

    int done_count = 0;
    boost::system::error_code err;

    auto get_lock = [&] { return std::unique_lock<std::mutex>(m); };

    auto aborted = [](boost::system::error_code const &ec) { return ec == boost::asio::error::operation_aborted; };

    auto common_handler = [&](auto ec)
    {
        if (not aborted(ec))
        {
            auto lock = get_lock();
            if (done_count++ == 0) {
                err = ec;
                boost::system::error_code sink;
                temp_socket.cancel(sink);
                timer.cancel(sink);
            }
            lock.unlock();
            cv.notify_one();
        }
    };

    async_read_until(temp_socket, buf, '\n', [&](auto ec, auto&&...) { common_handler(ec); });
    timer.async_wait([&](auto ec)
                     {
                         common_handler(ec ? ec : asio::error::timed_out);
                     });

    auto lock = get_lock();
    cv.wait(lock, [&] { return done_count == 2; });

    if (err) throw boost::system::system_error(err);
    std::istream is(&buf);
    std::string result;
    std::getline(is, result);
    return result;
}
*/
