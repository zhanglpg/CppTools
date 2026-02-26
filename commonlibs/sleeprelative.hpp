#pragma once
// Replaces boost::thread::sleep / boost::posix_time with std::chrono (C++11).
#include <chrono>
#include <thread>

namespace commonlibs {

struct sleep_relative_ms {
    explicit sleep_relative_ms(int millisecs) {
        std::this_thread::sleep_for(std::chrono::milliseconds(millisecs));
    }
};

struct systemtime_tools {
    using time_point = std::chrono::system_clock::time_point;

    static time_point get_system_time() {
        return std::chrono::system_clock::now();
    }

    static time_point get_next_system_time_click(const time_point &tprevious,
                                                  int milliseconds) {
        return tprevious + std::chrono::milliseconds(milliseconds);
    }

    static void sleep_ms_until(const time_point &ptuntil) {
        std::this_thread::sleep_until(ptuntil);
    }
};

} // namespace commonlibs
