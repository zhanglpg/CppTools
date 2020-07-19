// TCP connection: acceptor and tcp client connection
// connection.hpp
// ~~~~~~~~~~~~~~
// Author: Liping Zhang
/// 2008
#ifndef _COMMONLIBS_CONNECTION_NEW_HPP
#define _COMMONLIBS_CONNECTION_NEW_HPP

#include <boost/array.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/thread/xtime.hpp>
#include <boost/optional.hpp>
#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <boost/signal.hpp>
#include <boost/foreach.hpp>
#include <memory>
#include "commonlibs/uniqueptr.hpp"

using namespace boost::asio::ip ;

namespace commonlibs {

/////\brief connection classes for non-probemessages

// The connection class provides raw primitives on top of a socket.
class connection_tcp
{
public:
  /// Constructor.
	connection_tcp(boost::asio::io_service& io_service)
	: socket_p(new boost::asio::ip::tcp::socket(io_service)),
	  io_s_(io_service), stopped(false), 
	  dtimer(io_s_) 
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
	boost::asio::ip::tcp::socket& socket()
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
	
	void write_msg(boost::system::error_code &e , 
		 const void *m_, const size_t &size)
	{
		if(!stopped) 
			socket_p->write_some(boost::asio::buffer(m_, size), e);
		else 
			e = boost::system::error_code(boost::asio::error::not_connected) ;
		return ;
	}


	void set_option_nonblock(bool nonblock = true)
	{
		boost::asio::socket_base::non_blocking_io command(nonblock);
		socket_p->io_control(command);
		return ;
	}
	void set_option_keepalive(bool keepalive= true)
	{
		boost::asio::socket_base::keep_alive option(keepalive);
		socket_p->set_option(option);
	}

	/// \brief read_msg:wrapped by sync_read
	/// \brief using the low level socket.read_some function
	/// \param e: INOUT the error code
	/// \param returns > 0 when success, 0 no data 
	int sync_read_raw(boost::system::error_code& e, void *buf, size_t size)
	{
		
		boost::system::error_code err_ ;
		do {
			size_t len = socket_p->read_some(boost::asio::buffer(buf, size) ,
				err_) ;
			e = err_ ;
			if(!err_)
			{
				return (int)len;
			}
			else if(err_ == boost::system::error_code(boost::asio::error::would_block))
			{
				return 0; 
			}
			else 
				break ;
		}
		while(false) ;
		
		return -1 ; // on error
	}
	
	/*
	// synchronous read with time out implemented  using asynchronous read
	int sync_read_raw(boost::system::error_code &e, 
		void * buf, const size_t & size, 
		unsigned int timeout_ms) 
	{
//		set_option_nonblock(true); 
		
		size_t sizeread = 0 ; 
		if (verbose > 1) 
			std::cout << "connection_tcp::sync_read_raw_async "<< std::endl ;

		io_s_.reset() ;
		boost::optional<boost::system::error_code> timer_result; 
		boost::asio::deadline_timer timer(io_s_); 
		timer.expires_from_now(boost::posix_time::milliseconds(timeout_ms)); 
		timer.async_wait(boost::bind(&connection_tcp::set_resulttimerread,this, &timer_result, _1)); 
		boost::optional<boost::system::error_code> read_result;
		
		boost::asio::async_read(socket(), boost::asio::buffer(buf, size), 
		boost::bind(&connection_tcp::set_result,this, &read_result, _1, sizeread, _2)); 
  
		if (verbose > 1)
			std::cout <<"connection_tcp::sync_read_raw_async ios resetted \n" ;
		if (io_s_.run_one()) 
		{ 
		  if(verbose > 1)
			std::cout << "connection_tcp::sync_read_raw_async run_one returned\n" ;
		  if (read_result) 
			timer.cancel(); 
		  else if (timer_result) 
			socket_p->cancel(); 
		} 
		if (read_result)  {
			e = *read_result ;
			return (int) sizeread ;
		}
		else {
			if (verbose > 1)
				std::cout << "connection_tcp::sync_read_raw_async timed out\n" ;
			 
			e = boost::system::error_code(boost::asio::error::timed_out) ;
			return 0 ; // time out 
		}
	}*/

