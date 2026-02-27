#include "commonlibs/singleton.hpp"
#include <gtest/gtest.h>

// ---------------------------------------------------------------------------
// A concrete singleton type used for testing.
// The value member lets us verify identity across calls.
// ---------------------------------------------------------------------------
class IntSingleton : public commonlibs::Singleton<IntSingleton>
{
public:
    int value = 42;
};

// A second distinct type to show that Singleton<T> is independent per T.
class StringSingleton : public commonlibs::Singleton<StringSingleton>
{
public:
    std::string text = "hello";
};

// ---------------------------------------------------------------------------
// Basic identity tests
// ---------------------------------------------------------------------------

TEST(Singleton, InstanceReturnsSameAddress)
{
    auto &a = IntSingleton::instance();
    auto &b = IntSingleton::instance();
    EXPECT_EQ(&a, &b);
}

TEST(Singleton, InstanceIsNonNull)
{
    // The pointer behind the reference must be non-null.
    EXPECT_NE(nullptr, &IntSingleton::instance());
}

TEST(Singleton, InitialValueIsAccessible)
{
    EXPECT_EQ(42, IntSingleton::instance().value);
}

TEST(Singleton, ModificationPersistsAcrossCalls)
{
    IntSingleton::instance().value = 99;
    EXPECT_EQ(99, IntSingleton::instance().value);
}

// ---------------------------------------------------------------------------
// Independence between different Singleton types
// ---------------------------------------------------------------------------

TEST(Singleton, DifferentTypesHaveIndependentInstances)
{
    auto *iptr = &IntSingleton::instance();
    auto *sptr = &StringSingleton::instance();
    // The two pointers are to different objects in different types — their
    // addresses simply must not be the same.
    EXPECT_NE(static_cast<void*>(iptr), static_cast<void*>(sptr));
}

TEST(Singleton, StringSingleton_InitialValue)
{
    EXPECT_EQ("hello", StringSingleton::instance().text);
}

TEST(Singleton, StringSingleton_ModificationPersists)
{
    StringSingleton::instance().text = "world";
    EXPECT_EQ("world", StringSingleton::instance().text);
}
