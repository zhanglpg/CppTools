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

// ---------------------------------------------------------------------------
// Cross-process communication via fork()
// ---------------------------------------------------------------------------

#include <sys/wait.h>
#include <unistd.h>

TEST(SharedMemory, CrossProcess_ChildReadsParentData)
{
    const char *name = "/cpptools_test_fork";
    const std::size_t sz = 256;

    named_shared_mem_server srv(const_cast<char*>(name), sz);
    ASSERT_EQ(0, srv.open());

    // Parent writes data before forking
    std::strcpy(srv.get_address(), "from_parent");

    pid_t pid = fork();
    ASSERT_NE(-1, pid);

    if (pid == 0) {
        // Child: open as client and verify contents
        named_shared_mem_client cli(const_cast<char*>(name));
        if (cli.open() != 0) _exit(1);
        if (std::strcmp(cli.get_address(), "from_parent") != 0) _exit(2);
        _exit(0);   // _exit avoids running parent's destructors
    } else {
        int status;
        waitpid(pid, &status, 0);
        ASSERT_TRUE(WIFEXITED(status));
        EXPECT_EQ(0, WEXITSTATUS(status))
            << "Child exited with code " << WEXITSTATUS(status);
    }
}

TEST(SharedMemory, CrossProcess_LargePayload)
{
    const char *name = "/cpptools_test_fork_lg";
    const std::size_t sz = 8192;

    named_shared_mem_server srv(const_cast<char*>(name), sz);
    ASSERT_EQ(0, srv.open());

    // Fill with a repeating pattern
    char *p = srv.get_address();
    for (std::size_t i = 0; i < sz - 1; ++i)
        p[i] = 'A' + static_cast<char>(i % 26);
    p[sz - 1] = '\0';

    pid_t pid = fork();
    ASSERT_NE(-1, pid);

    if (pid == 0) {
        named_shared_mem_client cli(const_cast<char*>(name));
        if (cli.open() != 0) _exit(1);
        const char *cp = cli.get_address();
        for (std::size_t i = 0; i < sz - 1; ++i) {
            if (cp[i] != static_cast<char>('A' + i % 26)) _exit(2);
        }
        _exit(0);
    } else {
        int status;
        waitpid(pid, &status, 0);
        ASSERT_TRUE(WIFEXITED(status));
        EXPECT_EQ(0, WEXITSTATUS(status));
    }
}

TEST(SharedMemory, ClientOpenBeforeServer_Fails)
{
    // Ensure the name doesn't exist
    shm_unlink("/cpptools_test_noserver");

    named_shared_mem_client cli(const_cast<char*>("/cpptools_test_noserver"));
    EXPECT_NE(0, cli.open());
}
