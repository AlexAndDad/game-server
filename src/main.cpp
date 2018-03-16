//
// Created by Richard Hodges on 15/03/2018.
//

#include "config.hpp"
#include "error_code.hpp"
#include "connection_manager.hpp"
#include <memory>
#include <istream>


namespace game {

    using protocol = asio::ip::tcp;


}

void log_exception(const std::exception& e, int level =  0)
{
    BOOST_LOG_TRIVIAL(fatal) << std::string(level, ' ') << "exception: " << e.what() << '\n';
    try {
        std::rethrow_if_nested(e);
    } catch(const std::exception& e) {
        log_exception(e, level+1);
    } catch(...) {}
}

void run()
{
    using namespace game;
    BOOST_LOG_TRIVIAL(info) << "program start";

    asio::io_context executor;
    connection_manager manager(executor);
    manager.start();


    asio::signal_set signals(executor);
    signals.add(SIGINT);
    signals.async_wait([&](asio::error_code ec, int sig)
                       {
                           if (sig == SIGINT) {
                               BOOST_LOG_TRIVIAL(info) << "SIGINT received";
                           } else {
                               BOOST_LOG_TRIVIAL(error) << "unexpected signal: " << sig;
                           }
                           manager.cancel();
                       });


    executor.run();
}

int main(int argc, const char **argv)
{
    try{
        run();
    }
    catch (std::exception& e) {
        log_exception(e);
        return 100;
    }

    return 0;
}


