#pragma once
// Replaces boost::posix_time / boost::gregorian with std::chrono (C++11)
// and std::put_time for string formatting (C++11).
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <string>

namespace commonlibs {

using sys_time = std::chrono::system_clock::time_point;

// ---- epoch-relative arithmetic --------------------------------------------

struct seconds_since_epoch_from_posixtime {
    int64_t operator()(const sys_time &tp) const {
        return std::chrono::duration_cast<std::chrono::seconds>(
            tp.time_since_epoch()).count();
    }
};

struct milliseconds_since_epoch_from_posixtime {
    int64_t operator()(const sys_time &tp) const {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            tp.time_since_epoch()).count();
    }
};

struct utc_tools {
    static int64_t seconds_since_epoch_from_now() {
        return std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
    }
    static int64_t milliseconds_since_epoch_from_now() {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
    }
};

// ---- UTC ↔ local conversion -----------------------------------------------

struct utc_to_local {
    // Shifts a UTC time_point by the current local UTC offset so that the
    // resulting time_point, when treated as UTC, reads as local time.
    // Note: depends on the machine TZ setting.
    sys_time operator()(const sys_time &tp) const { return to_local(tp); }

    static sys_time to_local(const sys_time &tp) {
        std::time_t t = std::chrono::system_clock::to_time_t(tp);
        // mktime interprets the broken-down time as local, giving us the
        // difference between UTC and local wall-clock for that instant.
        std::tm utc_tm  = {};
        std::tm local_tm = {};
#ifdef _WIN32
        gmtime_s(&utc_tm,   &t);
        localtime_s(&local_tm, &t);
#else
        gmtime_r(&t,    &utc_tm);
        localtime_r(&t, &local_tm);
#endif
        std::time_t utc_t   = std::mktime(&utc_tm);
        std::time_t local_t = std::mktime(&local_tm);
        auto offset = std::chrono::seconds(local_t - utc_t);
        return tp + offset;
    }
};

// ---- Time-point from milliseconds since epoch -----------------------------

struct ptime_from_milliseconds_since_epoch {
    static sys_time get_ptime(int64_t totalms) {
        return sys_time{std::chrono::milliseconds(totalms)};
    }

    static sys_time get_ptime_local(int64_t totalms) {
        return utc_to_local::to_local(get_ptime(totalms));
    }

    // ISO basic format: "20230101T120000"
    static std::string get_iso_string(int64_t totalms) {
        return format(get_ptime(totalms), "%Y%m%dT%H%M%S");
    }

    // Human-readable UTC: "2023-01-01 12:00:00"
    static std::string get_simple_string(int64_t totalms) {
        return format(get_ptime(totalms), "%Y-%m-%d %H:%M:%S");
    }

    static std::string get_iso_string_local(int64_t totalms) {
        return format(get_ptime_local(totalms), "%Y%m%dT%H%M%S");
    }

    static std::string get_simple_string_local(int64_t totalms) {
        return format(get_ptime_local(totalms), "%Y-%m-%d %H:%M:%S");
    }

private:
    static std::string format(const sys_time &tp, const char *fmt) {
        std::time_t t = std::chrono::system_clock::to_time_t(tp);
        std::tm tm_   = {};
#ifdef _WIN32
        gmtime_s(&tm_, &t);
#else
        gmtime_r(&t, &tm_);
#endif
        std::ostringstream oss;
        oss << std::put_time(&tm_, fmt);
        return oss.str();
    }
};

} // namespace commonlibs
