// TCP connection: acceptor and tcp client connection
// connection.hpp
// ~~~~~~~~~~~~~~
// Author: Liping Zhang
/// 2010
#ifndef _COMMONLIBS_DATAGRAM_HPP
#define _COMMONLIBS_DATAGRAM_HPP

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/thread/xtime.hpp>
#include <iomanip>
#include <string>
#include <vector>
#include "commonlibs/probemessage.hpp" 
#include <boost/signal.hpp>
#include <boost/foreach.hpp>
#include "commonlibs/errorstatus.hpp"
#include <commonlibs/connection.hpp>
namespace commonlibs {

// The connection class provides serialization primitives on top of a socket.
template <class Tmessage,class TmessageParser>
class datagram_imp_template
{
public:

	typedef Tmessage message_type ;
  /// Constructor.
	datagram_imp_template(boost::asio::io_service& io_service)
		: socket_(io_service) , io_s_(io_service)
	{
	}
	
	datagram_imp_template(boost::asio::io_service& io_service, int port)
		: socket_(io_service, 
			boost::asio::ip::udp::endpoint(boost::asio::ip::udp::v4(), port)) , io_s_(io_service)
	{
	}
public:
	~datagram_imp_template() 
	{
		close() ;
	}
	/// Get the underlying socket. Used for making a connection or for accepting
	/// an incoming connection.
	boost::asio::ip::udp::socket& socket()
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

	void socket_open() 
	{
		if(!socket_.is_open())
		{
			socket_.open(boost::asio::ip::udp::v4()); 
		}
	}

	void write_msg(boost::system::error_code &e , 
		 const Tmessage &m_, 
		 const boost::asio::ip::udp::endpoint &endp_)
	{
		socket_open() ;
		socket_.send_to(boost::asio::buffer(m_.getmsg()), endp_);
		return ;
	}

	void set_option_nonblock()
	{
		socket_open() ;
		boost::asio::socket_base::non_blocking_io command(true);
		socket_.io_control(command);
		return ;
	}

