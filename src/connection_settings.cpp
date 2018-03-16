//
// Created by rhodges on 16/03/18.
//

#include "connection_settings.hpp"
#include <iomanip>
#include <string_view>

namespace game {

    namespace {

        template<class Duration> struct duration_suffix;


        template<typename Duration>
        struct duration_emitter
        {
            duration_emitter(Duration const& d) : d(d) {}

            Duration const& d;
        };


        template<class Duration>
        auto to_string(duration_emitter<Duration> const &arg) -> std::string
        {
            std::string s = std::to_string(arg.d.count());
            s += duration_suffix<Duration>::get();
            return s;
        }

        template<class Duration>
        auto operator<<(std::ostream &os, duration_emitter<Duration> const &arg) -> std::ostream &
        {
            return os << to_string(arg);
        }

        template<> struct duration_suffix<std::chrono::milliseconds>
        {
            static constexpr auto get() -> std::string_view
            {
                return "ms";
            }
        };

    }

    auto operator<<(std::ostream &os, connection_settings const &arg) -> std::ostream &
    {
        return os << "{ inactivity_timeout: " << std::quoted(to_string(duration_emitter(arg.inactivity_timeout())))
                  << " }";
    }

}