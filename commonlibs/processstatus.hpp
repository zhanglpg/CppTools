
#ifndef COMMONLIBS_PROCESS_STATUS_HPP
#define COMMONLIBS_PROCESS_STATUS_HPP

#include "commonlibs/mysqldatabaseConn.hpp"

#include "commonlibs/common_errorcode.hpp"
#include "commonlibs/currentprocessid.hpp"
namespace commonlibs {


			 
class Process_status {
public: 

	Process_status(
		boost::shared_ptr<mysqlpp::Connection>  con_ ,
		unsigned int uproc_id_ , 
		const std::string s_processname_ , 
		const std::string s_procseq_ = "null")
		: 
		con(con_) ,
		u_proc_id (uproc_id_) , 
		s_process_name(s_processname_) ,
		s_proc_seq(s_procseq_) , 
		u_lognum(0) 
	{
		std::cout << "Process status constructed .." <<std::endl ;
	}
	 	///\brief update the process status in mysql database 
	int update_process_status(
		const unsigned int errorcode, 
		const std::string& s_errmsg) 
	{
		try
		{
			mysqlpp::Query query = con->query() ;
			boost::posix_time::ptime pt_ap(
				boost::posix_time::second_clock::local_time()) ;				


			std::string s_time = 
				boost::posix_time::to_iso_extended_string(pt_ap) ;
			std::string qstr = "" ;
			if(errorcode == 
				(unsigned int)commonlibs::STATUS_OK)
			{
				qstr += 
					"UPDATE SM_processStatus set last_activity_time ='" + s_time + 
					"' , last_valid_status_time='" + s_time + 
					"', current_status =  "  + 
					boost::lexical_cast<std::string,unsigned int>(errorcode)  + 
					" , current_error_message = '" + s_errmsg  + "' , pid= " +
					boost::lexical_cast<std::string, unsigned int>(commonlibs::getprocessid::get_current_process_id()) +
					" where "  + 
					" proc_index = " + 
					boost::lexical_cast<std::string,  unsigned int>(u_proc_id)  + " and " 
					" process_seq = '" + s_proc_seq +"'"  ;
				query << qstr

					;
			}
			else 
			{
				qstr +="UPDATE SM_processStatus set last_activity_time ='" + s_time + 
					"' , current_status =  "  + 
					boost::lexical_cast<std::string,unsigned int>(errorcode)  + 
					" , current_error_message = '" + s_errmsg  + "' , pid= " +
					boost::lexical_cast<std::string, unsigned int>(commonlibs::getprocessid::get_current_process_id()) +
					" where "  + 
					" proc_index = " + 
					boost::lexical_cast<std::string,  unsigned int>(u_proc_id) + " and " 
					" process_seq = '" + s_proc_seq +"'" ; 
				query << qstr ;
			}

			query.execute() ;

			return 0 ;
			
		}
		catch(const boost::bad_lexical_cast &be)
		{	
			cerr << "update_process_status in  bad format :" << be.what() << endl ;
		}
		catch(const mysqlpp::BadQuery &bq_)
		{
			cerr << "update_process_status error: " << bq_.what() <<endl ;
		}
		catch(const mysqlpp::Exception &e_)
		{

			cerr << "update_process_status general error: " << e_.what() <<endl ;
		}
		catch(const std::exception &e_)
		{
			cerr << "update_process_status system error: " << e_.what() <<endl ;
		}
		return -1 ;		
	}

	 	///\brief update the process status in mysql database 
	int insert_process_message(
		const unsigned int messagetype, 
		const std::string &s_errmsg
		) 
	{
		try
		{
			mysqlpp::Query query = con->query() ;
			boost::posix_time::ptime pt_ap(
				boost::posix_time::second_clock::local_time()) ;				


			std::string s_time = 
				boost::posix_time::to_iso_extended_string(pt_ap) ;

			query << "INSERT INTO SM_messageslog values (" + boost::lexical_cast<std::string,  unsigned int>(u_proc_id) + 
				" , '" + s_process_name + "', '" + 
				s_proc_seq + "', '" +
				s_time + "' , '" + 
				s_errmsg +"', "  + 
				boost::lexical_cast<std::string, unsigned int>(u_lognum) + ", " + 
				boost::lexical_cast<std::string, unsigned int>(messagetype) + ",'" + 
				s_time.substr(0, 10) + "')" 
				 ;
			query.execute() ;
			u_lognum ++ ;
			return 0 ;
			
		}
		catch(const boost::bad_lexical_cast &be)
		{	
			cerr << "insert_process_message in  bad format :" << be.what() << endl ;
		}
		catch(const mysqlpp::BadQuery &bq_)
		{
			cerr << "insert_process_message error: " << bq_.what() <<endl ;
		}
		catch(const mysqlpp::Exception &e_)
		{

			cerr << "insert_process_message general error: " << e_.what() <<endl ;
		}
		catch(const std::exception &e_)
		{
			cerr << "insert_process_message system error: " << e_.what() <<endl ;
		}
		return -1 ;		
	}
	


private:
	boost::shared_ptr<mysqlpp::Connection>  con ;
	unsigned int u_proc_id ;
	std::string s_process_name ;
	std::string s_proc_seq ;
	unsigned int u_lognum ;
} ;

}			 


#endif

