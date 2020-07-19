// TCP connection: acceptor and tcp client connection
// connection.hpp
// ~~~~~~~~~~~~~~
// Author: Liping Zhang
/// 2008
#ifndef _COMMONLIBS_CONNECTION_HPP
#define _COMMONLIBS_CONNECTION_HPP

#include <boost/array.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>

#include <boost/optional.hpp>
#include <iomanip>
#include <string>
#include <vector>
#include "commonlibs/probemessage.hpp" 
#include <boost/signal.hpp>
#include <boost/foreach.hpp>
#include "commonlibs/errorstatus.hpp"
#include <memory>
#include "commonlibs/sleeprelative.hpp"
#ifdef __GNUC__
#  include <features.h>
#  if __GNUC_PREREQ(4,4)
	#define _HAS_UNIQUE_PTR
#  else
	#include <boost/scoped_ptr.hpp>
//       Else
#  endif
#else
//	assume windows has c++0x support (come on, install it!) 
	#define _HAS_UNIQUE_PTR 
#endif

using namespace boost::asio::ip ;

namespace commonlibs {



// The connection class provides serialization primitives on top of a socket.
template <class Tmessage,class TmessageParser>
class connection_imp_template
{
public:
  /// Constructor.
	connection_imp_template(boost::asio::io_service& io_service)
	: socket_(io_service) , io_s_(io_service)
	{
	}
public:
	~connection_imp_template() 
	{}
	/// Get the underlying socket. Used for making a connection or for accepting
	/// an incoming connection.
	boost::asio::ip::tcp::socket& socket()
	{
		return socket_;
	}
	
	void close() 
	{
		try {
			socket_.close() ;
		}
		catch(const std::exception &e_)
		{
			std::cerr <<  e_.what() <<endl ;
		}
	}


	void write_msg(boost::system::error_code &e , 
		 const Tmessage &m_)
	{
		socket_.write_some(boost::asio::buffer(m_.getmsg()), e);
		return ;
	}


	void set_option_nonblock()
	{
		boost::asio::socket_base::non_blocking_io command(true);
		socket_.io_control(command);
		return ;
	}
	void set_option_keepalive()
	{
		boost::asio::socket_base::keep_alive option(true);
		socket_.set_option(option);
	}

	/// \brief read_msg:wrapped by sync_read
	/// \brief using the low level socket.read_some function
	/// \param e: INOUT the error code
	/// \param returns > 0 when success
	int read_msg(boost::system::error_code& e, bool nonblock= false)
	{

		boost::system::error_code err_ ;
		int timec_ = 0 ;
		do {
			size_t len = socket_.read_some(boost::asio::buffer(inbound_data_) ,
				err_) ;
			e = err_ ;
			if(!err_)
			{
				if(len > (size_t)0) 
					return (int)len;
				else
					return 0 ;
			}
			else if(err_ == boost::system::error_code(boost::asio::error::would_block) && 
					nonblock == false)
			{
				sleep_relative_ms(100) ;
			}
			else 
				break ;
		}
		while(timec_ ++ < 10 * MAX_WAIT_SEC 
			 && nonblock == false) ;
		if(timec_ >= MAX_WAIT_SEC)
			err_ = boost::system::error_code(boost::asio::error::timed_out) ;
		return 0 ;
	}
	/// \brief sync_read:read some bytes the socket, non  blocking way
	/// \brief deals with the flow control, when a flow control msg received,
	/// \brief sends the ack to client automatically, parse the other msgs
	/// \param e: INOUT the error code, pm:OUTPUT, buffer contains the msgs when return
	/// \param returns > 0 when success
	int sync_read(boost::system::error_code& e , std::vector<Tmessage> &pm ,
		bool b_quiet = false ,bool nonblock = false ) 
	{
		size_t len = //socket_.read_some(boost::asio::buffer(inbound_data_), e);
			read_msg(e, nonblock) ;
		if(!e)
		{
			int nm_=	
				(int)mp_.parse2Message(inbound_data_.elems, (int)len , pm) ;
			return nm_ ;
		}  // if !e  (no error) 
		else 
		{
			if(!nonblock || (nonblock && e!= 
				boost::system::error_code(boost::asio::error::would_block)) 
				)
				throw boost::system::system_error(e); 
			else 
				return 0 ;
		}
	}


	/// \brief sync_read_raw: simplified version of sync_read, 
	///	\brief returns number of bytes received instead of number of msgs
	int sync_read_raw(boost::system::error_code& e , const char * &p_c ,
		bool b_quiet = false ,bool nonblock = false) 
	{
		size_t len = //socket_.read_some(boost::asio::buffer(inbound_data_), e);
			read_msg(e, nonblock) ;
		if(!e)
		{
			p_c = reinterpret_cast<const char *>(inbound_data_.elems) ;
			return (int)len ;
		}  // if !e  (no error) 
		else 
		{
			if(!nonblock || (nonblock && e!= 
				boost::system::error_code(boost::asio::error::would_block)) 
				)
				throw boost::system::system_error(e); 
			else 
				return 0 ;
		}
	}

