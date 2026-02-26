// TCP connection: acceptor and synchronized client
// connection.hpp  —  Author: Liping Zhang, 2008
// Modernised: boost::bind→lambdas, boost::shared_ptr→std::shared_ptr,
//             boost::thread→std::thread, boost::array→std::array,
//             deadline_timer→steady_timer, boost::posix_time→std::chrono.
// NOTE: this file still requires commonlibs/probemessage.hpp (not in repo).
#pragma once

#include <boost/asio.hpp>
#include <array>
#include <chrono>
#include <functional>
#include <iomanip>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>
#include "commonlibs/probemessage.hpp"
#include "commonlibs/errorstatus.hpp"
#include "commonlibs/sleeprelative.hpp"

using namespace boost::asio::ip;

namespace commonlibs {

// Serialisation primitives on top of a TCP socket.
template <class Tmessage, class TmessageParser>
class connection_imp_template {
public:
    connection_imp_template(boost::asio::io_service &io_service)
        : socket_(io_service), io_s_(io_service)
    {}

    boost::asio::ip::tcp::socket &socket() { return socket_; }

    void close() {
        try { socket_.close(); }
        catch (const std::exception &e_) { std::cerr << e_.what() << '\n'; }
    }

    void write_msg(boost::system::error_code &e, const Tmessage &m_) {
        socket_.write_some(boost::asio::buffer(m_.getmsg()), e);
    }

    void set_option_nonblock() {
        boost::asio::socket_base::non_blocking_io command(true);
        socket_.io_control(command);
    }

    void set_option_keepalive() {
        boost::asio::socket_base::keep_alive option(true);
        socket_.set_option(option);
    }

    int read_msg(boost::system::error_code &e, bool nonblock = false) {
        boost::system::error_code err_;
        int timec_ = 0;
        do {
            std::size_t len = socket_.read_some(
                boost::asio::buffer(inbound_data_), err_);
            e = err_;
            if (!err_) return len > 0 ? static_cast<int>(len) : 0;
            if (err_ == boost::system::error_code(boost::asio::error::would_block)
                && !nonblock) {
                sleep_relative_ms(100);
            } else break;
        } while (timec_++ < 10 * MAX_WAIT_SEC && !nonblock);
        if (timec_ >= MAX_WAIT_SEC)
            err_ = boost::system::error_code(boost::asio::error::timed_out);
        return 0;
    }

    int sync_read(boost::system::error_code &e,
                  std::vector<Tmessage> &pm,
                  bool /*b_quiet*/ = false, bool nonblock = false) {
        int len = read_msg(e, nonblock);
        if (!e) {
            return static_cast<int>(
                mp_.parse2Message(inbound_data_.data(), len, pm));
        }
        if (!nonblock || e != boost::system::error_code(
                boost::asio::error::would_block))
            throw boost::system::system_error(e);
        return 0;
    }

    int sync_read_raw(boost::system::error_code &e, const char *&p_c,
                      bool /*b_quiet*/ = false, bool nonblock = false) {
        int len = read_msg(e, nonblock);
        if (!e) {
            p_c = reinterpret_cast<const char *>(inbound_data_.data());
            return len;
        }
        if (!nonblock || e != boost::system::error_code(
                boost::asio::error::would_block))
            throw boost::system::system_error(e);
        return 0;
    }

