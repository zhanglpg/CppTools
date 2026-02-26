// TCP connection: client and server
// connection_new.hpp
// Author: Liping Zhang, 2008
// Modernised: boost::bind→lambdas, boost::shared_ptr→std::shared_ptr,
//             boost::thread→std::thread, deadline_timer→steady_timer,
//             boost::posix_time→std::chrono, uniqueptr.hpp shim removed.
#pragma once

#include <boost/asio.hpp>
#include <chrono>
#include <iostream>
#include <iomanip>
#include <memory>
#include <string>
#include <thread>
#include <vector>

using namespace boost::asio::ip;

namespace commonlibs {

// Provides raw byte I/O over a TCP socket.
class connection_tcp {
public:
    connection_tcp(boost::asio::io_service &io_service)
        : socket_p(std::make_unique<boost::asio::ip::tcp::socket>(io_service)),
          io_s_(io_service), stopped(false),
          dtimer(io_s_), verbose(0)
    {}

    ~connection_tcp() {
        if (verbose > 1)
            std::cout << "connection_tcp::~connection_tcp -- being destroyed\n";
        stopped = true;
        dtimer.cancel();
        close();
    }

    boost::asio::ip::tcp::socket &socket() { return *socket_p; }

    void close() {
        try {
            stopped = true;
            socket_p->close();
        } catch (const std::exception &e_) {
            std::cerr << e_.what() << '\n';
        }
    }

    void stop() {
        dtimer.cancel();
        if (verbose > 1) std::cout << "connection_tcp::stop timer canceled\n";
        close();
        if (verbose > 1) std::cout << "connection_tcp::stop closed\n";
    }

    void write_msg(boost::system::error_code &e,
                   const void *m_, const std::size_t &size) {
        if (!stopped)
            socket_p->write_some(boost::asio::buffer(m_, size), e);
        else
            e = boost::system::error_code(boost::asio::error::not_connected);
    }

    void set_option_nonblock(bool nonblock = true) {
        boost::asio::socket_base::non_blocking_io command(nonblock);
        socket_p->io_control(command);
    }

    void set_option_keepalive(bool keepalive = true) {
        boost::asio::socket_base::keep_alive option(keepalive);
        socket_p->set_option(option);
    }

    // Returns bytes read (>0), 0 if would-block, -1 on error.
    int sync_read_raw(boost::system::error_code &e, void *buf, std::size_t size) {
        boost::system::error_code err_;
        std::size_t len = socket_p->read_some(boost::asio::buffer(buf, size), err_);
        e = err_;
        if (!err_)  return static_cast<int>(len);
        if (err_ == boost::system::error_code(boost::asio::error::would_block))
            return 0;
        return -1;
    }

    void sync_connect_host(const std::string &host_,
                           const std::string &service_,
                           boost::system::error_code &ec_) {
        stopped = false;
        boost::system::error_code error = boost::asio::error::host_not_found;
        boost::asio::ip::tcp::resolver resolver(io_s_);
        auto endpoint_iterator =
            resolver.resolve(boost::asio::ip::tcp::resolver::query(host_, service_));
        try {
            for (auto end = boost::asio::ip::tcp::resolver::iterator{};
                 error && endpoint_iterator != end; ++endpoint_iterator) {
                socket_p->close();
                socket_p->connect(*endpoint_iterator, error);
            }
            ec_ = error;
        } catch (const std::exception &e_) {
            std::cerr << "connection_tcp::sync_connect_host -- " << e_.what() << '\n';
            ec_ = boost::asio::error::connection_aborted;
        }
    }

    void async_connect_host(const std::string &host_,
                            const std::string &service_,
                            int timeout_ms,
                            boost::system::error_code &ec_) {
        stopped = false;
        if (verbose > 1)
            std::cout << "connection_tcp::async_connect_host "
                      << host_ << ":" << service_ << '\n';
        boost::asio::ip::tcp::resolver resolver(io_s_);
        auto endpoint_iterator =
            resolver.resolve(boost::asio::ip::tcp::resolver::query(host_, service_), ec_);
        io_s_.reset();
        try {
            start_connect(endpoint_iterator, timeout_ms);
            io_s_.run();
            ec_ = ecret;
        } catch (const std::exception &e_) {
            std::cerr << "connection_tcp::async_connect_host -- " << e_.what() << '\n';
            ec_ = boost::asio::error::connection_aborted;
        }
    }

