#ifndef __TEXT_LOGGER_HPP
#define __TEXT_LOGGER_HPP
#include <filesystem>
#include <chrono>
#include <ctime>
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
	using time_point = std::chrono::system_clock::time_point;

	static std::string tp_to_iso_string(const time_point &tp)
	{
		std::time_t t = std::chrono::system_clock::to_time_t(tp);
		std::tm *ltm = std::localtime(&t);
		char buf[32];
		std::strftime(buf, sizeof(buf), "%Y%m%dT%H%M%S", ltm);
		return std::string(buf);
	}

	static int get_local_date_int(const time_point &tp)
	{
		std::time_t t = std::chrono::system_clock::to_time_t(tp);
		std::tm *ltm = std::localtime(&t);
		return (ltm->tm_year * 10000) + (ltm->tm_mon * 100) + ltm->tm_mday;
	}

	void make_name(time_point &ptnow, unsigned int u_extra_id, bool use_id, std::string &s_name)
	{
		s_name =
			s_prefix + (use_id ?
				std::to_string(u_extra_id) : std::string(""))
				+ std::string("_") +
					tp_to_iso_string(ptnow) +
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

			time_point ptnow = std::chrono::system_clock::now();

			std::string s_filenamenew ;
			bool b_start_new = false ;
			if(!b_logstarted) {
				make_name(ptnow, u_extra_id , use_id , s_filenamenew) ;
				b_start_new = true ;
			}
			else {
				auto td = std::chrono::duration_cast<std::chrono::seconds>(
					ptnow - pt_lastlog).count();
				if(td > static_cast<long long>(u_period_seconds) && !b_log_daily)
				{
					make_name(ptnow, u_extra_id , use_id , s_filenamenew) ;
					b_start_new = true ;
				}
				else if(b_log_daily && get_local_date_int(ptnow) != get_local_date_int(pt_lastlog))
				{
					make_name(ptnow, u_extra_id , use_id , s_filenamenew) ;
					b_start_new = true ;
				}
			}
			std::filesystem::path thepath_ = std::filesystem::path(s_logdir) ;
			if(true == b_start_new)
			{
				bool b_newfile_name_good = false ;
				if(!std::filesystem::exists(thepath_))
				{
					return -1 ;
				}
				if (!std::filesystem::is_directory(thepath_))
				{
					return -1 ;
				}
				std::string s_filenamenew_log =
					s_filenamenew + std::string(".log") ;
				std::string fullfilename_ = (thepath_ / s_filenamenew_log).string()  ;

				if(std::filesystem::exists(fullfilename_)) {
					int inc = 0 ;
					do {
						s_filenamenew_log =
							s_filenamenew + std::string("-") + std::to_string(++ex) +
							std::string(".log") ;
						fullfilename_ = (thepath_ / s_filenamenew_log).string()  ;
						inc ++ ;
					}
					while(std::filesystem::exists(fullfilename_) && inc < 100 ) ;
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
		std::string s_logdir , s_prefix , s_nameext;
		std::string s_logfilename ;
		unsigned int u_period_seconds ;
		bool b_logstarted ;
		bool b_log_daily ;
		time_point pt_lastlog ;
		std::ofstream ofs ;
		unsigned int ex ;
		std::ios_base::openmode omode ;
	} ;
}
#endif