	boost::asio::io_service & io_service() {return io_s_ ;}

public:
	enum packetsize {MAX_PACKET_SIZE = 65536 * 100 } ;
	enum  {MAX_WAIT_SEC = 100 } ;
private:
	//boost::asio::ip::tcp::acceptor acceptor_;
	/// The underlying socket.
	boost::asio::ip::tcp::socket socket_;

	/// Holds the inbound data.
	boost::array<unsigned char , MAX_PACKET_SIZE> inbound_data_;
	TmessageParser mp_ ;
	boost::asio::io_service &io_s_ ;
};

template< class Tmessage, class TmessageParser>  
class connection_template : public connection_imp_template <Tmessage, TmessageParser>
{
public:

	connection_template(boost::asio::io_service &io_service)
		: connection_imp_template<Tmessage, TmessageParser>(io_service)
	{
	}

	void sync_connect_host(const std::string & host_ , // IP v4 address
		const std::string & service_, boost::system::error_code &ec_ )
	{ 
		boost::system::error_code error = boost::asio::error::host_not_found;

		boost::asio::ip::tcp::resolver resolver(
			connection_imp_template<Tmessage, TmessageParser>
			::io_service());	
		boost::asio::ip::tcp::resolver::query query(host_, service_);		
		boost::asio::ip::tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
		boost::asio::ip::tcp::resolver::iterator end;
		try  {
			while (error && endpoint_iterator != end)
			{
				connection_imp_template<Tmessage, TmessageParser>
					::socket().close();
				connection_imp_template<Tmessage, TmessageParser>
					::socket().
					connect(*endpoint_iterator++, error);
			}
		}
		catch(const std::exception & e_)
		{
			std::cerr << e_.what() <<endl ;
			ec_ = error ;
			return ;
		}
		ec_ = error ;
	}
	template<typename Handler>
	void async_connect_host(const std::string & host_ , const std::string & service_, Handler handler)
	{
		boost::system::error_code error = boost::asio::error::host_not_found;

		boost::asio::ip::tcp::resolver resolver(
			connection_imp_template<Tmessage, TmessageParser>
				::io_service());	
		boost::asio::ip::tcp::resolver::query query(host_, service_);		
		boost::asio::ip::tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
		boost::asio::ip::tcp::resolver::iterator end;
		try  {
			while (error && endpoint_iterator != end)
			{
			  connection_imp_template<Tmessage, TmessageParser>
				  ::socket().close();
			  connection_imp_template<Tmessage, TmessageParser>
				  ::socket().
				  async_connect(*endpoint_iterator++, handler);
			}
		}
		catch(const std::exception & e_)
		{
			std::cerr << e_.what() <<endl ;
			return ;
		}
	}

} ;

typedef connection_imp_template<commonlibs::probemessage,
	commonlibs::messageParser> connection_imp ;
typedef connection_template<commonlibs::probemessage, 
	commonlibs::messageParser> connection ;

typedef connection_template<commonlibs::probemessage, 
	commonlibs::textlineParser> connection_tl ;


///	\brief acceptor
///	\brief signalrcver should provide two services:
///	\brief (1) operator (const vpm &) that receives the packages in format vector<probemessage>
///	\brief (2) operator (vpm, bool ) // that send the pm to the client bool indicates if the 
///	message should be resent next time or not
template<class tcp_acceptor_implementor>
class tcp_acceptor 
{
public:
	tcp_acceptor(
		boost::asio::io_service &io_service, 
		const int service
		) :	
		acceptor_(io_service,
		    boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), service)),
		timer_(io_service) 
	{
		finished = false ;
		b_quiet = false ;
		b_closed = true ;
		return ;
	}
	
	void close()
	{
		if(conn)
			conn->socket().close() ;
		b_closed = true ;
	}
	void async_accept_and_forward(int timeout = -1)
	{
		do {
			try {
				conn = boost::shared_ptr<
					connection_imp_template<probemessage,
					commonlibs::messageParser> >
					(new connection_imp_template
						<probemessage,commonlibs::messageParser>
						(acceptor_.get_io_service()));
				acceptor_.async_accept(conn->socket(),
					boost::bind(&tcp_acceptor::handle_accept_and_forward, this,
					  boost::asio::placeholders::error));
				// Start an asynchronous connect operation.
				if(timeout > 0)
				{
				    timer_.expires_from_now(boost::posix_time::seconds(timeout));
					timer_.async_wait(boost::bind(&tcp_acceptor::close, this));
				}
				acceptor_.get_io_service().run() ;
				if(!finished)
					sleep_relative_ms(1000) ;
			}
			catch(const  std::exception & e_)
			{
				std::cerr << "Acceptor asyn connect: " <<e_.what() <<endl ;
			}
			//sleep for a while
		} while (!finished) ;
	}
	void async_accept_and_open(int timeout = -1)
	{
		p_t_ = boost::shared_ptr<boost::thread>(new boost::thread(
			boost::bind(&tcp_acceptor::async_accept_and_open_thread, this, timeout))) ;
		finished = false ;
	}
	void async_accept_and_open_thread(int timeout = -1)
	{
		
		try {
			conn = boost::shared_ptr<connection_imp>
				(new connection_imp(acceptor_.get_io_service()));
			acceptor_.async_accept(conn->socket(),
				boost::bind(&tcp_acceptor::handle_accept, this,
				  boost::asio::placeholders::error));
			// Start an asynchronous connect operation.
			if(timeout > 0)
			{
			    timer_.expires_from_now(boost::posix_time::seconds(timeout));
				timer_.async_wait(boost::bind(&tcp_acceptor::close, this));
			}
			acceptor_.get_io_service().run() ;
			if(!finished)
				sleep_relative_ms(1000) ;
		}
		catch(const  std::exception & e_)
		{
			std::cerr << "Acceptor asyn connect: " <<e_.what() <<endl ;
		}
	}
	/// \brief Handle completion of a accept operation ( or an error of a connection).
	/// \brief Main loop of session, if a accept success , then starts communicating
	/// \brief otherwise, close the connection and re-connect it. 
	/// \param e : system error code, IN 
	void handle_accept(const boost::system::error_code& e)
	{
		b_closed = false ;
		try 
		{
			//unsigned char uc_ [128] ;
			if (!e)
			{
				// set to non block read
				conn->set_option_nonblock() ; //
				conn->set_option_keepalive() ;
				boost::system::error_code error;
				while(!error && !finished)
				{
					static_cast<tcp_acceptor_implementor*>(this) -> 
						handle_connected(error) ;
				} // while !e 
			} // if !e
		}// try
		catch(const std::exception & e_)
		{
			std::cerr << "TCP acceptor Error in asynchronized writing: " 
				<< e_.what() << endl ;
		}
		////NEED TO be changed
		if(!finished)
		{
			// An error occurred. close and reconnect  
			if(e)
			{
				std::cerr << "Connection error (acceptor): " << e.message() << std::endl;
				//throw boost::system::system_error(e) ;
			}
			close() ;
			conn = boost::shared_ptr<connection_imp>(new connection_imp(acceptor_.get_io_service()));

			acceptor_.async_accept(conn->socket(),
				boost::bind(&tcp_acceptor::handle_accept, this,
				  boost::asio::placeholders::error));

		} // if !finished 
		else
		{ 
			close() ;
		}
	} // handle_accept
	void setFinished() { finished = true ;} 
