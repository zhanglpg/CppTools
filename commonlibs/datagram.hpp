// UDP datagram connection
// datagram.hpp  —  Author: Liping Zhang, 2010
// Modernised: boost::bind→lambdas, boost::shared_ptr→std::shared_ptr,
//             boost::thread→std::thread, boost::array→std::array.
// NOTE: this file still requires commonlibs/probemessage.hpp (not in repo).
#pragma once

#include <boost/asio.hpp>
#include <array>
#include <iostream>
#include <iomanip>
#include <memory>
#include <string>
#include <thread>
#include <vector>
#include "commonlibs/probemessage.hpp"
#include "commonlibs/errorstatus.hpp"
#include "commonlibs/connection.hpp"

namespace commonlibs {

template <class Tmessage, class TmessageParser>
class datagram_imp_template {
public:
    typedef Tmessage message_type;

    datagram_imp_template(boost::asio::io_service &io_service)
        : socket_(io_service), io_s_(io_service)
    {}

    datagram_imp_template(boost::asio::io_service &io_service, int port)
        : socket_(io_service,
            boost::asio::ip::udp::endpoint(boost::asio::ip::udp::v4(), port)),
          io_s_(io_service)
    {}

    ~datagram_imp_template() { close(); }

    boost::asio::ip::udp::socket &socket() { return socket_; }

    void close() {
        try { socket_.close(); }
        catch (const std::exception &e_) { std::cerr << e_.what() << '\n'; }
    }

    void socket_open() {
        if (!socket_.is_open())
            socket_.open(boost::asio::ip::udp::v4());
    }

    void write_msg(boost::system::error_code &e,
                   const Tmessage &m_,
                   const boost::asio::ip::udp::endpoint &endp_) {
        socket_open();
        socket_.send_to(boost::asio::buffer(m_.getmsg()), endp_);
    }

    void set_option_nonblock() {
        socket_open();
        boost::asio::socket_base::non_blocking_io command(true);
        socket_.io_control(command);
    }

    int read_msg(boost::system::error_code &e,
                 boost::asio::ip::udp::endpoint &endp_,
                 bool nonblock = false) {
        boost::system::error_code err_;
        int timec_ = 0;
        do {
            socket_open();
            std::size_t len = socket_.receive_from(
                boost::asio::buffer(inbound_data_), endp_, 0, err_);
            e = err_;
            if (!err_ || err_ == boost::asio::error::message_size)
                return len > 0 ? static_cast<int>(len) : 0;
            if (err_ == boost::system::error_code(boost::asio::error::would_block)
                && !nonblock)
                sleep_relative_ms(10);
            else
                break;
        } while (timec_++ < 10 * MAX_WAIT_SEC && !nonblock);
        if (timec_ >= MAX_WAIT_SEC)
            err_ = boost::system::error_code(boost::asio::error::timed_out);
        return 0;
    }

    int sync_read(boost::system::error_code &e,
                  std::vector<Tmessage> &pm,
                  boost::asio::ip::udp::endpoint &endp_,
                  bool /*b_quiet*/ = false, bool nonblock = false) {
        int len = read_msg(e, endp_, nonblock);
        if (!e || e == boost::asio::error::message_size) {
            return static_cast<int>(
                mp_.parse2Message(inbound_data_.data(), len, pm));
        }
        if (!nonblock || e != boost::system::error_code(
                boost::asio::error::would_block))
            throw boost::system::system_error(e);
        return 0;
    }