	void sync_connect_host(const std::string & host_ , // IP v4 address
		const std::string & service_, boost::system::error_code &ec_ )
	{ 
		stopped = false ;
		boost::system::error_code error = boost::asio::error::host_not_found;

		boost::asio::ip::tcp::resolver resolver(
			io_s_);	
		boost::asio::ip::tcp::resolver::query query(host_, service_);		
		boost::asio::ip::tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
		boost::asio::ip::tcp::resolver::iterator end;
		
		
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
			//std::cerr << e_.what() <<endl ;
			//ec_ = boost::system::error_code(e_) ;
			std::cerr << "connection_tcp: sync_connect_host exception --" << e_.what() << std::endl ;
			ec_ = boost::asio::error::connection_aborted ;
		}
		return ;
	}
	
	
	void async_connect_host(const std::string & host_ , const std::string & service_, 
		int timeout_ms, boost::system::error_code &ec_ )
	{
		stopped = false ;
		if (verbose >1 )
			std::cout << "connection_tcp::async_connection_host " << host_ << ":" << service_<< std::endl ;
			
		boost::system::error_code error = boost::asio::error::host_not_found;

		boost::asio::ip::tcp::resolver resolver(io_s_);	
		boost::asio::ip::tcp::resolver::query query(host_, service_);		
		boost::asio::ip::tcp::resolver::iterator endpoint_iterator = resolver.resolve(query, ec_);
		boost::asio::ip::tcp::resolver::iterator end ;
		io_s_.reset() ; 
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
			ec_ = boost::asio::error::connection_aborted ;
		//	ec_ = boost::system::error_code(e_) ;
			return ;
		}
	}
	
	void close_and_reconnect_sync(const std::string & host_ , const std::string & service_, 
		boost::system::error_code &ec_)
	{
		close() ;
		socket_p.reset(new boost::asio::ip::tcp::socket(io_s_));
		sync_connect_host(host_, service_, ec_) ;
	}
	
	void close_and_reconnect_async(const std::string & host_ , const std::string & service_, int timeoutms, 
		boost::system::error_code &ec_)
	{
		close() ;
		socket_p.reset(new boost::asio::ip::tcp::socket(io_s_));
		async_connect_host(host_, service_,timeoutms, ec_) ;
	}
	
	
			
	boost::asio::io_service & 
		io_service() const {return io_s_ ;}
	
	void setverbose_level(const int &verbose_) {
		verbose = verbose_ ;
	}
private:
	
	void start_connect(boost::asio::ip::tcp::resolver::iterator endpoint_iterator, int timeoutms)
	{
		if (endpoint_iterator != boost::asio::ip::tcp::resolver::iterator())
		{
			if(verbose > 1)
				std::cout << "Trying " << endpoint_iterator->endpoint() << "...\n";

			// Set a deadline for the connect operation.
			dtimer.expires_from_now(boost::posix_time::milliseconds(timeoutms));
			dtimer.async_wait(boost::bind(&connection_tcp::set_resulttimer, this));
		  // Start the asynchronous connect operation.
			socket_p->async_connect(endpoint_iterator->endpoint(),
			  boost::bind(&connection_tcp::handle_connect,
				this, _1, endpoint_iterator, timeoutms));
		}
		else
		{
			// There are no more endpoints to try. Shut down the client.
			stop();
		}			
	}	
  
   void handle_connect(const boost::system::error_code& ec,
      boost::asio::ip::tcp::resolver::iterator endpoint_iterator, int timeoutms)
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

	
	void set_result(boost::optional<boost::system::error_code>* a, const boost::system::error_code& b, size_t & sizeread, const size_t transferred) 
	{ 
		a->reset(b); 
		sizeread = transferred ;
	} 
	
	void set_resulttimerread(boost::optional<boost::system::error_code> *a, const boost::system::error_code& b) 
	{
		a->reset(b) ;
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
		if (dtimer.expires_at() <= boost::asio::deadline_timer::traits_type::now())
		{
			// The deadline has passed. The socket is closed so that any outstanding
			// asynchronous operations are cancelled.
			if(verbose > 1)
				std::cout << "connection_tcp::set_resulttimer -- timer time out, timer set to infinity "<<  std::endl ;
			socket_p->close();

			// There is no longer an active deadline. The expiry is set to positive
			// infinity so that the actor takes no action until a new deadline is set.
		//	dtimer.expires_at(boost::posix_time::pos_infin);
			//if(verbose > 1)
				//std::cout << "connection_tcp::set_resulttimer -- timer time out, timer set to infinity -- done! "<<  std::endl ;
		}

		// Put the actor back to sleep.
	//	dtimer.async_wait(boost::bind(&connection_tcp::set_resulttimer, this));
	} 
  	boost::system::error_code ecret; 
	std::string hostname ; std::string service ;
	//boost::asio::ip::tcp::acceptor acceptor_;
	/// The underlying socket.
	bool stopped ;
	//boost::asio::ip::tcp::socket socket_;
#ifdef _HAS_UNIQUE_PTR
	std::unique_ptr<boost::asio::ip::tcp::socket> socket_p;
#else
	boost::scoped_ptr<boost::asio::ip::tcp::socket> socket_p;
#endif
	boost::asio::io_service &io_s_ ;
	boost::asio::deadline_timer dtimer  ;
	int verbose ;
};



// The connection class provides raw primitives on top of a socket.
class connection_tcp_server
{
public:
  /// Constructor.
	connection_tcp_server(boost::asio::io_service& io_service, int port)
	: socket_p(new tcp::socket(io_service)),
	  io_s_(io_service), stopped(false), 
	  dtimer(io_s_), 
	  port_(port), 
	  acceptor_(io_service, tcp::endpoint(tcp::v4(), port))
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
	
