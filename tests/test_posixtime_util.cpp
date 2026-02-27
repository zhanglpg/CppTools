#include "commonlibs/posixtime_util.hpp"
#include <gtest/gtest.h>
#include <chrono>
#include <string>

// 2000-01-01 00:00:00 UTC = 946684800 seconds since the Unix epoch.
// Using UTC avoids any local-timezone ambiguity.
static const long long kEpochS  = 946684800LL;
static const long long kEpochMs = kEpochS * 1000LL;

static const commonlibs::time_point kKnownTP =
    std::chrono::system_clock::time_point(std::chrono::milliseconds(kEpochMs));

// ---------------------------------------------------------------------------
// to_iso_string / to_simple_string
// ---------------------------------------------------------------------------

TEST(PosixTimeUtil, ToIsoString_KnownUtcTime)
{
    EXPECT_EQ("20000101T000000", commonlibs::to_iso_string(kKnownTP));
}

TEST(PosixTimeUtil, ToSimpleString_KnownUtcTime)
{
    EXPECT_EQ("2000-Jan-01 00:00:00", commonlibs::to_simple_string(kKnownTP));
}

TEST(PosixTimeUtil, ToIsoString_EpochOrigin)
{
    auto epoch = std::chrono::system_clock::time_point{};
    EXPECT_EQ("19700101T000000", commonlibs::to_iso_string(epoch));
}

TEST(PosixTimeUtil, ToSimpleString_EpochOrigin)
{
    auto epoch = std::chrono::system_clock::time_point{};
    EXPECT_EQ("1970-Jan-01 00:00:00", commonlibs::to_simple_string(epoch));
}

TEST(PosixTimeUtil, ToIsoString_HasCorrectLength)
{
    // "YYYYMMDDTHHmmss" = 15 characters
    EXPECT_EQ(15u, commonlibs::to_iso_string(kKnownTP).size());
}

// ---------------------------------------------------------------------------
// seconds_since_epoch_from_posixtime
// ---------------------------------------------------------------------------

TEST(PosixTimeUtil, SecondsSinceEpoch_KnownTime)
{
    commonlibs::seconds_since_epoch_from_posixtime f;
    EXPECT_EQ(kEpochS, f(kKnownTP));
}

TEST(PosixTimeUtil, SecondsSinceEpoch_Epoch)
{
    commonlibs::seconds_since_epoch_from_posixtime f;
    auto epoch = std::chrono::system_clock::time_point{};
    EXPECT_EQ(0LL, f(epoch));
}

TEST(PosixTimeUtil, SecondsSinceEpoch_OneSecondLater)
{
    commonlibs::seconds_since_epoch_from_posixtime f;
    auto tp = kKnownTP + std::chrono::seconds(1);
    EXPECT_EQ(kEpochS + 1, f(tp));
}

// ---------------------------------------------------------------------------
// milliseconds_since_epoch_from_posixtime
// ---------------------------------------------------------------------------

TEST(PosixTimeUtil, MillisecondsSinceEpoch_KnownTime)
{
    commonlibs::milliseconds_since_epoch_from_posixtime f;
    EXPECT_EQ(kEpochMs, f(kKnownTP));
}

TEST(PosixTimeUtil, MillisecondsSinceEpoch_FractionalSecond)
{
    commonlibs::milliseconds_since_epoch_from_posixtime f;
    auto tp = kKnownTP + std::chrono::milliseconds(500);
    EXPECT_EQ(kEpochMs + 500LL, f(tp));
}

TEST(PosixTimeUtil, MillisecondsSinceEpoch_Epoch)
{
    commonlibs::milliseconds_since_epoch_from_posixtime f;
    auto epoch = std::chrono::system_clock::time_point{};
    EXPECT_EQ(0LL, f(epoch));
}

// ---------------------------------------------------------------------------
// utc_tools
// ---------------------------------------------------------------------------

