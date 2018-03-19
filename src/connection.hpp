//
// Created by rhodges on 16/03/18.
//

#pragma once

#include "error_code.hpp"
#include "connection_settings.hpp"

namespace game {

    /// An object which represents a connection by a client.
    /// Objects of this type are designed to be owned by a shared_ptr, as once the start() method has been called,
    /// the object will take shared references to itself while awaiting execution of async completion handlers.
    /// In order to destroy a connection, call cancel() on it. It will be destroyed prior to the owning
    /// executor entering the stopped() state,
    struct connection
        : std::enable_shared_from_this<connection>
    {

        using protocol = client_connection_protocol;

        /// Create a shared_ptr to a connection with the given deleter.
        /// @tparam Deleter is an object which is responsible for deleting the connection at final release.
        /// @param owner is a reference to an io_context on which all async connection operations shall complete.
        /// @param deleter is the deleter which will destroy the connection
        /// @return std::shared_ptr<connection>
        /// @note the returned connection has not been started.
        template<class Deleter>
        static auto create_shared(asio::io_context &owner, Deleter &&deleter) -> std::shared_ptr<connection>;


        auto get_socket() -> protocol::socket & { return socket_; }


        /// Post a notification to the connection that it should start operating.
        /// @pre the connection is not started
        void start(connection_settings settings);

        /// Post a notification to the that the connection should cancel all async operations
        void notify_cancel();

    private:
        /// Construct a connection
        /// @param owner is a reference to an io_context, which must outlive the connection.
        /// @note the constructor is private, forcing construction through the static creation function.
        /// @see connection::create()
        connection(asio::io_context &owner);

        /// Actions the cancel event in the context of the strand
        /// @pre executed from within strand_
        void handle_cancel();

        void poke_timer();

        void handle_timeoout(asio::error_code ec);

        void handle_read(asio::error_code ec, std::size_t bytes);

        void initate_read();

        /// Return a bound handler bound to this object's strand.
        /// @param handler is the handler to wrap
        /// @tparam is the type of the handler, which is deduced.
        /// @return the result of calling asio::bind_executor, which is a specialisation of asio::executor_binder<>
        ///
        template<class Handler>
        auto wrap(Handler &&handler);

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

    // implementation

    template<class Deleter>
    auto connection::create_shared(asio::io_context &owner, Deleter &&deleter) -> std::shared_ptr<connection>
    {
        return std::shared_ptr<connection>(new connection(owner), std::forward<Deleter>(deleter));
    }

    template<class Handler>
    auto connection::wrap(Handler &&handler)
    {
        return asio::bind_executor(strand_,
                                   [self = shared_from_this(), handler = std::forward<Handler>(handler)]
                                       (auto &&...args)
                                   {
                                       std::invoke(handler, std::forward<decltype(args)>(args)...);
                                   });
    }


}