	void write_msg(boost::system::error_code &e , 
		 const void *m_, const size_t &size)
	{
		if(!stopped) 
			socket_p->write_some(boost::asio::buffer(m_, size), e);
		else 
			e = boost::system::error_code(boost::asio::error::not_connected) ;
		return ;
	}


	void set_option_nonblock(bool nonblock = true)
	{
		boost::asio::socket_base::non_blocking_io command(nonblock);
		socket_p->io_control(command);
		return ;
	}
	void set_option_keepalive(bool keepalive= true)
	{
		boost::asio::socket_base::keep_alive option(keepalive);
		socket_p->set_option(option);
	}

	/// \brief read_msg:wrapped by sync_read
	/// \brief using the low level socket.read_some function
	/// \param e: INOUT the error code
	/// \param returns > 0 when success, 0 no data 
	int sync_read_raw(boost::system::error_code& e, void *buf, size_t size)
	{
		
		boost::system::error_code err_ ;
		do {
			size_t len = socket_p->read_some(boost::asio::buffer(buf, size) ,
				err_) ;
			e = err_ ;
			if(!err_)
			{
				return (int)len;
			}
			else if(err_ == boost::system::error_code(boost::asio::error::would_block))
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
		boost::system::error_code &ec_ )
	{
		stopped = false ;
		
		
		try  {
			acceptor_.accept(*socket_p, ec_) ;
			if(ec_)
				throw boost::system::system_error(ec_) ;
			
		}
		catch(const std::exception & e_)
		{
			//std::cerr << e_.what() <<endl ;
			//ec_ = boost::system::error_code(e_) ;
			std::cerr << "connection_tcp_server::sync_accept_host exception --" << e_.what() << std::endl ;
			ec_ = boost::asio::error::connection_aborted ;
		}
		return ;
	}
	
	void async_accept_host(const std::string & host_ , 
		int timeout_ms, boost::system::error_code &ec_ )
	{
		stopped = false ;
		if (verbose >1 )
			std::cout << "connection_tcp_server::async_accept_host " << host_ << std::endl ;
			
		boost::system::error_code error = boost::asio::error::host_not_found;
		io_s_.reset() ; 
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
			ec_ = boost::asio::error::connection_aborted ;
		//	ec_ = boost::system::error_code(e_) ;
			return ;
		}
	}
	
		
	void close_and_reaccept_async(const std::string & host_ , int timeoutms, 
		boost::system::error_code &ec_)
	{
		close() ;
		socket_p.reset(new boost::asio::ip::tcp::socket(io_s_));
		async_accept_host(host_, timeoutms, ec_) ;
	}

	void close_and_reaccept_sync(const std::string & host_ , 
		boost::system::error_code &ec_)
	{
		close() ;
		socket_p.reset(new boost::asio::ip::tcp::socket(io_s_));
		sync_accept_host(host_, ec_) ;
	}
	
	
	
			
	boost::asio::io_service & 
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
		dtimer.expires_from_now(boost::posix_time::milliseconds(timeoutms));
		dtimer.async_wait(boost::bind(&connection_tcp_server::set_resulttimer, this));
		// Start the asynchronous connect operation.
		acceptor_.async_accept(*socket_p,
				boost::bind(&connection_tcp_server::handle_accept, this, _1, timeoutms));
		/*socket_p->async_connect(endpoint_iterator->endpoint(),
			boost::bind(&connection_tcp::handle_connect,
			this, _1, endpoint_iterator, timeoutms)); */
	}	
  
   void handle_accept(const boost::system::error_code& ec,
      int timeoutms)
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
				std::cerr << "connection_tcp_server::handle_accept -- Connect timed out" << std::endl ;

			//acceptor_.cancel() ;
			// Try the next available endpoint.
			//start_connect(++endpoint_iterator, timeoutms);
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
		if (dtimer.expires_at() <= boost::asio::deadline_timer::traits_type::now())
		{
			// The deadline has passed. The socket is closed so that any outstanding
			// asynchronous operations are cancelled.
			acceptor_.cancel() ;
			if(verbose > 1)
				std::cout << "connection_tcp_server::set_resulttimer -- timer time out"<<  std::endl ;
			socket_p->close();
			
		}

		// Put the actor back to sleep.
	//	dtimer.async_wait(boost::bind(&connection_tcp::set_resulttimer, this));
	} 
  	boost::system::error_code ecret; 
	std::string hostname ; std::string service ;
	//boost::asio::ip::tcp::acceptor acceptor_;
	/// The underlying socket.
	bool stopped ;
	//boost::asio::ip::tcp::socket socket_;
#ifdef _HAS_UNIQUE_PTR
	std::unique_ptr<boost::asio::ip::tcp::socket> socket_p;
#else
	boost::scoped_ptr<boost::asio::ip::tcp::socket> socket_p;
#endif
	
	boost::asio::io_service &io_s_ ;
	boost::asio::deadline_timer dtimer  ;
	int verbose ;
	int port_ ;
	tcp::acceptor acceptor_; 
};



} // namespace 

#endif // SERIALIZATION_CONNECTION_HPP

