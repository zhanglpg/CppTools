
#ifndef __SLEEP_RELATIVE_HPP
#define __SLEEP_RELATIVE_HPP

#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/thread/xtime.hpp>

namespace commonlibs {
struct sleep_relative_ms
{
public:
	sleep_relative_ms(int millisecs) 
	{
		boost::system_time const start=boost::get_system_time();
		boost::system_time const timeout=start+boost::posix_time::milliseconds(millisecs); 
		boost::thread::sleep(timeout) ;
	} 
} ;

struct systemtime_tools
{
public:
	static boost::system_time get_system_time( ) {
		return boost::get_system_time();
	}

	static boost::system_time get_next_system_time_click(const boost::system_time & tprevious, 
		const int milliseconds) {
		return tprevious + boost::posix_time::milliseconds(milliseconds);;
	}

	static void sleep_ms_until(const boost::system_time &ptuntil) 
	{
		boost::thread::sleep(ptuntil) ;
	} 
} ;


}
#endif 

