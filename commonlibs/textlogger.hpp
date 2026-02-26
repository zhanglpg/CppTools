#pragma once
// Replaces boost::filesystem with std::filesystem (C++17) and
// boost::posix_time with std::chrono (C++11).
#include <chrono>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

namespace commonlibs {

class log_control {
public:
    log_control(bool b_l_,
                const std::string &s_dirname_,
                unsigned int u_period_secs_,
                bool b_daily = false,
                const std::string &s_nameprefix_ = "",
                const std::string &s_nameext_    = "",
                std::ios_base::openmode omode_   = std::ios_base::out)
        : b_logging(b_l_), b_logstarted(false),
          s_logdir(s_dirname_), s_prefix(s_nameprefix_), s_nameext(s_nameext_),
          b_log_daily(b_daily), u_period_seconds(u_period_secs_),
          ex(0), omode(omode_)
    {}

    ~log_control() { close_all(); }

    int log_string(const std::string &s_pc,
                   unsigned int u_extra_id = 0, bool use_id = false) {
        return log_string(s_pc.c_str(),
                          static_cast<unsigned int>(s_pc.size()),
                          u_extra_id, use_id);
    }

    int log_string(const char *pc, unsigned int u_size,
                   unsigned int u_extra_id = 0, bool use_id = false) {
        if (!b_logging) return 0;

        auto now = std::chrono::system_clock::now();

        std::string s_filenamenew;
        bool b_start_new = false;

        if (!b_logstarted) {
            make_name(now, u_extra_id, use_id, s_filenamenew);
            b_start_new = true;
        } else {
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                now - pt_lastlog).count();
            if (!b_log_daily && elapsed > static_cast<long long>(u_period_seconds)) {
                make_name(now, u_extra_id, use_id, s_filenamenew);
                b_start_new = true;
            } else if (b_log_daily && !same_day(now, pt_lastlog)) {
                make_name(now, u_extra_id, use_id, s_filenamenew);
                b_start_new = true;
            }
        }

        namespace fs = std::filesystem;
        fs::path thepath_(s_logdir);

        if (b_start_new) {
            if (!fs::exists(thepath_))   return -1;
            if (!fs::is_directory(thepath_)) return -1;

            std::string s_filenamenew_log = s_filenamenew + ".log";
            std::string fullfilename_ = (thepath_ / s_filenamenew_log).string();

            if (fs::exists(fullfilename_)) {
                int inc = 0;
                do {
                    s_filenamenew_log = s_filenamenew + "-"
                                        + std::to_string(++ex) + ".log";
                    fullfilename_ = (thepath_ / s_filenamenew_log).string();
                    ++inc;
                } while (fs::exists(fullfilename_) && inc < 100);
                if (inc >= 100) return -1;
            }

            if (ofs.is_open()) ofs.close();
            ofs.open(fullfilename_, omode);
            if (!ofs.is_open()) return -1;

            b_logstarted = true;
            pt_lastlog   = now;
            ofs.write(pc, u_size);
        } else {
            if (ofs.is_open()) ofs.write(pc, u_size);
        }
        return 0;
    }

    void close_all() {
        if (ofs.is_open()) ofs.close();
        b_logstarted = false;
    }

private:
    // ISO-basic timestamp for use in filenames: "20230101T120000"
    static std::string iso_timestamp(const std::chrono::system_clock::time_point &tp) {
        std::time_t t = std::chrono::system_clock::to_time_t(tp);
        std::tm tm_   = {};
#ifdef _WIN32
        localtime_s(&tm_, &t);
#else
        localtime_r(&t, &tm_);
#endif
        std::ostringstream oss;
        oss << std::put_time(&tm_, "%Y%m%dT%H%M%S");
        return oss.str();
    }

    static bool same_day(const std::chrono::system_clock::time_point &a,
                         const std::chrono::system_clock::time_point &b) {
        std::time_t ta = std::chrono::system_clock::to_time_t(a);
        std::time_t tb = std::chrono::system_clock::to_time_t(b);
        std::tm tma = {}, tmb = {};
#ifdef _WIN32
        localtime_s(&tma, &ta);
        localtime_s(&tmb, &tb);
#else
        localtime_r(&ta, &tma);
        localtime_r(&tb, &tmb);
#endif
        return tma.tm_year == tmb.tm_year && tma.tm_yday == tmb.tm_yday;
    }

    void make_name(const std::chrono::system_clock::time_point &now,
                   unsigned int u_extra_id, bool use_id,
                   std::string &s_name) {
        s_name = s_prefix
                 + (use_id ? std::to_string(u_extra_id) : std::string(""))
                 + "_" + iso_timestamp(now)
                 + s_nameext;
    }

    bool         b_logging;
    bool         b_logstarted;
    std::string  s_logdir, s_prefix, s_nameext;
    unsigned int u_period_seconds;
    bool         b_log_daily;
    std::chrono::system_clock::time_point pt_lastlog;
    std::ofstream ofs;
    unsigned int  ex;
    std::ios_base::openmode omode;
};

} // namespace commonlibs
