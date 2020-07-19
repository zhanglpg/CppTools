///	\brief Header file for SMS data parser demo: 
///	\brief  Connects to a TCP server that provides real time feed of SMS sensor data
///	\brief	
///	\brief	Original Coding 03/19/2009	
///	Liping Zhang, PATH, UCB

#ifndef _SMSPARSER_HPP
#define _SMSPARSER_HPP

#include "connection.hpp"
#include <boost/thread.hpp>
#include <string>
namespace commonlibs {

class smsparser
{
public:
	/// 
	smsparser(boost::asio::io_service &ios_ ,
		const std::string &host_ ,
		const std::string &service_) 
		: 
		msgpool_(10), 
		hostname_ (host_) , 
		servicename_ (service_)
	{
		try 
		{
			finished = false ;
			b_quiet = false ;
			sptr_conn_ = commonlibs::connection_ptr(new commonlibs::connection(ios_)) ;
			boost::shared_ptr<boost::thread>
				t_ = boost::shared_ptr<boost::thread>(new boost::thread(
					boost::bind(&smsparser::start, this)));
		}
		catch(const std::exception &e_)
		{
			std::cerr << e_.what() <<endl ;
		}
	}
	void start ()
	{
		do {
			try {
				boost::system::error_code ec_ ;
				sptr_conn_->sync_connect_host(hostname_ ,
					servicename_,
					ec_) ;
				handle_connect(ec_) ;
				commonlibs::sleep_relative_ms(1000) ;
			}
			catch(const  std::exception & e_)
			{
				std::cerr << "sync connect: " <<e_.what() <<endl ;
			}
			//sleep for a while
		} while (finished == false) ;
	}

	~forward_statemap() 
	{	
		try {
			//commonlibs::sleep_relative_ms(2000) ;
			if(sptr_conn_)
				sptr_conn_ -> close() ;
		}
		catch(const std::exception &e_)
		{
			std::cerr <<e_.what() <<endl ;
		}
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
					while(!msgpool_.back_empty())
					{
						commonlibs::probemessage pm_ ;
						msgpool_.getBackwardMessage(pm_) ;
						sptr_conn_->write_msg(error , pm_) ;
					}
					commonlibs::sleep_relative_ms(100) ;
					if(error)
						throw boost::system::system_error(error) ;
				}
			} // if !e
		}// try
		catch(const std::exception & e_)
		{
			std::cerr <<hostname_ <<  " :Error in asynchronized reading: " << e_.what() << endl ;
		}
		if(!finished)
		{
			// An error occurred. close and reconnect  
			if(e)
			{
				std::cerr << e.message() << std::endl;
			}
			try {
				sptr_conn_->socket().close();	

				commonlibs::sleep_relative_ms(2000) ;

			}
			catch(const std::exception &e_)
			{
				std::cerr << "Error in closing: " << e_.what() <<endl ;
			}
		}
		else   // finished the session stops
		{
			try {
				sptr_conn_->socket().close();
			}
			catch(const std::exception &e_)
			{
				std::cerr << "Error in closing : " << e_.what() << endl ;
			}
		}
  }
  int addBackwardMsg(const commonlibs::probemessage &pm_)
  {
	  return msgpool_.addBackMessage(pm_) ;
  }

private:
	commonlibs::MessagePool<commonlibs::probemessage> msgpool_ ;
	commonlibs::connection_ptr	sptr_conn_ ;
	//boost::shared_ptr<basio::io_service> sptr_ios ;
	std::string hostname_ ;
	std::string servicename_ ;
	bool finished ; 
	bool b_quiet ;
} ;
} 
#endif 
