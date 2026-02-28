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

// ---------------------------------------------------------------------------
// Thread-safety: concurrent access from multiple threads
// ---------------------------------------------------------------------------

#include <thread>
#include <vector>
#include <atomic>

class CountingSingleton : public commonlibs::Singleton<CountingSingleton>
{
public:
    static std::atomic<int> construct_count;
    CountingSingleton() { construct_count.fetch_add(1); }
    int value = 100;
};
std::atomic<int> CountingSingleton::construct_count{0};

TEST(Singleton, MultithreadedAccessReturnsSameAddress)
{
    const int N = 16;
    std::vector<std::thread> threads;
    std::vector<void*> addresses(N, nullptr);

    for (int i = 0; i < N; ++i) {
        threads.emplace_back([&addresses, i]() {
            addresses[i] = &CountingSingleton::instance();
        });
    }
    for (auto &t : threads) t.join();

    // All threads must have obtained the same address
    for (int i = 1; i < N; ++i)
        EXPECT_EQ(addresses[0], addresses[i])
            << "Thread " << i << " got a different address";
}

TEST(Singleton, MultithreadedInitCalledExactlyOnce)
{
    // CountingSingleton was already constructed in the previous test;
    // std::call_once guarantees the constructor ran exactly once.
    EXPECT_EQ(1, CountingSingleton::construct_count.load());
}
