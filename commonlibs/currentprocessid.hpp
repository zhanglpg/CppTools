#pragma once
#include <string>

#ifdef WIN32
#  include <windows.h>
#else
#  include <unistd.h>
#endif

namespace commonlibs {

class getprocessid {
public:
    static unsigned int get_current_process_id() {
#ifdef WIN32
        return static_cast<unsigned int>(GetCurrentProcessId());
#else
        return static_cast<unsigned int>(getpid());
#endif
    }

    static std::string cmdrm() {
#ifdef WIN32
        return "del /Q ";
#else
        return "rm -f ";
#endif
    }
};

class pidsaver {
public:
    pidsaver(std::string s_pid_dir_, std::string s_filename_ = "")
        : s_piddir(std::move(s_pid_dir_))
    {
        pid = getprocessid::get_current_process_id();
        if (s_filename_.empty())
            s_filename_ = std::to_string(pid);
        if (!s_piddir.empty()) {
            std::string cmd = "echo \"" + std::to_string(pid)
                              + "\" > " + s_piddir + "/" + s_filename_;
            ::system(cmd.c_str());
        }
    }

    ~pidsaver() {
        if (!s_piddir.empty()) {
            std::string cmd = getprocessid::cmdrm()
                              + " " + s_piddir + "/" + std::to_string(pid);
            ::system(cmd.c_str());
        }
    }

private:
    std::string  s_piddir;
    unsigned int pid = 0;
};

} // namespace commonlibs
