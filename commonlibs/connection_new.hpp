// TCP connection: acceptor and tcp client connection
// connection_new.hpp
// ~~~~~~~~~~~~~~~~~~
// Author: Liping Zhang
/// 2008
#ifndef _COMMONLIBS_CONNECTION_NEW_HPP
#define _COMMONLIBS_CONNECTION_NEW_HPP

#include <asio.hpp>
#include <array>
#include <functional>
#include <iostream>
#include <iomanip>
#include <memory>
#include <optional>
#include <string>
#include <thread>
#include <vector>

using namespace asio::ip ;

namespace commonlibs {

/////\brief connection classes for non-probemessages

// The connection class provides raw primitives on top of a socket.
class connection_tcp
{
public:
  /// Constructor.
	connection_tcp(asio::io_context& io_service)
	: socket_p(new asio::ip::tcp::socket(io_service)),
	  io_s_(io_service), stopped(false),
	  dtimer(io_s_), verbose(0)
	{
	}
public:
	~connection_tcp()
	{
		if(verbose >1)
			std::cout << "connection_tcp:: ~connection_tcp -- being destroyed " << std::endl ;
		stopped = true ;
		dtimer.cancel() ;
		close() ;
	}
	/// Get the underlying socket. Used for making a connection or for accepting
	/// an incoming connection.
	asio::ip::tcp::socket& socket()
	{
		return *socket_p;
	}


	void close()
	{
		try {
			stopped = true ;
			socket_p->close() ;
		}
		catch(const std::exception &e_)
		{
			std::cerr <<  e_.what() << std::endl ;
		}
	}

	void stop()
	{
		dtimer.cancel() ;
		if(verbose > 1)
			std::cout << "connection_tcp::stop timer canceled\n";
		close();
		if(verbose >1)
			std::cout << "connection_tcp::stop closed\n" ;
	}

	void write_msg(asio::error_code &e ,
		 const void *m_, const size_t &size)
	{
		if(!stopped)
			socket_p->write_some(asio::buffer(m_, size), e);
		else
			e = asio::error_code(asio::error::not_connected) ;
		return ;
	}


	void set_option_nonblock(bool nonblock = true)
	{
		socket_p->non_blocking(nonblock);
		return ;
	}
	void set_option_keepalive(bool keepalive= true)
	{
		asio::socket_base::keep_alive option(keepalive);
		socket_p->set_option(option);
	}

	/// \brief read_msg:wrapped by sync_read
	/// \brief using the low level socket.read_some function
	/// \param e: INOUT the error code
	/// \param returns > 0 when success, 0 no data
	int sync_read_raw(asio::error_code& e, void *buf, size_t size)
	{

		asio::error_code err_ ;
		do {
			size_t len = socket_p->read_some(asio::buffer(buf, size) ,
				err_) ;
			e = err_ ;
			if(!err_)
			{
				return (int)len;
			}
			else if(err_ == asio::error_code(asio::error::would_block))
			{
				return 0;
			}
			else
				break ;
		}
		while(false) ;

		return -1 ; // on error
	}

	void sync_connect_host(const std::string & host_ , // IP v4 address
		const std::string & service_, asio::error_code &ec_ )
	{
		stopped = false ;
		asio::error_code error = asio::error::host_not_found;

		asio::ip::tcp::resolver resolver(io_s_);
		asio::ip::tcp::resolver::query query(host_, service_);
		asio::ip::tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
		asio::ip::tcp::resolver::iterator end;


		try  {
			while (error && endpoint_iterator != end)
			{
				socket_p->close();
				socket_p->connect(*endpoint_iterator++, error);
			}
			ec_ = error ;
		}
		catch(const std::exception & e_)
		{
			std::cerr << "connection_tcp: sync_connect_host exception --" << e_.what() << std::endl ;
			ec_ = asio::error::connection_aborted ;
		}
		return ;
	}