	/// \brief read_msg:wrapped by sync_read
	/// \brief using the low level socket.read_some function
	/// \param e: INOUT the error code
	/// \param returns > 0 when success
	int read_msg(boost::system::error_code& e, 
		boost::asio::ip::udp::endpoint &endp_,
		bool nonblock= false)
	{
		

		boost::system::error_code err_ ;
		int timec_ = 0 ;
		do {
			socket_open() ;
			size_t len = socket_.receive_from(boost::asio::buffer(inbound_data_) ,
				endp_, 0 ,
				err_) ;
			e = err_ ;
			if(!err_ || err_== boost::asio::error::message_size )
			{
				if(len > (size_t)0) 
					return (int)len;
				else
					return 0 ;
			}
			else if(err_ == boost::system::error_code(boost::asio::error::would_block) && 
					nonblock == false)
			{
				sleep_relative_ms(10) ;
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
	int sync_read(boost::system::error_code& e , 
		std::vector<Tmessage> &pm ,
		boost::asio::ip::udp::endpoint &endp_,
		bool b_quiet = false ,bool nonblock = false ) 
	{

		size_t len = //socket_.read_some(boost::asio::buffer(inbound_data_), e);
			read_msg(e, endp_, nonblock) ;
		if(!e || e == boost::asio::error::message_size)
		{
			int nm_=	
				(int)mp_.parse2Message(inbound_data_.elems, (int)len , pm) ;
			return nm_ ;
		}  // if !e  (no error) 
		else 
		{
			if(  (!nonblock || (nonblock && e!= 
				boost::system::error_code(boost::asio::error::would_block)) ) 
				&& e != boost::asio::error::try_again 
				)
				throw boost::system::system_error(e); 
			else 
				return 0 ;
		}
	}


	/// \brief sync_read_raw: simplified version of sync_read, 
	///	\brief returns number of bytes received instead of number of msgs
	int sync_read_raw(boost::system::error_code& e , 
		const char * &p_c ,
		boost::asio::ip::udp::endpoint &endp_,
		bool b_quiet = false ,bool nonblock = false) 
	{
		size_t len = //socket_.read_some(boost::asio::buffer(inbound_data_), e);
			read_msg(e, endp_ , nonblock) ;
		if(!e || e == boost::asio::error::message_size)
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

	/// The underlying socket.
	boost::asio::ip::udp::socket socket_;

	/// Holds the inbound data.
	boost::array<typename Tmessage::Tchar , MAX_PACKET_SIZE> inbound_data_;
	TmessageParser mp_ ;
	boost::asio::io_service &io_s_ ;
};

template< class Tmessage, class TmessageParser>  
class datagram_template : public datagram_imp_template <Tmessage, TmessageParser>
{
public:

	datagram_template(boost::asio::io_service &io_service)
		: datagram_imp_template<Tmessage, TmessageParser>(io_service)
	{
	}
	datagram_template(boost::asio::io_service &io_service, int port)
		: datagram_imp_template<Tmessage, TmessageParser>(io_service, port)
	{
	}

	void sync_connect_host(const std::string & host_ , // IP v4 address
		const std::string & service_, boost::system::error_code &ec_ )
	{ 
		boost::system::error_code error = boost::asio::error::host_not_found;

		boost::asio::ip::udp::resolver resolver(
			datagram_imp_template<Tmessage, TmessageParser>
			::io_service());	
		boost::asio::ip::udp::resolver::query query(host_, service_);		
		boost::asio::ip::udp::resolver::iterator endpoint_iterator = resolver.resolve(query);
		boost::asio::ip::udp::resolver::iterator end;
		try  {
			while (error && endpoint_iterator != end)
			{
				datagram_imp_template<Tmessage, TmessageParser>
					::socket().close();
				datagram_imp_template<Tmessage, TmessageParser>
					::socket().
					connect(*endpoint_iterator++, error);
			}
			if(!error && endpoint_iterator != end)
			{
				endp_ = *endpoint_iterator ;
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
	
	const boost::asio::ip::udp::endpoint &get_endp() const {
		return endp_ ;
	}
private :
	boost::asio::ip::udp::endpoint endp_ ;
} ;

typedef datagram_imp_template<commonlibs::probemessage,
	commonlibs::messageParser> datagram_imp ;
typedef datagram_template<commonlibs::probemessage, 
	commonlibs::messageParser> datagram_pm ;

typedef datagram_template<commonlibs::probemessage, 
	commonlibs::textlineParser> datagram_tl ;

typedef datagram_template<commonlibs::probemessage, 
	commonlibs::rawbinParser> datagram_raw ;


typedef datagram_template<commonlibs::strmessage, 
	commonlibs::textlineStrParser> datagram_tl_str ;


typedef boost::shared_ptr<datagram_pm> datagram_pm_ptr;

template <class datagram_implementor, class Tdatagram = datagram_pm, class Tmsgsaver = simple_saver>
class datagram_synchronized_connector 
{
public:
	datagram_synchronized_connector(boost::asio::io_service &ios_ ,
		const std::string &host_ ,
		const std::string &service_, 
		bool start_session = true
		) 
		: 
		ios(ios_) ,
		hostname_ (host_) , 
		servicename_ (service_), 
		b_server (false)
	{
		try 
		{
			b_connected = false ;
			finished = false ;
			b_quiet = false ;
			sptr_conn_ = boost::shared_ptr<Tdatagram>(new Tdatagram(ios_)) ;
				
			if(start_session)
			{
				p_t_ = boost::shared_ptr<boost::thread>(new boost::thread(
						boost::bind(&datagram_synchronized_connector::start_imp, this)));
			}
		}
		catch(const std::exception &e_)
		{
			msgsaver.set_errstatus(commonlibs::STATUS_COMM_ERROR, 
				e_.what()) ;
			std::cerr << e_.what() <<endl ;
		}
	}
	
	datagram_synchronized_connector(boost::asio::io_service &ios_ ,
		int service_, 
		bool start_session = true
		) 
		: 
		ios(ios_) ,
		hostname_ ("localhost") , 
		port (service_), 
		b_server (true)
	{
		try 
		{
			b_connected = false ;
			finished = false ;
			b_quiet = false ;
			
				
			if(start_session)
			{
				p_t_ = boost::shared_ptr<boost::thread>(new boost::thread(
						boost::bind(&datagram_synchronized_connector::start_imp, this)));
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
		if(! b_connected) {
			p_t_ = boost::shared_ptr<boost::thread>(new boost::thread(
					boost::bind(&datagram_synchronized_connector::start_imp, this)));
		}
	}

	
	void start ()
	{
		do {
			try {
				boost::system::error_code ec_ ;
				if(! b_server) // 
				{
					sptr_conn_->sync_connect_host(hostname_ ,
						servicename_,
						ec_) ;

					b_connected = true ;
					handle_connect(ec_) ;
				}
				else {
					sptr_conn_ = boost::shared_ptr<Tdatagram>(new Tdatagram(ios, port)) ;
					handle_connect(ec_) ;
					//receive_data() ;
				}
				
			}
			catch(const  std::exception & e_)
			{
				msgsaver.set_errstatus(commonlibs::STATUS_COMM_ERROR, 
					e_.what() + std::string("--datagram_synchronized_connector::start()")) ;
				std::cerr << e_.what() << std::string("--datagram_synchronized_connector::start()") << std::endl ;
				msgsaver.set_errstatus(commonlibs::STATUS_COMM_ERROR, 
					std::string("Closing ... ")  + std::string("--datagram_synchronized_connector::start()")) ;
				try {
					close() ;
				}
				catch(...)
				{
				}
			}
			//sleep for a while
			sleep_relative_ms(200) ;
		} while (finished == false) ;
	}

	~datagram_synchronized_connector() 
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

			  // Successfully established connection. 
				boost::system::error_code error;
				while(!finished) 
				{
					//if(!error || error == boost::asio::error::message_size)
					static_cast<datagram_implementor*>(this)->handle_connected(error) ;

					if(error && error != boost::asio::error::message_size && 
						error != boost::asio::error::would_block && 
						error != boost::asio::error::try_again 
						)
						throw boost::system::system_error(error) ;

//					sleep_relative_ms((int)this->interval_sleepms) ;
				}
			} // if !e
			else {
				// inform the error happened
				boost::system::error_code error = e ;
				static_cast<datagram_implementor*>(this)->handle_connected(error) ;
			}
		}// try
		catch(const std::exception & e_)
		{
			msgsaver.set_errstatus(commonlibs::STATUS_COMM_ERROR, 
				e_.what() + std::string("--datagram_syncrhonized_connector(") + hostname_ + 
				":" + this->servicename_ + 
				"::handle_connect()" ) ;
			std::cerr << hostname_ << ":" << e_.what() << "--datagram_syncrhonized_connector::handle_connect()"  << endl ;
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

/*  void receive_data(void)
  {
		try 
		{
			// set to non block read
			sptr_conn_->set_option_nonblock() ; //
			boost::system::error_code error;
			while(!finished) 
			{
				//if(!error || error == boost::asio::error::message_size)
				static_cast<datagram_implementor*>(this)->handle_connected(error) ;
				if(error && error != boost::asio::error::message_size)
					throw boost::system::system_error(error) ;

				sleep_relative_ms((int)this->interval_sleepms) ;
			}
		}// try
		catch(const std::exception & e_)
		{
			msgsaver.set_errstatus(commonlibs::STATUS_COMM_ERROR, 
				e_.what() + std::string("--datagram_syncrhonized_connector(") + hostname_ + 
				":" + this->servicename_ + 
				"::receive_data()" ) ;
			std::cerr << hostname_ << ":" << e_.what() << "--datagram_syncrhonized_connector::receive_data()"  << endl ;
		}
		if(finished)
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
  }  */
  
  void write_probemessage(boost::system::error_code &e, const commonlibs::probemessage &pm_)
  {
	  if(sptr_conn_)
	  {
		  sptr_conn_->write_msg(e, pm_, 
			  sptr_conn_->get_endp() ) ;
	  }
  }

  int conn_sync_read(boost::system::error_code &e, 
	  std::vector<commonlibs::probemessage> &vpm, bool q)
  {
	  return sptr_conn_->sync_read(e, vpm, 
		  sptr_conn_->get_endp() ,
		  q) ;
  }

  const std::string &get_hostname() const 
  {
	  return hostname_ ;
  }
  const std::string &get_servicename() const {
	  return servicename_ ;
  }
  const std::vector<typename Tdatagram::message_type> &
	  get_vpm()  const
  {
	  return v_pm ; 
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
	
	boost::shared_ptr<Tdatagram>	sptr_conn_ ;
private:
	boost::asio::io_service &ios ;
	void start_imp()
	{
		this->start() ;
	}
	std::string hostname_ ;
	std::string servicename_ ;
	int port ;
	bool finished ; 
	bool b_quiet ;
	bool b_connected ;
	bool b_server ;
	boost::shared_ptr<boost::thread> p_t_ ;

protected:
	boost::asio::ip::udp::endpoint endp_ ;
	Tmsgsaver msgsaver ;
	std::vector<typename Tdatagram::message_type> v_pm  ; 
	int interval_sleepms ;
} ;


/// do not build connection, no extra thread needed 
template <class Tdatagram = datagram_pm, class Tmsgsaver = simple_saver>
class datagram_synchronized_transceiver 
{
public:
	datagram_synchronized_transceiver(boost::asio::io_service &ios_ ,
		const std::string &host_ ,
		const std::string &service_) 
		: 
		hostname_ (host_) , 
		servicename_ (service_)
	{
		try 
		{
			finished = false ;
			b_quiet = false ;
			sptr_conn_ = boost::shared_ptr<Tdatagram>(new Tdatagram(ios_)) ;
			boost::system::error_code ec ;
			resolve_host(ec) ;

		}
		catch(const std::exception &e_)
		{
			msgsaver.set_errstatus(commonlibs::STATUS_COMM_ERROR, 
				e_.what()) ;
			std::cerr << e_.what() <<endl ;
		}
	}

	~datagram_synchronized_transceiver() 
	{	
		try {
			close_connection() ;
		}
		catch(const std::exception &e_)
		{
			std::cerr <<e_.what() <<endl ;
		}
	}

	void close()
	{
		if(sptr_conn_) 
			sptr_conn_->socket().close() ;


	}


	void write_probemessage(boost::system::error_code &e, 
		const commonlibs::probemessage &pm_)
  {
	  if(sptr_conn_)
	  {
		  //std::cout << endp_.address() << ":" << endp_.port() << std::endl ;
		  sptr_conn_->write_msg(e, 
			  pm_, 
			  endp_ ) ;
	  }
  }

  int conn_sync_read(boost::system::error_code &e, 
		std::vector<commonlibs::probemessage> &vpm, 
		bool q)
  {
	  return sptr_conn_->sync_read(e, 
		  vpm, 
		  endp_ ,
		  q) ;
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
			//std::cout << hostname_ << " - Exiting... , closing socket " << std::endl ;
			close() ;
		}catch(...)
		{
		}
  }
protected:
	boost::shared_ptr<Tdatagram>	sptr_conn_ ;
private:

	std::string hostname_ ;
	std::string servicename_ ;
	bool finished ; 
	bool b_quiet ;

	void resolve_host(boost::system::error_code &ec_ )
	{ 
		boost::system::error_code error = boost::asio::error::host_not_found;

		boost::asio::ip::udp::resolver resolver(
			sptr_conn_->io_service());	
		boost::asio::ip::udp::resolver::query query(hostname_, servicename_);		
		boost::asio::ip::udp::resolver::iterator endpoint_iterator = resolver.resolve(query, error);
		boost::asio::ip::udp::resolver::iterator end;
		try  {
			if(!error && endpoint_iterator != end)
			{
				endp_ = *endpoint_iterator ;
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

	const boost::asio::ip::udp::endpoint &get_endp() const {
		return endp_ ;
	}
private :
	boost::asio::ip::udp::endpoint endp_ ;
protected:

	Tmsgsaver msgsaver ;
} ;

typedef datagram_synchronized_transceiver<datagram_tl> datagram_transceiver_tl ;


} // namespace 

#endif // 
