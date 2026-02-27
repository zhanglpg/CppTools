
#ifndef CURRENT_PROCESS_ID_COMMONLIBS_HPP
#define CURRENT_PROCESS_ID_COMMONLIBS_HPP
#include <string>
#include <sstream>

namespace commonlibs
{

template <class T>
inline std::string to_string (const T& t)
{
	std::stringstream ss;
	ss << t;
	return ss.str();
}


#ifdef WIN32
#include "windows.h"
#else
#include "unistd.h"
#endif

class getprocessid {
public:
	static unsigned  int get_current_process_id()
	{
#ifdef WIN32
		return static_cast<unsigned int>(GetCurrentProcessId ()) ;
#else
		return static_cast<unsigned int>(getpid()) ;
#endif

	}
//	static std::string cmdrm ;
#ifdef WIN32
	static std::string cmdrm () {
		return std::string("del /Q ") ;
	}
#else
	static std::string cmdrm () {
		return std::string("rm -f ") ;
	}
#endif

} ;



class pidsaver
{
private:
	std::string s_piddir ;
	unsigned int pid ;
public:
	pidsaver(std::string s_pid_dir_, std::string s_filename_ = "")  : s_piddir(s_pid_dir_)
	{
		pid = getprocessid::get_current_process_id() ;
		std::string cmd ;
		if(s_filename_ == "")
		{
			s_filename_ = to_string<unsigned int>(pid) ;
		}
		if(s_piddir != "")
		{
			cmd = std::string("echo \"") +
				std::to_string(pid) + "\" > "
				+ s_piddir + "/" + s_filename_ ;
			::system(cmd.c_str()) ;
		}

	}
	~pidsaver()
	{
		if(s_piddir != "")
		{
			std::string cmd ;
			cmd = getprocessid::cmdrm() + " " +
				s_piddir + "/" + to_string<unsigned int>(pid) ;
			::system(cmd.c_str()) ;
		}
	}
} ;


}


#endif