    int sync_read_raw(boost::system::error_code &e, const char *&p_c,
                      boost::asio::ip::udp::endpoint &endp_,
                      bool /*b_quiet*/ = false, bool nonblock = false) {
        int len = read_msg(e, endp_, nonblock);
        if (!e || e == boost::asio::error::message_size) {
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
    boost::asio::ip::udp::socket                           socket_;
    std::array<typename Tmessage::Tchar, MAX_PACKET_SIZE>  inbound_data_;
    TmessageParser                                         mp_;
    boost::asio::io_service                               &io_s_;
};

template <class Tmessage, class TmessageParser>
class datagram_template
    : public datagram_imp_template<Tmessage, TmessageParser> {
public:
    datagram_template(boost::asio::io_service &io_service)
        : datagram_imp_template<Tmessage, TmessageParser>(io_service)
    {}
    datagram_template(boost::asio::io_service &io_service, int port)
        : datagram_imp_template<Tmessage, TmessageParser>(io_service, port)
    {}

    void sync_connect_host(const std::string &host_,
                           const std::string &service_,
                           boost::system::error_code &ec_) {
        boost::system::error_code error = boost::asio::error::host_not_found;
        boost::asio::ip::udp::resolver resolver(
            datagram_imp_template<Tmessage, TmessageParser>::io_service());
        auto endpoint_iterator = resolver.resolve(
            boost::asio::ip::udp::resolver::query(host_, service_));
        try {
            for (auto end = boost::asio::ip::udp::resolver::iterator{};
                 error && endpoint_iterator != end; ++endpoint_iterator) {
                datagram_imp_template<Tmessage, TmessageParser>::socket().close();
                datagram_imp_template<Tmessage, TmessageParser>::socket()
                    .connect(*endpoint_iterator, error);
            }
            if (!error) endp_ = *endpoint_iterator;
        } catch (const std::exception &e_) {
            std::cerr << e_.what() << '\n';
            ec_ = error;
            return;
        }
        ec_ = error;
    }

    const boost::asio::ip::udp::endpoint &get_endp() const { return endp_; }

private:
    boost::asio::ip::udp::endpoint endp_;
};

typedef datagram_imp_template<probemessage, messageParser>       datagram_imp;
typedef datagram_template<probemessage, messageParser>           datagram_pm;
typedef datagram_template<probemessage, textlineParser>          datagram_tl;
typedef datagram_template<probemessage, rawbinParser>            datagram_raw;
typedef datagram_template<strmessage,   textlineStrParser>       datagram_tl_str;
typedef std::shared_ptr<datagram_pm>                             datagram_pm_ptr;

// ---- Synchronized connector ------------------------------------------------

template <class datagram_implementor,
          class Tdatagram = datagram_pm,
          class Tmsgsaver = simple_saver>
class datagram_synchronized_connector {
public:
    datagram_synchronized_connector(boost::asio::io_service &ios_,
                                    const std::string &host_,
                                    const std::string &service_,
                                    bool start_session = true)
        : ios(ios_), hostname_(host_), servicename_(service_),
          b_server(false), b_connected(false), finished(false), b_quiet(false)
    {
        try {
            sptr_conn_ = std::make_shared<Tdatagram>(ios_);
            if (start_session)
                p_t_ = std::make_shared<std::thread>([this]{ start_imp(); });
        } catch (const std::exception &e_) {
            msgsaver.set_errstatus(STATUS_COMM_ERROR, e_.what());
            std::cerr << e_.what() << '\n';
        }
    }

    datagram_synchronized_connector(boost::asio::io_service &ios_,
                                    int service_,
                                    bool start_session = true)
        : ios(ios_), hostname_("localhost"), port(service_),
          b_server(true), b_connected(false), finished(false), b_quiet(false)
    {
        try {
            if (start_session)
                p_t_ = std::make_shared<std::thread>([this]{ start_imp(); });
        } catch (const std::exception &e_) {
            msgsaver.set_errstatus(STATUS_COMM_ERROR, e_.what());
            std::cerr << e_.what() << '\n';
        }
    }

    void start_session_explicit() {
        if (!b_connected)
            p_t_ = std::make_shared<std::thread>([this]{ start_imp(); });
    }

    void start() {
        do {
            try {
                boost::system::error_code ec_;
                if (!b_server) {
                    sptr_conn_->sync_connect_host(hostname_, servicename_, ec_);
                    b_connected = true;
                    handle_connect(ec_);
                } else {
                    sptr_conn_ = std::make_shared<Tdatagram>(ios, port);
                    handle_connect(ec_);
                }
            } catch (const std::exception &e_) {
                msgsaver.set_errstatus(STATUS_COMM_ERROR,
                    std::string(e_.what()) +
                    "--datagram_synchronized_connector::start()");
                std::cerr << e_.what()
                          << "--datagram_synchronized_connector::start()\n";
                try { close(); } catch (...) {}
            }
            sleep_relative_ms(200);
        } while (!finished);
    }

    ~datagram_synchronized_connector() {
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
                boost::system::error_code error;
                while (!finished) {
                    static_cast<datagram_implementor*>(this)
                        ->handle_connected(error);
                    if (error &&
                        error != boost::asio::error::message_size &&
                        error != boost::asio::error::would_block &&
                        error != boost::asio::error::try_again)
                        throw boost::system::system_error(error);
                }
            } else {
                boost::system::error_code err = e;
                static_cast<datagram_implementor*>(this)->handle_connected(err);
            }
        } catch (const std::exception &e_) {
            msgsaver.set_errstatus(STATUS_COMM_ERROR,
                std::string(e_.what()) +
                "--datagram_synchronized_connector(" +
                hostname_ + ":" + servicename_ + "::handle_connect())");
            std::cerr << hostname_ << ":" << e_.what()
                      << "--datagram_synchronized_connector::handle_connect()\n";
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
        if (sptr_conn_)
            sptr_conn_->write_msg(e, pm_, sptr_conn_->get_endp());
    }

    int conn_sync_read(boost::system::error_code &e,
                       std::vector<probemessage> &vpm, bool q) {
        return sptr_conn_->sync_read(e, vpm, sptr_conn_->get_endp(), q);
    }

    const std::string &get_hostname()    const { return hostname_; }
    const std::string &get_servicename() const { return servicename_; }
    const std::vector<typename Tdatagram::message_type> &get_vpm() const {
        return v_pm;
    }

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
    std::shared_ptr<Tdatagram> sptr_conn_;

private:
    boost::asio::io_service &ios;
    void start_imp() { this->start(); }

    std::string hostname_, servicename_;
    int  port = 0;
    bool finished, b_quiet, b_connected, b_server;
    std::shared_ptr<std::thread> p_t_;

protected:
    boost::asio::ip::udp::endpoint                          endp_;
    Tmsgsaver                                               msgsaver;
    std::vector<typename Tdatagram::message_type>           v_pm;
    int                                                     interval_sleepms = 0;
};

// ---- Transceiver (no background thread) ------------------------------------

template <class Tdatagram = datagram_pm, class Tmsgsaver = simple_saver>
class datagram_synchronized_transceiver {
public:
    datagram_synchronized_transceiver(boost::asio::io_service &ios_,
                                      const std::string &host_,
                                      const std::string &service_)
        : hostname_(host_), servicename_(service_), finished(false), b_quiet(false)
    {
        try {
            sptr_conn_ = std::make_shared<Tdatagram>(ios_);
            boost::system::error_code ec;
            resolve_host(ec);
        } catch (const std::exception &e_) {
            msgsaver.set_errstatus(STATUS_COMM_ERROR, e_.what());
            std::cerr << e_.what() << '\n';
        }
    }