	void async_connect_host(const std::string & host_ , const std::string & service_,
		int timeout_ms, asio::error_code &ec_ )
	{
		stopped = false ;
		if (verbose >1 )
			std::cout << "connection_tcp::async_connection_host " << host_ << ":" << service_<< std::endl ;

		asio::ip::tcp::resolver resolver(io_s_);
		asio::ip::tcp::resolver::query query(host_, service_);
		asio::ip::tcp::resolver::iterator endpoint_iterator = resolver.resolve(query, ec_);
		io_s_.restart() ;
		try  {

			// Start the connect actor.
			start_connect(endpoint_iterator, timeout_ms);
			io_s_.run();
			if (verbose >1 )
				std::cout << "connection_tcp:: async_connect_host -- async connnection done after io_service run" << std::endl;
			ec_ = ecret ;
		}
		catch(const std::exception & e_)
		{
			std::cerr << "connection_tcp: async_connect_host exception --" << e_.what() << std::endl ;
			ec_ = asio::error::connection_aborted ;
			return ;
		}
	}

	void close_and_reconnect_sync(const std::string & host_ , const std::string & service_,
		asio::error_code &ec_)
	{
		close() ;
		socket_p.reset(new asio::ip::tcp::socket(io_s_));
		sync_connect_host(host_, service_, ec_) ;
	}

	void close_and_reconnect_async(const std::string & host_ , const std::string & service_, int timeoutms,
		asio::error_code &ec_)
	{
		close() ;
		socket_p.reset(new asio::ip::tcp::socket(io_s_));
		async_connect_host(host_, service_,timeoutms, ec_) ;
	}



	asio::io_context &
		io_service() const {return io_s_ ;}

	void setverbose_level(const int &verbose_) {
		verbose = verbose_ ;
	}
private:

	void start_connect(asio::ip::tcp::resolver::iterator endpoint_iterator, int timeoutms)
	{
		if (endpoint_iterator != asio::ip::tcp::resolver::iterator())
		{
			if(verbose > 1)
				std::cout << "Trying " << endpoint_iterator->endpoint() << "...\n";

			// Set a deadline for the connect operation.
			dtimer.expires_after(std::chrono::milliseconds(timeoutms));
			dtimer.async_wait([this](const asio::error_code &) { set_resulttimer(); });
		  // Start the asynchronous connect operation.
			socket_p->async_connect(endpoint_iterator->endpoint(),
			  [this, endpoint_iterator, timeoutms](const asio::error_code &ec) {
				  handle_connect(ec, endpoint_iterator, timeoutms);
			  });
		}
		else
		{
			// There are no more endpoints to try. Shut down the client.
			stop();
		}
	}

   void handle_connect(const asio::error_code& ec,
      asio::ip::tcp::resolver::iterator endpoint_iterator, int timeoutms)
	{
		ecret = ec;
		if (stopped) {
			dtimer.cancel() ;
			return;
		}


		// The async_connect() function automatically opens the socket at the start
		// of the asynchronous operation. If the socket is closed at this time then
		// the timeout handler must have run first.
		if (!socket_p->is_open())
		{
			if(verbose)
				std::cerr << "connection_tcp::handle_connect -- Connect timed out, trying next one" << std::endl ;

			// Try the next available endpoint.
			start_connect(++endpoint_iterator, timeoutms);
		}

		// Check if the connect operation failed before the deadline expired.
		else if (ec)
		{
			if(verbose)
				std::cerr << "connection_tcp::handle_connect -- Connect error: " << ec.message() << std::endl ;

		  // We need to close the socket used in the previous connection attempt
		  // before starting a new one.
			socket_p->close();

		  // Try the next available endpoint.
			if(verbose > 1)
				std::cerr << "connection_tcp::handle_connect -- trying next one" << std::endl ;
			start_connect(++endpoint_iterator, timeoutms);
		}

		// Otherwise we have successfully established a connection.
		else
		{
			if(verbose)
				std::cout << "connection_tcp::handle_connect -- Connected to " << endpoint_iterator->endpoint() <<  std::endl ;
			dtimer.cancel() ;
		}
	}


	void set_result(std::optional<asio::error_code>* a, const asio::error_code& b, size_t & sizeread, const size_t transferred)
	{
		*a = b;
		sizeread = transferred ;
	}