public:
	bool finished ; 
	bool b_quiet ;
	boost::asio::ip::tcp::acceptor acceptor_;
	boost::asio::deadline_timer timer_;
	boost::shared_ptr<connection_imp_template<probemessage,
					commonlibs::messageParser> > conn ;
	boost::shared_ptr<boost::thread> p_t_ ;
	bool b_closed ; 
} ;

typedef boost::shared_ptr<connection> connection_ptr;

template <class connector_implementor, class Tconnection = connection, class Tmsgsaver = simple_saver>
class synchronized_connector 
{
public:
	synchronized_connector(boost::asio::io_service &ios_ ,
		const std::string &host_ ,
		const std::string &service_, 
		bool start_session = true) 
		: 
		hostname_ (host_) , 
		servicename_ (service_)
	{
		try 
		{
			b_connected = false ;
			finished = false ;
			b_quiet = false ;
			sptr_conn_ = boost::shared_ptr<Tconnection>(new Tconnection(ios_)) ;
			if(start_session)
			{
				p_t_ = boost::shared_ptr<boost::thread>(new boost::thread(
						boost::bind(&synchronized_connector::start_imp, this)));
			}
		}
		catch(const std::exception &e_)
		{
			msgsaver.set_errstatus(commonlibs::STATUS_COMM_ERROR, 
				e_.what()) ;
			std::cerr << e_.what() <<endl ;
		}
	}