TEST(PosixTimeUtil, UtcTools_SecondsNow_AfterYear2000)
{
    auto s = commonlibs::utc_tools::seconds_since_epoch_from_now();
    EXPECT_GT(s, kEpochS);   // after 2000-01-01
}

TEST(PosixTimeUtil, UtcTools_MillisecondsNow_Consistent)
{
    auto s  = commonlibs::utc_tools::seconds_since_epoch_from_now();
    auto ms = commonlibs::utc_tools::milliseconds_since_epoch_from_now();
    // ms count must be at least 1000 × s count
    EXPECT_GE(ms, s * 1000LL);
    // and at most 1 second more
    EXPECT_LT(ms, (s + 1) * 1000LL);
}

TEST(PosixTimeUtil, UtcTools_SecondsMonotonicallyNonDecreasing)
{
    auto s1 = commonlibs::utc_tools::seconds_since_epoch_from_now();
    auto s2 = commonlibs::utc_tools::seconds_since_epoch_from_now();
    EXPECT_LE(s1, s2);
}

// ---------------------------------------------------------------------------
// utc_to_local (functor and static)
// ---------------------------------------------------------------------------

TEST(PosixTimeUtil, UtcToLocal_FunctorNoCrash)
{
    commonlibs::utc_to_local conv;
    EXPECT_NO_THROW({ auto t = conv(kKnownTP); (void)t; });
}

TEST(PosixTimeUtil, UtcToLocal_StaticNoCrash)
{
    EXPECT_NO_THROW({
        auto t = commonlibs::utc_to_local::to_local(kKnownTP);
        (void)t;
    });
}

TEST(PosixTimeUtil, UtcToLocal_FunctorAndStaticAgree)
{
    commonlibs::utc_to_local conv;
    auto via_functor = conv(kKnownTP);
    auto via_static  = commonlibs::utc_to_local::to_local(kKnownTP);
    EXPECT_EQ(via_functor, via_static);
}

// ---------------------------------------------------------------------------
// ptime_from_milliseconds_since_epoch
// ---------------------------------------------------------------------------

TEST(PosixTimeUtil, PtimeFromMs_GetPtime_RoundTrip)
{
    auto tp = commonlibs::ptime_from_milliseconds_since_epoch::get_ptime(kEpochMs);
    commonlibs::milliseconds_since_epoch_from_posixtime f;
    EXPECT_EQ(kEpochMs, f(tp));
}

TEST(PosixTimeUtil, PtimeFromMs_GetIsoString_KnownTime)
{
    auto s = commonlibs::ptime_from_milliseconds_since_epoch::get_iso_string(kEpochMs);
    EXPECT_EQ("20000101T000000", s);
}

TEST(PosixTimeUtil, PtimeFromMs_GetSimpleString_KnownTime)
{
    auto s = commonlibs::ptime_from_milliseconds_since_epoch::get_simple_string(kEpochMs);
    EXPECT_EQ("2000-Jan-01 00:00:00", s);
}

TEST(PosixTimeUtil, PtimeFromMs_GetPtimeLocal_NoCrash)
{
    EXPECT_NO_THROW({
        auto t = commonlibs::ptime_from_milliseconds_since_epoch::get_ptime_local(kEpochMs);
        (void)t;
    });
}

TEST(PosixTimeUtil, PtimeFromMs_GetSimpleStringLocal_NoCrash)
{
    EXPECT_NO_THROW({
        auto s = commonlibs::ptime_from_milliseconds_since_epoch::get_simple_string_local(kEpochMs);
        EXPECT_FALSE(s.empty());
    });
}

TEST(PosixTimeUtil, PtimeFromMs_GetIsoStringLocal_NoCrash)
{
    EXPECT_NO_THROW({
        auto s = commonlibs::ptime_from_milliseconds_since_epoch::get_iso_string_local(kEpochMs);
        EXPECT_EQ(15u, s.size());
    });
}