	void set_resulttimerread(std::optional<asio::error_code> *a, const asio::error_code& b)
	{
		*a = b;
	}

	void set_resulttimer()
	{
		if (stopped) {
			dtimer.cancel() ;
			return;
		}

		// Check whether the deadline has passed. We compare the deadline against
		// the current time since a new asynchronous operation may have moved the
		// deadline before this actor had a chance to run.
		if (dtimer.expiry() <= asio::steady_timer::clock_type::now())
		{
			// The deadline has passed. The socket is closed so that any outstanding
			// asynchronous operations are cancelled.
			if(verbose > 1)
				std::cout << "connection_tcp::set_resulttimer -- timer time out, timer set to infinity "<<  std::endl ;
			socket_p->close();
		}
	}
  	asio::error_code ecret;
	std::string hostname ; std::string service ;
	/// The underlying socket.
	bool stopped ;
	std::unique_ptr<asio::ip::tcp::socket> socket_p;
	asio::io_context &io_s_ ;
	asio::steady_timer dtimer  ;
	int verbose ;
};



// The connection class provides raw primitives on top of a socket.
class connection_tcp_server
{
public:
  /// Constructor.
	connection_tcp_server(asio::io_context& io_service, int port)
	: socket_p(new tcp::socket(io_service)),
	  io_s_(io_service), stopped(false),
	  dtimer(io_s_),
	  port_(port),
	  acceptor_(io_service, tcp::endpoint(tcp::v4(), port)),
	  verbose(0)
	{
	}
public:
	~connection_tcp_server()
	{
		if(verbose >1)
			std::cout << "connection_tcp:: ~connection_tcp -- being destroyed " << std::endl ;
		stopped = true ;
		dtimer.cancel() ;
		close() ;
	}
	/// Get the underlying socket. Used for making a connection or for accepting
	/// an incoming connection.
	tcp::socket& socket()
	{
		return *socket_p;
	}

	void close()
	{
		try {
			stopped = true ;
			acceptor_.cancel() ;
			socket_p->close() ;
		}
		catch(const std::exception &e_)
		{
			std::cerr <<  e_.what() << std::endl ;
		}
	}

	void stop()
	{
		dtimer.cancel() ;
		if(verbose > 1)
			std::cout << "connection_tcp_server::stop timer canceled\n";
		close();
		if(verbose >1)
			std::cout << "connection_tcp_server::stop closed\n" ;
	}

	void write_msg(asio::error_code &e ,
		 const void *m_, const size_t &size)
	{
		if(!stopped)
			socket_p->write_some(asio::buffer(m_, size), e);
		else
			e = asio::error_code(asio::error::not_connected) ;
		return ;
	}


	void set_option_nonblock(bool nonblock = true)
	{
		socket_p->non_blocking(nonblock);
		return ;
	}
	void set_option_keepalive(bool keepalive= true)
	{
		asio::socket_base::keep_alive option(keepalive);
		socket_p->set_option(option);
	}

	/// \brief read_msg:wrapped by sync_read
	/// \brief using the low level socket.read_some function
	/// \param e: INOUT the error code
	/// \param returns > 0 when success, 0 no data
	int sync_read_raw(asio::error_code& e, void *buf, size_t size)
	{

		asio::error_code err_ ;
		do {
			size_t len = socket_p->read_some(asio::buffer(buf, size) ,
				err_) ;
			e = err_ ;
			if(!err_)
			{
				return (int)len;
			}
			else if(err_ == asio::error_code(asio::error::would_block))
			{
				return 0;
			}
			else
				break ;
		}
		while(false) ;

		return -1 ; // on error
	}

	void sync_accept_host(const std::string & host_ ,
		asio::error_code &ec_ )
	{
		stopped = false ;

		try  {
			acceptor_.accept(*socket_p, ec_) ;
			if(ec_)
				throw asio::system_error(ec_) ;

		}
		catch(const std::exception & e_)
		{
			std::cerr << "connection_tcp_server::sync_accept_host exception --" << e_.what() << std::endl ;
			ec_ = asio::error::connection_aborted ;
		}
		return ;
	}

