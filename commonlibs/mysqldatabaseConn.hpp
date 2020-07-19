
#ifndef _DATABASE_MYSQLCONN_H
#define _DATABASE_MYSQLCONN_H

#include <string> 
#include <list>
#include <vector>
#include <memory>
#include "mysql++.h"
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include "boost/date_time/posix_time/posix_time.hpp"
#include "boost/date_time/gregorian/gregorian.hpp"
#include "boost/date_time/local_time_adjustor.hpp"
#include "boost/date_time/c_local_time_adjustor.hpp"

#if (MYSQLPP_HEADER_VERSION < 0x30000)
// updated for mysql 3.0.2 
	#include <custom.h>
#endif
#if (MYSQLPP_HEADER_VERSION >= 0x30000)	
	#include <ssqls.h>
#endif
using namespace std; 
//using namespace mysqlpp ;

namespace commonlibs 
{

class MySQLdatabaseConn
{
private:
	std::string hostip , username, pw ;
	int port ;
	std::string dbname ;
public:
	// in the form of :host  username password port
	MySQLdatabaseConn( const std::string &hostip_ , 
		const std::string &username_ , 
		const std::string &pw_ ,
		const int port_, 
		const std::string &dbname_
		) : hostip(hostip_) , username(username_) , pw(pw_) , port(port_) , 
		dbname(dbname_) 
	{
		
		try 
		{
			con = boost::shared_ptr<mysqlpp::Connection> (new mysqlpp::Connection(false)) ;
			std::cout <<" Mysql connection built..." << std::endl ;
			con->set_option(new mysqlpp::ReconnectOption(true)) ;
			mysqlpp::MultiStatementsOption* opt = new mysqlpp::MultiStatementsOption(true);
			con->set_option(opt);

			if ( //!connect_to_db(argc, argv, con, "clientdb")) {
				!con->connect(dbname.c_str() , hostip_.c_str()  , username_.c_str() , 
					pw_.c_str(), 
				    port_)) {
				//throw databasepoll::ConnectionError()  ;
				return ;
			}
			//con.set_option(Connection::opt_reconnect, true) ;
			//con_m.set_option(Connection::opt_reconnect ,true) ;
			
		} // try
		catch(const mysqlpp::ConnectionFailed &cf_)
		{
			cerr << "Connect to clientdb failed: " << cf_.what()  <<endl ;
		}
		catch(const mysqlpp::Exception &e_)
		{
			cerr << "Connect to clientdb failed: " << e_.what() <<endl ;
		}
		catch(const std::exception & e_)
		{
			cerr << "Connect to clientdb,system failed: " << e_.what() <<endl ;
		}
	}
	~MySQLdatabaseConn() {
		try{
			con->disconnect(); 
		}
		catch(const mysqlpp::ConnectionFailed &cf_)
		{
			cerr << "Closing error: " << cf_.what()  << endl ;
		}
		catch(const std::exception &e_)
		{
			cerr << "Closing... system error :" << e_.what() <<endl ;
		}
	} 

	/// 
	/// \brief reconnect to the database (clost and conenct)
	/// \brief using the hostname, username , password and Port in sequence
	/// \param argc is the number of parameters, usually four
	/// \param  argv are array of unsigned char strings of hostname, username , password and port
	void reconnect() 
	{
		try
		{
			con = boost::shared_ptr<mysqlpp::Connection> (new mysqlpp::Connection(false)) ;
			con->set_option(new mysqlpp::ReconnectOption(true)) ;
			mysqlpp::MultiStatementsOption* opt = new mysqlpp::MultiStatementsOption(true);
			con->set_option(opt);

			if (// !connect_to_db(argc, argv, con, "clientdb")) {
				!con->connect(dbname.c_str() , 
					hostip.c_str()  , username.c_str() , 
					pw.c_str(), 
				    port)) {
				return ;
			}
		}
		catch(const mysqlpp::ConnectionFailed &cf_)
		{
			cerr << "ReConnect to clientdb failed: " << cf_.what()  <<endl ;
		}
		catch(const mysqlpp::Exception &e_)
		{
			cerr << "Connect to clientdb failed: " << e_.what() <<endl ;
		}
		catch(const std::exception & e_)
		{
			cerr << "Connect to clientdb,system failed: " << e_.what() <<endl ;
		}	
		return ;
	}
	bool connected(void) const {return con->connected() ;}
	void closeConn(void) {
		try{
			con->disconnect(); 
		}
		catch(const mysqlpp::ConnectionFailed &cf_)
		{
			cerr << "Closing error: " << cf_.what()  << endl ;
		}
		catch(const std::exception &e_)
		{
			cerr << "Closing... system error :" << e_.what() <<endl ;
		}
	}
public:

	boost::shared_ptr<mysqlpp::Connection> con ;
} ;

}

#endif
