// TCP connection: acceptor and tcp client connection
// connection.hpp
// ~~~~~~~~~~~~~~
// Author: Liping Zhang
/// 2008
#ifndef _COMMONLIBS_CONNECTION_HPP
#define _COMMONLIBS_CONNECTION_HPP

#include <asio.hpp>
#include <array>
#include <functional>
#include <iomanip>
#include <memory>
#include <string>
#include <thread>
#include <vector>
#include "commonlibs/probemessage.hpp"
#include "commonlibs/errorstatus.hpp"
#include "commonlibs/sleeprelative.hpp"

using namespace asio::ip ;

namespace commonlibs {



// The connection class provides serialization primitives on top of a socket.
template <class Tmessage,class TmessageParser>
class connection_imp_template
{
public:
  /// Constructor.
	connection_imp_template(asio::io_context& io_service)
	: socket_(io_service) , io_s_(io_service)
	{
	}
public:
	~connection_imp_template()
	{}
	/// Get the underlying socket. Used for making a connection or for accepting
	/// an incoming connection.
	asio::ip::tcp::socket& socket()
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
			std::cerr <<  e_.what() << std::endl ;
		}
	}


	void write_msg(asio::error_code &e ,
		 const Tmessage &m_)
	{
		socket_.write_some(asio::buffer(m_.getmsg()), e);
		return ;
	}


	void set_option_nonblock()
	{
		socket_.non_blocking(true);
		return ;
	}
	void set_option_keepalive()
	{
		asio::socket_base::keep_alive option(true);
		socket_.set_option(option);
	}

	/// \brief read_msg:wrapped by sync_read
	/// \brief using the low level socket.read_some function
	/// \param e: INOUT the error code
	/// \param returns > 0 when success
	int read_msg(asio::error_code& e, bool nonblock= false)
	{

		asio::error_code err_ ;
		int timec_ = 0 ;
		do {
			size_t len = socket_.read_some(asio::buffer(inbound_data_) ,
				err_) ;
			e = err_ ;
			if(!err_)
			{
				if(len > (size_t)0)
					return (int)len;
				else
					return 0 ;
			}
			else if(err_ == asio::error_code(asio::error::would_block) &&
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
			err_ = asio::error_code(asio::error::timed_out) ;
		return 0 ;
	}
	/// \brief sync_read:read some bytes the socket, non  blocking way
	/// \brief deals with the flow control, when a flow control msg received,
	/// \brief sends the ack to client automatically, parse the other msgs
	/// \param e: INOUT the error code, pm:OUTPUT, buffer contains the msgs when return
	/// \param returns > 0 when success
	int sync_read(asio::error_code& e , std::vector<Tmessage> &pm ,
		bool b_quiet = false ,bool nonblock = false )
	{
		size_t len = read_msg(e, nonblock) ;
		if(!e)
		{
			int nm_=
				(int)mp_.parse2Message(inbound_data_.data(), (int)len , pm) ;
			return nm_ ;
		}  // if !e  (no error)
		else
		{
			if(!nonblock || (nonblock && e!=
				asio::error_code(asio::error::would_block))
				)
				throw asio::system_error(e);
			else
				return 0 ;
		}
	}


	/// \brief sync_read_raw: simplified version of sync_read,
	///	\brief returns number of bytes received instead of number of msgs
	int sync_read_raw(asio::error_code& e , const char * &p_c ,
		bool b_quiet = false ,bool nonblock = false)
	{
		size_t len = read_msg(e, nonblock) ;
		if(!e)
		{
			p_c = reinterpret_cast<const char *>(inbound_data_.data()) ;
			return (int)len ;
		}  // if !e  (no error)
		else
		{
			if(!nonblock || (nonblock && e!=
				asio::error_code(asio::error::would_block))
				)
				throw asio::system_error(e);
			else
				return 0 ;
		}
	}

	asio::io_context & io_service() {return io_s_ ;}

public:
	enum packetsize {MAX_PACKET_SIZE = 65536 * 100 } ;
	enum  {MAX_WAIT_SEC = 100 } ;
private:
	/// The underlying socket.
	asio::ip::tcp::socket socket_;

	/// Holds the inbound data.
	std::array<unsigned char , MAX_PACKET_SIZE> inbound_data_;
	TmessageParser mp_ ;
	asio::io_context &io_s_ ;
};

template< class Tmessage, class TmessageParser>
class connection_template : public connection_imp_template <Tmessage, TmessageParser>
{
public:

	connection_template(asio::io_context &io_service)
		: connection_imp_template<Tmessage, TmessageParser>(io_service)
	{
	}

	void sync_connect_host(const std::string & host_ , // IP v4 address
		const std::string & service_, asio::error_code &ec_ )
	{
		asio::error_code error = asio::error::host_not_found;

		asio::ip::tcp::resolver resolver(
			connection_imp_template<Tmessage, TmessageParser>
			::io_service());
		asio::ip::tcp::resolver::query query(host_, service_);
		asio::ip::tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
		asio::ip::tcp::resolver::iterator end;
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
			std::cerr << e_.what() << std::endl ;
			ec_ = error ;
			return ;
		}
		ec_ = error ;
	}
	template<typename Handler>
	void async_connect_host(const std::string & host_ , const std::string & service_, Handler handler)
	{
		asio::error_code error = asio::error::host_not_found;

		asio::ip::tcp::resolver resolver(
			connection_imp_template<Tmessage, TmessageParser>
				::io_service());
		asio::ip::tcp::resolver::query query(host_, service_);
		asio::ip::tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
		asio::ip::tcp::resolver::iterator end;
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
			std::cerr << e_.what() << std::endl ;
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
		asio::io_context &io_service,
		const int service
		) :
		io_s_(io_service),
		acceptor_(io_service,
		    asio::ip::tcp::endpoint(asio::ip::tcp::v4(), service)),
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
				conn = std::shared_ptr<
					connection_imp_template<probemessage,
					commonlibs::messageParser> >
					(new connection_imp_template
						<probemessage,commonlibs::messageParser>
						(io_s_));
				acceptor_.async_accept(conn->socket(),
					[this](const asio::error_code &e) {
						handle_accept_and_forward(e);
					});
				// Start an asynchronous connect operation.
				if(timeout > 0)
				{
				    timer_.expires_after(std::chrono::seconds(timeout));
					timer_.async_wait([this](const asio::error_code &) { close(); });
				}
				io_s_.run() ;
				if(!finished)
					sleep_relative_ms(1000) ;
			}
			catch(const  std::exception & e_)
			{
				std::cerr << "Acceptor asyn connect: " << e_.what() << std::endl ;
			}
			//sleep for a while
		} while (!finished) ;
	}
	void async_accept_and_open(int timeout = -1)
	{
		auto t = std::make_shared<std::thread>(
			[this, timeout]() { async_accept_and_open_thread(timeout); });
		t->detach();
		p_t_ = t;
		finished = false ;
	}
	void async_accept_and_open_thread(int timeout = -1)
	{

		try {
			conn = std::shared_ptr<connection_imp>
				(new connection_imp(io_s_));
			acceptor_.async_accept(conn->socket(),
				[this](const asio::error_code &e) {
					handle_accept(e);
				});
			// Start an asynchronous connect operation.
			if(timeout > 0)
			{
			    timer_.expires_after(std::chrono::seconds(timeout));
				timer_.async_wait([this](const asio::error_code &) { close(); });
			}
			io_s_.run() ;
			if(!finished)
				sleep_relative_ms(1000) ;
		}
		catch(const  std::exception & e_)
		{
			std::cerr << "Acceptor asyn connect: " << e_.what() << std::endl ;
		}
	}
	/// \brief Handle completion of a accept operation ( or an error of a connection).
	/// \brief Main loop of session, if a accept success , then starts communicating
	/// \brief otherwise, close the connection and re-connect it.
	/// \param e : system error code, IN
	void handle_accept(const asio::error_code& e)
	{
		b_closed = false ;
		try
		{
			if (!e)
			{
				// set to non block read
				conn->set_option_nonblock() ; //
				conn->set_option_keepalive() ;
				asio::error_code error;
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
				<< e_.what() << std::endl ;
		}
		////NEED TO be changed
		if(!finished)
		{
			// An error occurred. close and reconnect
			if(e)
			{
				std::cerr << "Connection error (acceptor): " << e.message() << std::endl;
			}
			close() ;
			conn = std::shared_ptr<connection_imp>(new connection_imp(io_s_));

			acceptor_.async_accept(conn->socket(),
				[this](const asio::error_code &ec) { handle_accept(ec); });

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
	asio::io_context &io_s_;
	asio::ip::tcp::acceptor acceptor_;
	asio::steady_timer timer_;
	std::shared_ptr<connection_imp_template<probemessage,
					commonlibs::messageParser> > conn ;
	std::shared_ptr<std::thread> p_t_ ;
	bool b_closed ;
} ;

typedef std::shared_ptr<connection> connection_ptr;

template <class connector_implementor, class Tconnection = connection, class Tmsgsaver = simple_saver>
class synchronized_connector
{
public:
	synchronized_connector(asio::io_context &ios_ ,
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
			sptr_conn_ = std::shared_ptr<Tconnection>(new Tconnection(ios_)) ;
			if(start_session)
			{
				auto t = std::make_shared<std::thread>(
					[this]() { start_imp(); });
				t->detach();
				p_t_ = t;
			}
		}
		catch(const std::exception &e_)
		{
			msgsaver.set_errstatus(commonlibs::STATUS_COMM_ERROR,
				e_.what()) ;
			std::cerr << e_.what() << std::endl ;
		}
	}


	void start_session_explicit()
	{
		auto t = std::make_shared<std::thread>([this]() { start_imp(); });
		t->detach();
		p_t_ = t;
	}


	void start (unsigned int sleepms = 20000)
	{
		do {
			try {
				asio::error_code ec_ ;
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
			std::cerr << e_.what() << std::endl ;
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
  void handle_connect(const asio::error_code& e)
  {
		try
		{
			if (!e)
			{
				// set to non block read
				sptr_conn_->set_option_nonblock() ; //
				sptr_conn_->set_option_keepalive() ;
			  // Successfully established connection.
				asio::error_code error;
				while(!finished)
				{
					static_cast<connector_implementor*>(this)->handle_connected(error) ;
					if(error)
						throw asio::system_error(error) ;
					msgsaver.set_errstatus(commonlibs::STATUS_OK,
						"") ;
				}
			} // if !e
			else {
				// inform the error happened
				asio::error_code error = e ;
				static_cast<connector_implementor*>(this)->handle_connected(error) ;
			}
		}// try
		catch(const std::exception & e_)
		{
			msgsaver.set_errstatus(commonlibs::STATUS_COMM_ERROR,
				e_.what() + std::string("--syncrhonized_connector(") + hostname_ +
				":" + this->servicename_ +
				"::handle_connect()" ) ;
			std::cerr << hostname_ << ":" << e_.what() << "--syncrhonized_connector::handle_connect()"  << std::endl ;
		}
		if(!finished)
		{
			// An error occurred. close and reconnect
			if(e)
			{
				asio::system_error se(e) ;
				msgsaver.set_errstatus(commonlibs::STATUS_COMM_ERROR,
					hostname_ + ":" +
					this->servicename_ + " - " +
					se.what()) ;
			}
			try {
				close() ;
				commonlibs::sleep_relative_ms(100) ;
			}
			catch(const std::exception &e_)
			{
				std::cerr << "Error in closing: " << e_.what() << std::endl ;
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
				std::cerr << "Error in closing : " << e_.what() << std::endl ;
			}
		}
  }

  void write_probemessage(asio::error_code &e, const probemessage &pm_)
  {
	  if(sptr_conn_)
	  {
		  sptr_conn_->write_msg(e, pm_) ;
	  }
  }

  int conn_sync_read(asio::error_code &e, std::vector<probemessage> &vpm, bool q)
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
	std::shared_ptr<Tconnection>	sptr_conn_ ;
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
	std::shared_ptr<std::thread> p_t_ ;
protected:

	Tmsgsaver msgsaver ;
} ;


} // namespace

#endif // SERIALIZATION_CONNECTION_HPP