    ~datagram_synchronized_transceiver() {
        try { close_connection(); }
        catch (const std::exception &e_) { std::cerr << e_.what() << '\n'; }
    }

    void close() { if (sptr_conn_) sptr_conn_->socket().close(); }

    void write_probemessage(boost::system::error_code &e,
                            const probemessage &pm_) {
        if (sptr_conn_) sptr_conn_->write_msg(e, pm_, endp_);
    }

    int conn_sync_read(boost::system::error_code &e,
                       std::vector<probemessage> &vpm, bool q) {
        return sptr_conn_->sync_read(e, vpm, endp_, q);
    }

    const std::string &get_hostname()    const { return hostname_; }
    const std::string &get_servicename() const { return servicename_; }
    void setFinished() { finished = true; }
    void close_connection() { try { close(); } catch (...) {} }

protected:
    std::shared_ptr<Tdatagram> sptr_conn_;

private:
    std::string hostname_, servicename_;
    bool        finished, b_quiet;

    void resolve_host(boost::system::error_code &ec_) {
        boost::system::error_code error = boost::asio::error::host_not_found;
        boost::asio::ip::udp::resolver resolver(sptr_conn_->io_service());
        auto endpoint_iterator = resolver.resolve(
            boost::asio::ip::udp::resolver::query(hostname_, servicename_), error);
        try {
            if (!error) endp_ = *endpoint_iterator;
        } catch (const std::exception &e_) {
            std::cerr << e_.what() << '\n';
            ec_ = error;
            return;
        }
        ec_ = error;
    }

    boost::asio::ip::udp::endpoint endp_;

protected:
    Tmsgsaver msgsaver;
};

typedef datagram_synchronized_transceiver<datagram_tl> datagram_transceiver_tl;

} // namespace commonlibs
