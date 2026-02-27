#include "commonlibs/sleeprelative.hpp"
#include <gtest/gtest.h>
#include <chrono>

// ---------------------------------------------------------------------------
// systemtime_tools helpers
// ---------------------------------------------------------------------------

TEST(SystemTimeTools, GetSystemTime_IsCloseToNow)
{
    auto before = std::chrono::system_clock::now();
    auto t      = commonlibs::systemtime_tools::get_system_time();
    auto after  = std::chrono::system_clock::now();

    EXPECT_GE(t, before);
    EXPECT_LE(t, after);
}

TEST(SystemTimeTools, GetNextSystemTimeClick_AdvancesByExactMs)
{
    auto base = commonlibs::systemtime_tools::get_system_time();
    auto next = commonlibs::systemtime_tools::get_next_system_time_click(base, 250);

    auto diff_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                       next - base).count();
    EXPECT_EQ(250, diff_ms);
}

TEST(SystemTimeTools, GetNextSystemTimeClick_ZeroMs_EqualsToPrevious)
{
    auto base = commonlibs::systemtime_tools::get_system_time();
    auto next = commonlibs::systemtime_tools::get_next_system_time_click(base, 0);
    EXPECT_EQ(base, next);
}

TEST(SystemTimeTools, GetNextSystemTimeClick_NegativeMs_IsInThePast)
{
    auto base = commonlibs::systemtime_tools::get_system_time();
    auto prev = commonlibs::systemtime_tools::get_next_system_time_click(base, -100);
    EXPECT_LT(prev, base);
}

TEST(SystemTimeTools, SleepMsUntil_PastTimeReturnsImmediately)
{
    // A time 1 second in the past should cause sleep_until to return at once.
    auto past = std::chrono::system_clock::now() - std::chrono::seconds(1);

    auto wall_before = std::chrono::steady_clock::now();
    commonlibs::systemtime_tools::sleep_ms_until(past);
    auto elapsed_ms  = std::chrono::duration_cast<std::chrono::milliseconds>(
                           std::chrono::steady_clock::now() - wall_before).count();

    EXPECT_LT(elapsed_ms, 100);   // should be essentially instant
}

// ---------------------------------------------------------------------------
// sleep_relative_ms
// ---------------------------------------------------------------------------

TEST(SleepRelative, SleeperWaitsAtLeastRequestedMs)
{
    auto before = std::chrono::steady_clock::now();
    commonlibs::sleep_relative_ms(50);   // construct → sleeps 50 ms
    auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                          std::chrono::steady_clock::now() - before).count();

    EXPECT_GE(elapsed_ms, 50);
}

TEST(SleepRelative, SleeperDoesNotSleepTooLong)
{
    auto before = std::chrono::steady_clock::now();
    commonlibs::sleep_relative_ms(10);
    auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                          std::chrono::steady_clock::now() - before).count();

    // Under normal conditions this should complete well within 2 seconds.
    EXPECT_LT(elapsed_ms, 2000);
}

TEST(SleepRelative, ZeroMs_ReturnsQuickly)
{
    auto before = std::chrono::steady_clock::now();
    commonlibs::sleep_relative_ms(0);
    auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                          std::chrono::steady_clock::now() - before).count();

    EXPECT_LT(elapsed_ms, 100);
}