	void start_session_explicit()
	{
		p_t_ = boost::shared_ptr<boost::thread>(new boost::thread(
				boost::bind(&synchronized_connector::start_imp, this)));
	}

	
	void start (unsigned int sleepms = 20000)
	{
		do {
			try {
				boost::system::error_code ec_ ;
				sptr_conn_->sync_connect_host(hostname_ ,
					servicename_,
					ec_) ;

				b_connected = true ;
				handle_connect(ec_) ;
				sleep_relative_ms((int)sleepms) ;
			}
			catch(const  std::exception & e_)
			{
				msgsaver.set_errstatus(commonlibs::STATUS_COMM_ERROR, 
					e_.what() + std::string("--synchronized_connector::start()")) ;
				std::cerr << e_.what() << std::string("--synchronized_connector::start()") << std::endl ;
				msgsaver.set_errstatus(commonlibs::STATUS_COMM_ERROR, 
					std::string("Closing ... ")  + std::string("--synchronized_connector::start()")) ;
				try {
					close() ;
				}
				catch(...)
				{
				}
			}
			//sleep for a while
		} while (finished == false) ;
	}

	~synchronized_connector() 
	{	
		try {
			close_connection() ;
		}
		catch(const std::exception &e_)
		{
			std::cerr <<e_.what() <<endl ;
		}
	}

	bool is_connected () const {
		return b_connected ; 
	}

	void close()
	{
		if(sptr_conn_) 
			sptr_conn_->socket().close() ;

		b_connected = false ;
	}

	/// \brief Handle completion of a connect operation ( or an error of a connection).
	/// \brief Main loop of session, if a connecion success , then starts communicating
	/// \brief otherwise, close the connection and re-connect it. 
	/// \brief When the "stopped" flag is set, the procedure will stop the connection
	/// \brief when the "finished" flag is set, the session will terminate
	/// \param e : system error code, INOUT 
  void handle_connect(const boost::system::error_code& e)
  {
		try 
		{
			if (!e)
			{
				// set to non block read
				sptr_conn_->set_option_nonblock() ; //
				sptr_conn_->set_option_keepalive() ;
			  // Successfully established connection. 
				boost::system::error_code error;
				while(!finished) 
				{
					static_cast<connector_implementor*>(this)->handle_connected(error) ;
					if(error)
						throw boost::system::system_error(error) ;
					msgsaver.set_errstatus(commonlibs::STATUS_OK, 
						"") ;
				}
			} // if !e
			else {
				// inform the error happened
				boost::system::error_code error = e ;
				static_cast<connector_implementor*>(this)->handle_connected(error) ;
			}
		}// try
		catch(const std::exception & e_)
		{
			msgsaver.set_errstatus(commonlibs::STATUS_COMM_ERROR, 
				e_.what() + std::string("--syncrhonized_connector(") + hostname_ + 
				":" + this->servicename_ + 
				"::handle_connect()" ) ;
			std::cerr << hostname_ << ":" << e_.what() << "--syncrhonized_connector::handle_connect()"  << endl ;
		}
		if(!finished)
		{
			// An error occurred. close and reconnect  
			if(e)
			{
				boost::system::system_error se(e) ;
				msgsaver.set_errstatus(commonlibs::STATUS_COMM_ERROR, 
					hostname_ + ":" + 
					this->servicename_ + " - " + 
					se.what()) ;
				//std::cerr << e.message() << std::endl;
			}
			try {
				close() ;
				commonlibs::sleep_relative_ms(100) ;
			}
			catch(const std::exception &e_)
			{
				std::cerr << "Error in closing: " << e_.what() <<endl ;
			}
		}
		else   // finished the session stops
		{
			try {
				close() ;
				b_connected = false ;
			}
			catch(const std::exception &e_)
			{
				std::cerr << "Error in closing : " << e_.what() << endl ;
			}
		}
  }

  void write_probemessage(boost::system::error_code &e, const probemessage &pm_)
  {
	  if(sptr_conn_)
	  {
		  sptr_conn_->write_msg(e, pm_) ;
	  }
  }

  int conn_sync_read(boost::system::error_code &e, std::vector<probemessage> &vpm, bool q)
  {
	  return sptr_conn_->sync_read(e, vpm,q) ;
  }

  const std::string &get_hostname() const 
  {
	  return hostname_ ;
  }
  const std::string &get_servicename() const {
	  return servicename_ ;
  }

  void setFinished() { finished = true ; } 

  void close_connection()
  {
		try {
			std::cout << hostname_ << " - Exiting... , closing socket " << std::endl ;
			close() ;
			sleep_relative_ms(100) ;
			unsigned int temp_ = 0 ;
			while(!b_connected && ++ temp_ < 10)
			{
				sleep_relative_ms(100) ;
			}
		}catch(...)
		{
		}
  }
protected:
	boost::shared_ptr<Tconnection>	sptr_conn_ ;
private:
	void start_imp()
	{
		this->start() ;
	}
	std::string hostname_ ;
	std::string servicename_ ;
	bool finished ; 
	bool b_quiet ;
	bool b_connected ;
	boost::shared_ptr<boost::thread> p_t_ ;
protected:

	Tmsgsaver msgsaver ;
} ;


} // namespace 

#endif // SERIALIZATION_CONNECTION_HPP

