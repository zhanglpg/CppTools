
#ifndef __POSIXTIME_UTILITIES_HPP
#define __POSIXTIME_UTILITIES_HPP

#include <chrono>
#include <ctime>
#include <string>

namespace commonlibs {

using tick_type = long long;
using time_point = std::chrono::system_clock::time_point;

// Convert a UTC time_point to the local equivalent time_point
// (shifts the stored value by the local UTC offset so that
//  formatting it as UTC yields the local wall-clock time).
inline time_point utc_to_local_tp(const time_point &tp)
{
	std::time_t t = std::chrono::system_clock::to_time_t(tp);
	std::tm local_tm = *std::localtime(&t);
	// timegm (GNU/POSIX extension) converts struct tm treated as UTC back to time_t
	std::time_t local_as_utc = timegm(&local_tm);
	return std::chrono::system_clock::from_time_t(local_as_utc);
}

// Formats a time_point as ISO string "YYYYMMDDTHHmmss"
inline std::string to_iso_string(const time_point &tp)
{
	std::time_t t = std::chrono::system_clock::to_time_t(tp);
	std::tm *ltm = std::gmtime(&t);
	char buf[32];
	std::strftime(buf, sizeof(buf), "%Y%m%dT%H%M%S", ltm);
	return std::string(buf);
}

// Formats a time_point as simple string "YYYY-Mon-DD HH:MM:SS"
inline std::string to_simple_string(const time_point &tp)
{
	std::time_t t = std::chrono::system_clock::to_time_t(tp);
	std::tm *ltm = std::gmtime(&t);
	char buf[64];
	std::strftime(buf, sizeof(buf), "%Y-%b-%d %H:%M:%S", ltm);
	return std::string(buf);
}

// ---- functor: seconds since epoch from a time_point ----
struct seconds_since_epoch_from_posixtime {
	seconds_since_epoch_from_posixtime() {}
	tick_type operator()(const time_point &tp)
	{
		return std::chrono::duration_cast<std::chrono::seconds>(
			tp.time_since_epoch()).count();
	}
};

// ---- functor: milliseconds since epoch from a time_point ----
struct milliseconds_since_epoch_from_posixtime {
	milliseconds_since_epoch_from_posixtime() {}
	tick_type operator()(const time_point &tp)
	{
		return std::chrono::duration_cast<std::chrono::milliseconds>(
			tp.time_since_epoch()).count();
	}
};

// ---- UTC clock helpers ----
struct utc_tools {
	static tick_type seconds_since_epoch_from_now()
	{
		return std::chrono::duration_cast<std::chrono::seconds>(
			std::chrono::system_clock::now().time_since_epoch()).count();
	}

	static tick_type milliseconds_since_epoch_from_now()
	{
		return std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::system_clock::now().time_since_epoch()).count();
	}
};

// ---- UTC to local conversion ----
struct utc_to_local {
	utc_to_local() {}
	time_point operator()(const time_point &tp)
	{
		return utc_to_local_tp(tp);
	}

	static time_point to_local(const time_point &tp)
	{
		return utc_to_local_tp(tp);
	}
};

// ---- Build time_point / strings from milliseconds since epoch ----
struct ptime_from_milliseconds_since_epoch {
	ptime_from_milliseconds_since_epoch() {}

	static time_point get_ptime(const tick_type &totalms)
	{
		return std::chrono::system_clock::time_point(
			std::chrono::milliseconds(totalms));
	}

	static time_point get_ptime_local(const tick_type &totalms)
	{
		return utc_to_local::to_local(get_ptime(totalms));
	}

	static std::string get_simple_string(const tick_type &totalms)
	{
		return to_simple_string(get_ptime(totalms));
	}

	static std::string get_iso_string(const tick_type &totalms)
	{
		return to_iso_string(get_ptime(totalms));
	}

	static std::string get_simple_string_local(const tick_type &totalms)
	{
		return to_simple_string(get_ptime_local(totalms));
	}

	static std::string get_iso_string_local(const tick_type &totalms)
	{
		return to_iso_string(get_ptime_local(totalms));
	}
};


}

#endif
