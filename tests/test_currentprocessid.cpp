// Include <unistd.h> BEFORE currentprocessid.hpp so that POSIX symbols
// are declared at global scope rather than being pulled into commonlibs.
#include <unistd.h>
#include "commonlibs/currentprocessid.hpp"
#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>

// ---------------------------------------------------------------------------
// getprocessid
// ---------------------------------------------------------------------------

TEST(CurrentProcessId, GetPidReturnsPositive)
{
    unsigned int pid = commonlibs::getprocessid::get_current_process_id();
    EXPECT_GT(pid, 0u);
}

TEST(CurrentProcessId, GetPidMatchesGetpid)
{
    unsigned int pid = commonlibs::getprocessid::get_current_process_id();
    EXPECT_EQ(static_cast<unsigned int>(::getpid()), pid);
}

TEST(CurrentProcessId, CmdrmReturnsRmOnLinux)
{
    std::string rm = commonlibs::getprocessid::cmdrm();
    EXPECT_EQ("rm -f ", rm);
}

// ---------------------------------------------------------------------------
// commonlibs::to_string helper
// ---------------------------------------------------------------------------

TEST(CurrentProcessId, ToStringConvertsInt)
{
    EXPECT_EQ("42", commonlibs::to_string(42));
    EXPECT_EQ("0", commonlibs::to_string(0));
    EXPECT_EQ("-1", commonlibs::to_string(-1));
}

TEST(CurrentProcessId, ToStringConvertsDouble)
{
    std::string result = commonlibs::to_string(3.14);
    EXPECT_FALSE(result.empty());
    EXPECT_NE(std::string::npos, result.find("3.14"));
}

// ---------------------------------------------------------------------------
// pidsaver
// ---------------------------------------------------------------------------

TEST(PidSaver, CreatesAndDeletesPidFile)
{
    auto tmpdir = std::filesystem::temp_directory_path() / "cpptools_pid_test";
    std::filesystem::remove_all(tmpdir);
    std::filesystem::create_directories(tmpdir);

    unsigned int expected_pid = static_cast<unsigned int>(::getpid());
    auto pidfile = tmpdir / std::to_string(expected_pid);

    {
        commonlibs::pidsaver ps(tmpdir.string());

        // PID file should now exist
        EXPECT_TRUE(std::filesystem::exists(pidfile))
            << "Expected PID file at " << pidfile;

        // File content should contain the PID
        std::ifstream ifs(pidfile);
        unsigned int stored_pid = 0;
        ifs >> stored_pid;
        EXPECT_EQ(expected_pid, stored_pid);
    }
    // After pidsaver destructor, file should be deleted
    EXPECT_FALSE(std::filesystem::exists(pidfile));

    std::filesystem::remove_all(tmpdir);
}

TEST(PidSaver, CustomFilename)
{
    auto tmpdir = std::filesystem::temp_directory_path() / "cpptools_pid_test2";
    std::filesystem::remove_all(tmpdir);
    std::filesystem::create_directories(tmpdir);

    unsigned int expected_pid = static_cast<unsigned int>(::getpid());
    auto customfile = tmpdir / "myapp.pid";

    {
        commonlibs::pidsaver ps(tmpdir.string(), "myapp.pid");

        EXPECT_TRUE(std::filesystem::exists(customfile))
            << "Expected PID file at " << customfile;

        std::ifstream ifs(customfile);
        unsigned int stored_pid = 0;
        ifs >> stored_pid;
        EXPECT_EQ(expected_pid, stored_pid);
    }
    // Note: destructor removes s_piddir/to_string(pid), not the custom filename.
    // This is a known quirk of the implementation.

    std::filesystem::remove_all(tmpdir);
}

TEST(PidSaver, EmptyDirDoesNotCrash)
{
    EXPECT_NO_THROW({
        commonlibs::pidsaver ps("");
    });
}
