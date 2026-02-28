#include "commonlibs/connection_new.hpp"
#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include <cstring>

// Find an available TCP port by binding to port 0 and reading the assignment.
static int find_free_port()
{
    asio::io_context io;
    asio::ip::tcp::acceptor acc(io,
        asio::ip::tcp::endpoint(asio::ip::tcp::v4(), 0));
    int port = acc.local_endpoint().port();
    acc.close();
    return port;
}

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

TEST(ConnectionNew, TcpClientConstructionDoesNotThrow)
{
    asio::io_context io;
    EXPECT_NO_THROW({
        commonlibs::connection_tcp client(io);
    });
}

TEST(ConnectionNew, TcpServerConstructionDoesNotThrow)
{
    asio::io_context io;
    EXPECT_NO_THROW({
        commonlibs::connection_tcp_server server(io, find_free_port());
    });
}

// ---------------------------------------------------------------------------
// Server accept timeout (no client connects)
// ---------------------------------------------------------------------------

TEST(ConnectionNew, TcpServerAcceptTimeout)
{
    asio::io_context io;
    int port = find_free_port();
    commonlibs::connection_tcp_server server(io, port);

    asio::error_code ec;
    server.async_accept_host("localhost", 200, ec);
    // After timeout, socket should be closed
    EXPECT_FALSE(server.socket().is_open());
}

// ---------------------------------------------------------------------------
// Client-server loopback: sync connect, write, read
// ---------------------------------------------------------------------------

TEST(ConnectionNew, TcpClientServerSyncLoopback)
{
    int port = find_free_port();

    asio::io_context server_io;
    commonlibs::connection_tcp_server server(server_io, port);

    // Accept in background thread with timeout
    asio::error_code server_ec;
    std::thread server_thread([&]() {
        server.async_accept_host("localhost", 3000, server_ec);
    });

    // Give the server time to start listening
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Client connects with its own io_context
    asio::io_context client_io;
    commonlibs::connection_tcp client(client_io);
    asio::error_code client_ec;
    client.sync_connect_host("127.0.0.1", std::to_string(port), client_ec);
    EXPECT_FALSE(client_ec) << "Client connect failed: " << client_ec.message();

    server_thread.join();
    EXPECT_FALSE(server_ec) << "Server accept failed: " << server_ec.message();

    // Client writes, server reads
    const char msg[] = "hello";
    asio::error_code write_ec;
    client.write_msg(write_ec, msg, 5);
    EXPECT_FALSE(write_ec) << "Write failed: " << write_ec.message();

    char buf[256] = {};
    asio::error_code read_ec;
    int n = server.sync_read_raw(read_ec, buf, sizeof(buf));
    EXPECT_GT(n, 0);
    EXPECT_EQ(std::string("hello"), std::string(buf, 5));

    client.close();
    server.close();
}

// ---------------------------------------------------------------------------
// Bidirectional communication
// ---------------------------------------------------------------------------

TEST(ConnectionNew, TcpBidirectional)
{
    int port = find_free_port();

    asio::io_context server_io;
    commonlibs::connection_tcp_server server(server_io, port);

    asio::error_code server_ec;
    std::thread server_thread([&]() {
        server.async_accept_host("localhost", 3000, server_ec);
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    asio::io_context client_io;
    commonlibs::connection_tcp client(client_io);
    asio::error_code client_ec;
    client.sync_connect_host("127.0.0.1", std::to_string(port), client_ec);
    ASSERT_FALSE(client_ec) << client_ec.message();

    server_thread.join();
    ASSERT_FALSE(server_ec) << server_ec.message();

    // Server writes, client reads
    const char srv_msg[] = "world";
    asio::error_code ec;
    server.write_msg(ec, srv_msg, 5);
    EXPECT_FALSE(ec) << "Server write failed: " << ec.message();

    char buf[256] = {};
    client.set_option_nonblock(false);
    int n = client.sync_read_raw(ec, buf, sizeof(buf));
    EXPECT_GT(n, 0);
    EXPECT_EQ(std::string("world"), std::string(buf, 5));

    client.close();
    server.close();
}

// ---------------------------------------------------------------------------
// Write after close returns error
// ---------------------------------------------------------------------------

TEST(ConnectionNew, TcpWriteAfterClose_ReturnsError)
{
    asio::io_context io;
    commonlibs::connection_tcp client(io);
    client.close();

    asio::error_code ec;
    const char msg[] = "test";
    client.write_msg(ec, msg, 4);
    EXPECT_TRUE(ec);
    EXPECT_EQ(asio::error::not_connected, ec);
}

// ---------------------------------------------------------------------------
// Multiple sequential connections on the same port
// ---------------------------------------------------------------------------

TEST(ConnectionNew, TcpServerAcceptsAfterClientDisconnects)
{
    int port = find_free_port();

    // First connection
    {
        asio::io_context server_io;
        commonlibs::connection_tcp_server server(server_io, port);

        asio::error_code server_ec;
        std::thread server_thread([&]() {
            server.async_accept_host("localhost", 3000, server_ec);
        });

        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        asio::io_context client_io;
        commonlibs::connection_tcp client(client_io);
        asio::error_code ec;
        client.sync_connect_host("127.0.0.1", std::to_string(port), ec);
        ASSERT_FALSE(ec);

        server_thread.join();
        EXPECT_FALSE(server_ec);

        client.close();
        server.close();
    }

    // Second connection on the same port
    {
        asio::io_context server_io;
        commonlibs::connection_tcp_server server(server_io, port);

        asio::error_code server_ec;
        std::thread server_thread([&]() {
            server.async_accept_host("localhost", 3000, server_ec);
        });

        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        asio::io_context client_io;
        commonlibs::connection_tcp client(client_io);
        asio::error_code ec;
        client.sync_connect_host("127.0.0.1", std::to_string(port), ec);
        EXPECT_FALSE(ec) << "Second connect failed: " << ec.message();

        server_thread.join();
        EXPECT_FALSE(server_ec);

        client.close();
        server.close();
    }
}