    boost::asio::io_service &io_service() { return io_s_; }

public:
    enum packetsize { MAX_PACKET_SIZE = 65536 * 100 };
    enum { MAX_WAIT_SEC = 100 };

private:
    boost::asio::ip::tcp::socket                  socket_;
    std::array<unsigned char, MAX_PACKET_SIZE>    inbound_data_;
    TmessageParser                                mp_;
    boost::asio::io_service                      &io_s_;
};

template <class Tmessage, class TmessageParser>
class connection_template
    : public connection_imp_template<Tmessage, TmessageParser> {
public:
    connection_template(boost::asio::io_service &io_service)
        : connection_imp_template<Tmessage, TmessageParser>(io_service)
    {}

    void sync_connect_host(const std::string &host_,
                           const std::string &service_,
                           boost::system::error_code &ec_) {
        boost::system::error_code error = boost::asio::error::host_not_found;
        boost::asio::ip::tcp::resolver resolver(
            connection_imp_template<Tmessage, TmessageParser>::io_service());
        auto endpoint_iterator = resolver.resolve(
            boost::asio::ip::tcp::resolver::query(host_, service_));
        try {
            for (auto end = boost::asio::ip::tcp::resolver::iterator{};
                 error && endpoint_iterator != end; ++endpoint_iterator) {
                connection_imp_template<Tmessage, TmessageParser>::socket().close();
                connection_imp_template<Tmessage, TmessageParser>::socket()
                    .connect(*endpoint_iterator, error);
            }
        } catch (const std::exception &e_) { std::cerr << e_.what() << '\n'; }
        ec_ = error;
    }

    template<typename Handler>
    void async_connect_host(const std::string &host_,
                            const std::string &service_,
                            Handler handler) {
        boost::system::error_code error = boost::asio::error::host_not_found;
        boost::asio::ip::tcp::resolver resolver(
            connection_imp_template<Tmessage, TmessageParser>::io_service());
        auto endpoint_iterator = resolver.resolve(
            boost::asio::ip::tcp::resolver::query(host_, service_));
        try {
            for (auto end = boost::asio::ip::tcp::resolver::iterator{};
                 error && endpoint_iterator != end; ++endpoint_iterator) {
                connection_imp_template<Tmessage, TmessageParser>::socket().close();
                connection_imp_template<Tmessage, TmessageParser>::socket()
                    .async_connect(*endpoint_iterator, handler);
            }
        } catch (const std::exception &e_) { std::cerr << e_.what() << '\n'; }
    }
};

typedef connection_imp_template<probemessage, messageParser>    connection_imp;
typedef connection_template<probemessage, messageParser>        connection;
typedef connection_template<probemessage, textlineParser>       connection_tl;

// ---- TCP acceptor ----------------------------------------------------------

template<class tcp_acceptor_implementor>
class tcp_acceptor {
public:
    tcp_acceptor(boost::asio::io_service &io_service, int service)
        : acceptor_(io_service,
            boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), service)),
          timer_(io_service),
          finished(false), b_quiet(false), b_closed(true)
    {}

    void close() {
        if (conn) conn->socket().close();
        b_closed = true;
    }

    void async_accept_and_forward(int timeout = -1) {
        do {
            try {
                conn = std::make_shared<
                    connection_imp_template<probemessage, messageParser>>(
                    acceptor_.get_io_service());
                acceptor_.async_accept(conn->socket(),
                    [this](const boost::system::error_code &e) {
                        handle_accept_and_forward(e);
                    });
                if (timeout > 0) {
                    timer_.expires_after(std::chrono::seconds(timeout));
                    timer_.async_wait([this](const boost::system::error_code &) {
                        close();
                    });
                }
                acceptor_.get_io_service().run();
                if (!finished) sleep_relative_ms(1000);
            } catch (const std::exception &e_) {
                std::cerr << "Acceptor async connect: " << e_.what() << '\n';
            }
        } while (!finished);
    }

    void async_accept_and_open(int timeout = -1) {
        p_t_ = std::make_shared<std::thread>(
            [this, timeout]{ async_accept_and_open_thread(timeout); });
        finished = false;
    }

    void async_accept_and_open_thread(int timeout = -1) {
        try {
            conn = std::make_shared<connection_imp>(acceptor_.get_io_service());
            acceptor_.async_accept(conn->socket(),
                [this](const boost::system::error_code &e) { handle_accept(e); });
            if (timeout > 0) {
                timer_.expires_after(std::chrono::seconds(timeout));
                timer_.async_wait([this](const boost::system::error_code &) {
                    close();
                });
            }
            acceptor_.get_io_service().run();
            if (!finished) sleep_relative_ms(1000);
        } catch (const std::exception &e_) {
            std::cerr << "Acceptor async connect: " << e_.what() << '\n';
        }
    }

    void handle_accept(const boost::system::error_code &e) {
        b_closed = false;
        try {
            if (!e) {
                conn->set_option_nonblock();
                conn->set_option_keepalive();
                boost::system::error_code error;
                while (!error && !finished)
                    static_cast<tcp_acceptor_implementor*>(this)
                        ->handle_connected(error);
            }
        } catch (const std::exception &e_) {
            std::cerr << "TCP acceptor error: " << e_.what() << '\n';
        }
        if (!finished) {
            if (e) std::cerr << "Connection error (acceptor): "
                             << e.message() << '\n';
            close();
            conn = std::make_shared<connection_imp>(acceptor_.get_io_service());
            acceptor_.async_accept(conn->socket(),
                [this](const boost::system::error_code &ec) { handle_accept(ec); });
        } else {
            close();
        }
    }

    void setFinished() { finished = true; }

public:
    bool finished, b_quiet, b_closed;
    boost::asio::ip::tcp::acceptor acceptor_;
    boost::asio::steady_timer      timer_;
    std::shared_ptr<connection_imp_template<probemessage, messageParser>> conn;
    std::shared_ptr<std::thread>   p_t_;
};