    void close_and_reconnect_sync(const std::string &host_,
                                  const std::string &service_,
                                  boost::system::error_code &ec_) {
        close();
        socket_p = std::make_unique<boost::asio::ip::tcp::socket>(io_s_);
        sync_connect_host(host_, service_, ec_);
    }

    void close_and_reconnect_async(const std::string &host_,
                                   const std::string &service_,
                                   int timeoutms,
                                   boost::system::error_code &ec_) {
        close();
        socket_p = std::make_unique<boost::asio::ip::tcp::socket>(io_s_);
        async_connect_host(host_, service_, timeoutms, ec_);
    }

    boost::asio::io_service &io_service() const { return io_s_; }
    void setverbose_level(int v) { verbose = v; }

private:
    void start_connect(boost::asio::ip::tcp::resolver::iterator endpoint_iterator,
                       int timeoutms) {
        if (endpoint_iterator == boost::asio::ip::tcp::resolver::iterator()) {
            stop(); return;
        }
        if (verbose > 1)
            std::cout << "Trying " << endpoint_iterator->endpoint() << "...\n";

        dtimer.expires_after(std::chrono::milliseconds(timeoutms));
        dtimer.async_wait([this](const boost::system::error_code &) {
            set_resulttimer();
        });
        socket_p->async_connect(
            endpoint_iterator->endpoint(),
            [this, endpoint_iterator, timeoutms](
                const boost::system::error_code &ec) mutable {
                handle_connect(ec, endpoint_iterator, timeoutms);
            });
    }

    void handle_connect(const boost::system::error_code &ec,
                        boost::asio::ip::tcp::resolver::iterator endpoint_iterator,
                        int timeoutms) {
        ecret = ec;
        if (stopped) { dtimer.cancel(); return; }

        if (!socket_p->is_open()) {
            if (verbose)
                std::cerr << "connection_tcp::handle_connect -- timed out\n";
            start_connect(++endpoint_iterator, timeoutms);
        } else if (ec) {
            if (verbose)
                std::cerr << "connection_tcp::handle_connect -- error: "
                          << ec.message() << '\n';
            socket_p->close();
            start_connect(++endpoint_iterator, timeoutms);
        } else {
            if (verbose)
                std::cout << "connection_tcp::handle_connect -- connected to "
                          << endpoint_iterator->endpoint() << '\n';
            dtimer.cancel();
        }
    }

    void set_resulttimer() {
        if (stopped) { dtimer.cancel(); return; }
        if (dtimer.expiry() <= std::chrono::steady_clock::now()) {
            if (verbose > 1)
                std::cout << "connection_tcp::set_resulttimer -- timed out\n";
            socket_p->close();
        }
    }

    boost::system::error_code                      ecret;
    bool                                           stopped;
    std::unique_ptr<boost::asio::ip::tcp::socket>  socket_p;
    boost::asio::io_service                       &io_s_;
    boost::asio::steady_timer                      dtimer;
    int                                            verbose;
};


// Server-side TCP connection (accepts incoming connections).
class connection_tcp_server {
public:
    connection_tcp_server(boost::asio::io_service &io_service, int port)
        : socket_p(std::make_unique<tcp::socket>(io_service)),
          io_s_(io_service), stopped(false),
          dtimer(io_s_), port_(port), verbose(0),
          acceptor_(io_service, tcp::endpoint(tcp::v4(), port))
    {}

    ~connection_tcp_server() {
        if (verbose > 1)
            std::cout << "connection_tcp_server::~connection_tcp_server\n";
        stopped = true;
        dtimer.cancel();
        close();
    }

    tcp::socket &socket() { return *socket_p; }

    void close() {
        try {
            stopped = true;
            acceptor_.cancel();
            socket_p->close();
        } catch (const std::exception &e_) {
            std::cerr << e_.what() << '\n';
        }
    }

    void stop() {
        dtimer.cancel();
        if (verbose > 1) std::cout << "connection_tcp_server::stop timer canceled\n";
        close();
        if (verbose > 1) std::cout << "connection_tcp_server::stop closed\n";
    }

    void write_msg(boost::system::error_code &e,
                   const void *m_, const std::size_t &size) {
        if (!stopped)
            socket_p->write_some(boost::asio::buffer(m_, size), e);
        else
            e = boost::system::error_code(boost::asio::error::not_connected);
    }