	void async_accept_host(const std::string & host_ ,
		int timeout_ms, asio::error_code &ec_ )
	{
		stopped = false ;
		if (verbose >1 )
			std::cout << "connection_tcp_server::async_accept_host " << host_ << std::endl ;

		io_s_.restart() ;
		try  {

			// Start the connect actor.
			start_connect(timeout_ms);
			io_s_.run();
			if (verbose >1 )
				std::cout << "connection_tcp_server:: async_accept_host -- async accept done after io_service run" << std::endl;
			ec_ = ecret ;
		}
		catch(const std::exception & e_)
		{
			std::cerr << "connection_tcp_server: async_accept_host exception --" << e_.what() << std::endl ;
			ec_ = asio::error::connection_aborted ;
			return ;
		}
	}


	void close_and_reaccept_async(const std::string & host_ , int timeoutms,
		asio::error_code &ec_)
	{
		close() ;
		socket_p.reset(new asio::ip::tcp::socket(io_s_));
		async_accept_host(host_, timeoutms, ec_) ;
	}

	void close_and_reaccept_sync(const std::string & host_ ,
		asio::error_code &ec_)
	{
		close() ;
		socket_p.reset(new asio::ip::tcp::socket(io_s_));
		sync_accept_host(host_, ec_) ;
	}



	asio::io_context &
		io_service() const {return io_s_ ;}

	void setverbose_level(const int &verbose_) {
		verbose = verbose_ ;
	}
private:

	void start_connect(int timeoutms)
	{
		if(verbose > 1)
			std::cout << "Trying accepting at " << port_ <<  "\n";

		// Set a deadline for the connect operation.
		dtimer.expires_after(std::chrono::milliseconds(timeoutms));
		dtimer.async_wait([this](const asio::error_code &) { set_resulttimer(); });
		// Start the asynchronous accept operation.
		acceptor_.async_accept(*socket_p,
			[this, timeoutms](const asio::error_code &ec) { handle_accept(ec, timeoutms); });
	}

   void handle_accept(const asio::error_code& ec,
      int timeoutms)
	{
		ecret = ec;
		if (stopped) {
			dtimer.cancel() ;
			return;
		}

		if (!socket_p->is_open())
		{
			if(verbose)
				std::cerr << "connection_tcp_server::handle_accept -- Connect timed out" << std::endl ;
		}

		// Check if the connect operation failed before the deadline expired.
		else if (ec)
		{
			if(verbose)
				std::cerr << "connection_tcp_server::handle_accept -- Connect error: " << ec.message() << std::endl ;
			dtimer.cancel() ;
		  // We need to close the socket used in the previous connection attempt
		  // before starting a new one.
			socket_p->close();

		}

		// Otherwise we have successfully established a connection.
		else
		{
			if(verbose)
				std::cout << "connection_tcp_server::handle_accept -- accepted connection from " << socket_p->remote_endpoint() <<  std::endl ;
			dtimer.cancel() ;
		}
	}



	void set_resulttimer()
	{
		if (stopped) {
			dtimer.cancel() ;
			return;
		}

		// Check whether the deadline has passed. We compare the deadline against
		// the current time since a new asynchronous operation may have moved the
		// deadline before this actor had a chance to run.
		if (dtimer.expiry() <= asio::steady_timer::clock_type::now())
		{
			// The deadline has passed. The socket is closed so that any outstanding
			// asynchronous operations are cancelled.
			acceptor_.cancel() ;
			if(verbose > 1)
				std::cout << "connection_tcp_server::set_resulttimer -- timer time out"<<  std::endl ;
			socket_p->close();

		}
	}
  	asio::error_code ecret;
	std::string hostname ; std::string service ;
	/// The underlying socket.
	bool stopped ;
	std::unique_ptr<asio::ip::tcp::socket> socket_p;

	asio::io_context &io_s_ ;
	asio::steady_timer dtimer  ;
	int verbose ;
	int port_ ;
	tcp::acceptor acceptor_;
};



} // namespace

#endif // SERIALIZATION_CONNECTION_HPP
