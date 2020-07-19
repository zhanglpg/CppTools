#ifndef __TEXT_LOGGER_HPP
#define __TEXT_LOGGER_HPP
#include "boost/filesystem.hpp"
#include <iostream>
#include <fstream>
#include <string>

namespace commonlibs {
class log_control{
	public:
		log_control(bool b_l_ , const std::string &s_dirname_, 
			unsigned int u_period_secs_,
			bool b_daily = false, 
			const std::string &s_nameprefix_ = std::string(""),
			const std::string &s_nameext_ = std::string(""), 
			std::ios_base::openmode omode_ = std::ios_base::out) 
		{
			//std::cout << "Building the logger..." << std::endl ;
			b_logging = b_l_ ;
			b_logstarted = false ;
			s_logdir = s_dirname_ ;
			s_prefix = s_nameprefix_ ;
			s_nameext = s_nameext_ ;
			b_log_daily = b_daily ;
			u_period_seconds = u_period_secs_ ;
			ex = 0 ;
			omode = omode_ ;
		}
		~log_control () {
			this->close_all() ;
		}
private:
	void make_name(boost::posix_time::ptime& ptnow, unsigned int u_extra_id , bool use_id , std::string &s_name)
	{
		s_name = 
			s_prefix + (use_id? 
				boost::lexical_cast<std::string, unsigned int>(u_extra_id) : std::string(""))
				+ std::string("_") + 
					boost::posix_time::to_iso_string(ptnow) +
			s_nameext;
	}
	public:
		int log_string(const std::string &s_pc , unsigned int u_extra_id = 0 , bool use_id = false) 
		{
			return log_string(s_pc.c_str() , (unsigned int)s_pc.size() , u_extra_id, use_id) ; 
		}

		int log_string(const char *pc, unsigned int u_size, unsigned int u_extra_id = 0, bool use_id = false)
		{
			
			if(!b_logging)
				return 0;

			boost::posix_time::ptime ptnow =
				boost::posix_time::ptime(
				boost::posix_time::second_clock::local_time()) ;

			std::string s_filenamenew ; 
			bool b_start_new = false ;
			if(!b_logstarted) {
				make_name(ptnow, u_extra_id , use_id , s_filenamenew) ;
				b_start_new = true ;
			}
			else {
				boost::posix_time::time_duration td =
					ptnow - pt_lastlog ; 
				if(td.total_seconds() > (int)u_period_seconds && !b_log_daily)
				{
					make_name(ptnow, u_extra_id , use_id , s_filenamenew) ;
					b_start_new = true ;
				}
				else if(b_log_daily && ptnow.date() != pt_lastlog.date()) // every day
				{
					make_name(ptnow, u_extra_id , use_id , s_filenamenew) ;
					b_start_new = true ;
				}
			}
			boost::filesystem::path thepath_  =
				boost::filesystem::path(s_logdir) ;
			if(true == b_start_new)
			{
				bool b_newfile_name_good = false ;
				if(!boost::filesystem::exists(thepath_))
				{
				//	std::cerr << "Could not find the logging dir :"
				//		<< thepath_.string() << endl ;
					return -1 ;
				}
				if (!boost::filesystem::is_directory(thepath_))
				{
				//	std::cerr << thepath_.string() << ": is not a directory!" << endl ;
					return -1 ;
				}
				std::string s_filenamenew_log = 
					s_filenamenew + std::string(".log") ;
				std::string fullfilename_ = (thepath_ / s_filenamenew_log).string()  ;

				if(boost::filesystem::exists(fullfilename_)) {
					int inc = 0 ;	
					do {
						s_filenamenew_log = 
							s_filenamenew + std::string("-")+ boost::lexical_cast<std::string, unsigned int>( ++ ex) + 
							std::string(".log") ;
						fullfilename_ = (thepath_ / s_filenamenew_log).string()  ;
						inc ++ ;
					}
					while(boost::filesystem::exists(fullfilename_) && inc < 100 ) ;
					if(inc < 100)
						b_newfile_name_good = true ;
				}
				else {
					b_newfile_name_good = true ;
				}

				if(ofs.is_open())
					ofs.close() ;
				ofs.open(fullfilename_.c_str(), omode) ;
				if(!ofs.is_open())
					return -1 ;
				b_logstarted = true ;
				pt_lastlog = ptnow ;
				ofs.write(pc , u_size) ;
			}
			else 
			{
				if(ofs.is_open())
					ofs.write(pc , u_size) ;
			}
			return 0 ;
		}
		void close_all()
		{
			if(ofs.is_open())
				ofs.close() ;
			b_logstarted = false ;
		}
	private:
		bool b_logging ;
		std::string s_logdir , s_prefix ,s_nameext;
		std::string s_logfilename ; 
		unsigned int u_period_seconds ;
		bool b_logstarted ; 
		bool b_log_daily ;
		boost::posix_time::ptime pt_lastlog  ;
		std::ofstream ofs ;
		unsigned int ex ;
		std::ios_base::openmode omode ; 
	} ;
} 
#endif