    void set_option_nonblock(bool nonblock = true) {
        boost::asio::socket_base::non_blocking_io command(nonblock);
        socket_p->io_control(command);
    }

    void set_option_keepalive(bool keepalive = true) {
        boost::asio::socket_base::keep_alive option(keepalive);
        socket_p->set_option(option);
    }

    int sync_read_raw(boost::system::error_code &e, void *buf, std::size_t size) {
        boost::system::error_code err_;
        std::size_t len = socket_p->read_some(boost::asio::buffer(buf, size), err_);
        e = err_;
        if (!err_)  return static_cast<int>(len);
        if (err_ == boost::system::error_code(boost::asio::error::would_block))
            return 0;
        return -1;
    }

    void sync_accept_host(const std::string &,
                          boost::system::error_code &ec_) {
        stopped = false;
        try {
            acceptor_.accept(*socket_p, ec_);
            if (ec_) throw boost::system::system_error(ec_);
        } catch (const std::exception &e_) {
            std::cerr << "connection_tcp_server::sync_accept_host -- "
                      << e_.what() << '\n';
            ec_ = boost::asio::error::connection_aborted;
        }
    }

    void async_accept_host(const std::string &host_,
                           int timeout_ms,
                           boost::system::error_code &ec_) {
        stopped = false;
        if (verbose > 1)
            std::cout << "connection_tcp_server::async_accept_host "
                      << host_ << '\n';
        io_s_.reset();
        try {
            start_connect(timeout_ms);
            io_s_.run();
            ec_ = ecret;
        } catch (const std::exception &e_) {
            std::cerr << "connection_tcp_server::async_accept_host -- "
                      << e_.what() << '\n';
            ec_ = boost::asio::error::connection_aborted;
        }
    }

    void close_and_reaccept_async(const std::string &host_, int timeoutms,
                                   boost::system::error_code &ec_) {
        close();
        socket_p = std::make_unique<boost::asio::ip::tcp::socket>(io_s_);
        async_accept_host(host_, timeoutms, ec_);
    }

    void close_and_reaccept_sync(const std::string &host_,
                                  boost::system::error_code &ec_) {
        close();
        socket_p = std::make_unique<boost::asio::ip::tcp::socket>(io_s_);
        sync_accept_host(host_, ec_);
    }

    boost::asio::io_service &io_service() const { return io_s_; }
    void setverbose_level(int v) { verbose = v; }

private:
    void start_connect(int timeoutms) {
        if (verbose > 1)
            std::cout << "Waiting for connection on port " << port_ << '\n';
        dtimer.expires_after(std::chrono::milliseconds(timeoutms));
        dtimer.async_wait([this](const boost::system::error_code &) {
            set_resulttimer();
        });
        acceptor_.async_accept(*socket_p,
            [this, timeoutms](const boost::system::error_code &ec) {
                handle_accept(ec, timeoutms);
            });
    }

    void handle_accept(const boost::system::error_code &ec, int) {
        ecret = ec;
        if (stopped) { dtimer.cancel(); return; }

        if (!socket_p->is_open()) {
            if (verbose)
                std::cerr << "connection_tcp_server::handle_accept -- timed out\n";
        } else if (ec) {
            if (verbose)
                std::cerr << "connection_tcp_server::handle_accept -- error: "
                          << ec.message() << '\n';
            dtimer.cancel();
            socket_p->close();
        } else {
            if (verbose)
                std::cout << "connection_tcp_server::handle_accept -- accepted from "
                          << socket_p->remote_endpoint() << '\n';
            dtimer.cancel();
        }
    }

    void set_resulttimer() {
        if (stopped) { dtimer.cancel(); return; }
        if (dtimer.expiry() <= std::chrono::steady_clock::now()) {
            acceptor_.cancel();
            if (verbose > 1)
                std::cout << "connection_tcp_server::set_resulttimer -- timed out\n";
            socket_p->close();
        }
    }

    boost::system::error_code                      ecret;
    bool                                           stopped;
    std::unique_ptr<boost::asio::ip::tcp::socket>  socket_p;
    boost::asio::io_service                       &io_s_;
    boost::asio::steady_timer                      dtimer;
    int                                            verbose;
    int                                            port_;
    tcp::acceptor                                  acceptor_;
};

} // namespace commonlibs
