
#ifndef __SLEEP_RELATIVE_HPP
#define __SLEEP_RELATIVE_HPP

#include <thread>
#include <chrono>

namespace commonlibs {

struct sleep_relative_ms
{
public:
	sleep_relative_ms(int millisecs)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(millisecs));
	}
} ;

struct systemtime_tools
{
	using time_point = std::chrono::system_clock::time_point;
public:
	static time_point get_system_time()
	{
		return std::chrono::system_clock::now();
	}

	static time_point get_next_system_time_click(const time_point &tprevious,
		const int milliseconds)
	{
		return tprevious + std::chrono::milliseconds(milliseconds);
	}

	static void sleep_ms_until(const time_point &ptuntil)
	{
		std::this_thread::sleep_until(ptuntil);
	}
} ;


}
#endif