typedef std::shared_ptr<connection> connection_ptr;

// ---- Synchronized client connector -----------------------------------------

template <class connector_implementor,
          class Tconnection = connection,
          class Tmsgsaver   = simple_saver>
class synchronized_connector {
public:
    synchronized_connector(boost::asio::io_service &ios_,
                           const std::string &host_,
                           const std::string &service_,
                           bool start_session = true)
        : hostname_(host_), servicename_(service_),
          b_connected(false), finished(false), b_quiet(false)
    {
        try {
            sptr_conn_ = std::make_shared<Tconnection>(ios_);
            if (start_session)
                p_t_ = std::make_shared<std::thread>(
                    [this]{ start_imp(); });
        } catch (const std::exception &e_) {
            msgsaver.set_errstatus(STATUS_COMM_ERROR, e_.what());
            std::cerr << e_.what() << '\n';
        }
    }

    void start_session_explicit() {
        p_t_ = std::make_shared<std::thread>([this]{ start_imp(); });
    }

    void start(unsigned int sleepms = 20000) {
        do {
            try {
                boost::system::error_code ec_;
                sptr_conn_->sync_connect_host(hostname_, servicename_, ec_);
                b_connected = true;
                handle_connect(ec_);
                sleep_relative_ms(static_cast<int>(sleepms));
            } catch (const std::exception &e_) {
                msgsaver.set_errstatus(STATUS_COMM_ERROR,
                    std::string(e_.what()) + "--synchronized_connector::start()");
                std::cerr << e_.what()
                          << "--synchronized_connector::start()\n";
                try { close(); } catch (...) {}
            }
        } while (!finished);
    }

    ~synchronized_connector() {
        try { close_connection(); }
        catch (const std::exception &e_) { std::cerr << e_.what() << '\n'; }
    }

    bool is_connected() const { return b_connected; }

    void close() {
        if (sptr_conn_) sptr_conn_->socket().close();
        b_connected = false;
    }

    void handle_connect(const boost::system::error_code &e) {
        try {
            if (!e) {
                sptr_conn_->set_option_nonblock();
                sptr_conn_->set_option_keepalive();
                boost::system::error_code error;
                while (!finished) {
                    static_cast<connector_implementor*>(this)
                        ->handle_connected(error);
                    if (error) throw boost::system::system_error(error);
                    msgsaver.set_errstatus(STATUS_OK, "");
                }
            } else {
                boost::system::error_code error = e;
                static_cast<connector_implementor*>(this)
                    ->handle_connected(error);
            }
        } catch (const std::exception &e_) {
            msgsaver.set_errstatus(STATUS_COMM_ERROR,
                std::string(e_.what()) + "--synchronized_connector(" +
                hostname_ + ":" + servicename_ + "::handle_connect())");
            std::cerr << hostname_ << ":" << e_.what()
                      << "--synchronized_connector::handle_connect()\n";
        }
        if (!finished) {
            if (e) {
                boost::system::system_error se(e);
                msgsaver.set_errstatus(STATUS_COMM_ERROR,
                    hostname_ + ":" + servicename_ + " - " + se.what());
            }
            try { close(); sleep_relative_ms(100); }
            catch (const std::exception &e_) {
                std::cerr << "Error closing: " << e_.what() << '\n';
            }
        } else {
            try { close(); b_connected = false; }
            catch (const std::exception &e_) {
                std::cerr << "Error closing: " << e_.what() << '\n';
            }
        }
    }

    void write_probemessage(boost::system::error_code &e,
                            const probemessage &pm_) {
        if (sptr_conn_) sptr_conn_->write_msg(e, pm_);
    }

    int conn_sync_read(boost::system::error_code &e,
                       std::vector<probemessage> &vpm, bool q) {
        return sptr_conn_->sync_read(e, vpm, q);
    }

    const std::string &get_hostname()    const { return hostname_; }
    const std::string &get_servicename() const { return servicename_; }
    void setFinished() { finished = true; }

    void close_connection() {
        try {
            std::cout << hostname_ << " - Exiting...\n";
            close();
            sleep_relative_ms(100);
            unsigned int temp_ = 0;
            while (!b_connected && ++temp_ < 10) sleep_relative_ms(100);
        } catch (...) {}
    }

protected:
    std::shared_ptr<Tconnection> sptr_conn_;

private:
    void start_imp() { this->start(); }

    std::string hostname_, servicename_;
    bool finished, b_quiet, b_connected;
    std::shared_ptr<std::thread> p_t_;

protected:
    Tmsgsaver msgsaver;
};

} // namespace commonlibs
