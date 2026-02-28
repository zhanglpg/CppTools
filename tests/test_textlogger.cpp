#include "commonlibs/textlogger.hpp"
#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include <thread>

class TextLoggerTest : public ::testing::Test {
protected:
    std::filesystem::path tmpdir;

    void SetUp() override {
        tmpdir = std::filesystem::temp_directory_path() / "cpptools_logger_test";
        std::filesystem::remove_all(tmpdir);
        std::filesystem::create_directories(tmpdir);
    }

    void TearDown() override {
        std::filesystem::remove_all(tmpdir);
    }

    int count_log_files() {
        int count = 0;
        for (auto &p : std::filesystem::directory_iterator(tmpdir))
            if (p.path().extension() == ".log") ++count;
        return count;
    }

    std::string read_first_log() {
        for (auto &p : std::filesystem::directory_iterator(tmpdir)) {
            if (p.path().extension() == ".log") {
                std::ifstream ifs(p.path());
                return std::string((std::istreambuf_iterator<char>(ifs)),
                                   std::istreambuf_iterator<char>());
            }
        }
        return {};
    }
};

// ---------------------------------------------------------------------------
// Logging disabled
// ---------------------------------------------------------------------------

TEST_F(TextLoggerTest, LoggingDisabled_ReturnsZero)
{
    commonlibs::log_control lc(false, tmpdir.string(), 3600);
    EXPECT_EQ(0, lc.log_string("test message"));
}

TEST_F(TextLoggerTest, LoggingDisabled_CreatesNoFile)
{
    commonlibs::log_control lc(false, tmpdir.string(), 3600);
    lc.log_string("test message");
    lc.close_all();
    EXPECT_EQ(0, count_log_files());
}

// ---------------------------------------------------------------------------
// Invalid directory
// ---------------------------------------------------------------------------

TEST_F(TextLoggerTest, NonExistentDirectory_ReturnsMinus1)
{
    commonlibs::log_control lc(true, "/nonexistent/path/xyz", 3600);
    EXPECT_EQ(-1, lc.log_string("test message"));
}

TEST_F(TextLoggerTest, DirectoryIsFile_ReturnsMinus1)
{
    // Create a regular file where a directory is expected
    auto filepath = tmpdir / "notadir";
    std::ofstream(filepath) << "x";

    commonlibs::log_control lc(true, filepath.string(), 3600);
    EXPECT_EQ(-1, lc.log_string("test message"));
}

// ---------------------------------------------------------------------------
// Basic logging
// ---------------------------------------------------------------------------

TEST_F(TextLoggerTest, LoggingToValidDirectory_CreatesFile)
{
    commonlibs::log_control lc(true, tmpdir.string(), 3600, false, "test", ".txt");
    EXPECT_EQ(0, lc.log_string("hello"));
    lc.close_all();
    EXPECT_EQ(1, count_log_files());
}

TEST_F(TextLoggerTest, LogFileContainsWrittenString)
{
    commonlibs::log_control lc(true, tmpdir.string(), 3600, false, "test", ".txt");
    lc.log_string("hello world");
    lc.close_all();

    EXPECT_EQ("hello world", read_first_log());
}

TEST_F(TextLoggerTest, MultipleWritesSameFile)
{
    commonlibs::log_control lc(true, tmpdir.string(), 3600, false, "test", ".txt");
    lc.log_string("aaa");
    lc.log_string("bbb");
    lc.close_all();

    EXPECT_EQ(1, count_log_files());
    EXPECT_EQ("aaabbb", read_first_log());
}

TEST_F(TextLoggerTest, LogString_CharPointerOverload)
{
    commonlibs::log_control lc(true, tmpdir.string(), 3600, false, "test", ".txt");
    const char *msg = "raw chars";
    EXPECT_EQ(0, lc.log_string(msg, 9));
    lc.close_all();
    EXPECT_EQ("raw chars", read_first_log());
}

// ---------------------------------------------------------------------------
// File naming: prefix and extension appear in the filename
// ---------------------------------------------------------------------------

TEST_F(TextLoggerTest, FilenameContainsPrefix)
{
    commonlibs::log_control lc(true, tmpdir.string(), 3600, false, "myprefix", ".csv");
    lc.log_string("data");
    lc.close_all();

    for (auto &p : std::filesystem::directory_iterator(tmpdir)) {
        std::string name = p.path().filename().string();
        EXPECT_NE(std::string::npos, name.find("myprefix"));
        EXPECT_NE(std::string::npos, name.find(".csv"));
        return;
    }
    FAIL() << "No log file found";
}

// ---------------------------------------------------------------------------
// close_all resets state, allowing a new log file to be created
// ---------------------------------------------------------------------------

TEST_F(TextLoggerTest, CloseAll_AllowsNewFile)
{
    commonlibs::log_control lc(true, tmpdir.string(), 3600, false, "test", ".txt");
    lc.log_string("first");
    lc.close_all();

    // Small delay to ensure different timestamp in filename
    std::this_thread::sleep_for(std::chrono::milliseconds(1100));

    lc.log_string("second");
    lc.close_all();

    EXPECT_EQ(2, count_log_files());
}

// ---------------------------------------------------------------------------
// Append mode
// ---------------------------------------------------------------------------

TEST_F(TextLoggerTest, AppendMode_WritesData)
{
    commonlibs::log_control lc(true, tmpdir.string(), 3600, false, "test", ".txt",
                                std::ios_base::app);
    lc.log_string("appended");
    lc.close_all();
    EXPECT_EQ("appended", read_first_log());
}

// ---------------------------------------------------------------------------
// Extra ID parameter
// ---------------------------------------------------------------------------

TEST_F(TextLoggerTest, UseIdAppearsInFilename)
{
    commonlibs::log_control lc(true, tmpdir.string(), 3600, false, "pfx", ".ext");
    lc.log_string(std::string("data"), 42, true);
    lc.close_all();

    for (auto &p : std::filesystem::directory_iterator(tmpdir)) {
        std::string name = p.path().filename().string();
        EXPECT_NE(std::string::npos, name.find("42"));
        return;
    }
    FAIL() << "No log file found";
}
