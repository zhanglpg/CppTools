#include "commonlibs/interproc/interp.hpp"
#include <gtest/gtest.h>
#include <cstring>
#include <string>

// POSIX shared memory names must begin with '/'.
// Using unique names per test avoids cross-test interference.

// ---------------------------------------------------------------------------
// Server-mode: create, open, write, read back
// ---------------------------------------------------------------------------

TEST(SharedMemory, ServerCreate_OpenSucceeds)
{
    named_shared_mem_server srv(const_cast<char*>("/cpptools_test_open"), 256);
    EXPECT_EQ(0, srv.open());
}

TEST(SharedMemory, ServerCreate_AddressIsNonNull)
{
    named_shared_mem_server srv(const_cast<char*>("/cpptools_test_addr"), 128);
    ASSERT_EQ(0, srv.open());
    EXPECT_NE(nullptr, srv.get_address());
}

TEST(SharedMemory, ServerCreate_SizeIsAtLeastRequested)
{
    named_shared_mem_server srv(const_cast<char*>("/cpptools_test_size"), 512);
    ASSERT_EQ(0, srv.open());
    EXPECT_GE(srv.get_size(), static_cast<std::size_t>(512));
}

TEST(SharedMemory, ServerCreate_WriteAndReadBack)
{
    named_shared_mem_server srv(const_cast<char*>("/cpptools_test_write"), 256);
    ASSERT_EQ(0, srv.open());

    std::strcpy(srv.get_address(), "hello_shm");
    EXPECT_STREQ("hello_shm", srv.get_address());
}

TEST(SharedMemory, ServerCreate_MemoryIsZeroInitialised)
{
    const std::size_t sz = 64;
    named_shared_mem_server srv(const_cast<char*>("/cpptools_test_zero"), sz);
    ASSERT_EQ(0, srv.open());

    char *p = srv.get_address();
    for (std::size_t i = 0; i < sz; ++i)
        EXPECT_EQ('\0', p[i]) << "non-zero at index " << i;
}

TEST(SharedMemory, ServerCreate_ZeroSize_OpenFails)
{
    named_shared_mem_server srv(const_cast<char*>("/cpptools_test_zerosize"), 0);
    EXPECT_NE(0, srv.open());
}

// ---------------------------------------------------------------------------
// Client-mode: open the shm the server created and read the data
// ---------------------------------------------------------------------------

TEST(SharedMemory, ClientReadsServerData)
{
    const char *name = "/cpptools_test_client";

    named_shared_mem_server srv(const_cast<char*>(name), 128);
    ASSERT_EQ(0, srv.open());
    std::strcpy(srv.get_address(), "from_server");

    // Client opens while server is still alive (shm still exists).
    {
        named_shared_mem_client cli(const_cast<char*>(name));
        ASSERT_EQ(0, cli.open());
        EXPECT_STREQ("from_server", cli.get_address());
        EXPECT_GE(cli.get_size(), static_cast<std::size_t>(128));
    }
    // srv destructor calls shm_unlink.
}

TEST(SharedMemory, ClientReadsUpdatedData)
{
    const char *name = "/cpptools_test_update";

    named_shared_mem_server srv(const_cast<char*>(name), 64);
    ASSERT_EQ(0, srv.open());
    std::strcpy(srv.get_address(), "first");

    {
        named_shared_mem_client cli(const_cast<char*>(name));
        ASSERT_EQ(0, cli.open());
        EXPECT_STREQ("first", cli.get_address());
    }

    // Update via server and open a second client.
    std::strcpy(srv.get_address(), "second");
    {
        named_shared_mem_client cli2(const_cast<char*>(name));
        ASSERT_EQ(0, cli2.open());
        EXPECT_STREQ("second", cli2.get_address());
    }
}

// ---------------------------------------------------------------------------
// get_size consistency
// ---------------------------------------------------------------------------

TEST(SharedMemory, GetSize_ReturnsRequestedSize)
{
    named_shared_mem_server srv(const_cast<char*>("/cpptools_test_sz2"), 1024);
    ASSERT_EQ(0, srv.open());
    EXPECT_EQ(static_cast<std::size_t>(1024), srv.get_size());
}
