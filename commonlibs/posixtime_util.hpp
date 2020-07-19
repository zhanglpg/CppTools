
#ifndef __POSIXTIME_UTILITIES_HPP
#define __POSIXTIME_UTILITIES_HPP

#include "boost/date_time/posix_time/posix_time.hpp" 
#include "boost/date_time/local_time/local_time.hpp"
#include "boost/date_time/local_time_adjustor.hpp"
#include "boost/date_time/c_local_time_adjustor.hpp"

namespace commonlibs {

 using namespace boost::posix_time;
 using namespace boost::gregorian;

    //This local adjustor depends on the machine TZ settings-- highly dangerous!



struct seconds_since_epoch_from_posixtime {
	public: 
		seconds_since_epoch_from_posixtime() {} 
		
		boost::posix_time::time_duration::tick_type	operator () (const boost::posix_time::ptime &pt) 
		{
			static boost::posix_time::ptime const epoch(date(1970, 1, 1));
			return (pt- epoch).total_seconds();
		}
} ;


struct milliseconds_since_epoch_from_posixtime {
	public: 
		milliseconds_since_epoch_from_posixtime() {} 
		
		boost::posix_time::time_duration::tick_type	operator () (const boost::posix_time::ptime &pt) 
		{
			static boost::posix_time::ptime const epoch(date(1970, 1, 1));
			return (pt - epoch).total_milliseconds();
		}
} ;

struct utc_tools {
public:
	static boost::posix_time::time_duration::tick_type	seconds_since_epoch_from_now() 
	{
		static boost::posix_time::ptime const epoch(date(1970, 1, 1));
		return (boost::posix_time::second_clock::universal_time() - epoch).total_milliseconds();
	}

	static boost::posix_time::time_duration::tick_type	milliseconds_since_epoch_from_now() 
	{
		static boost::posix_time::ptime const epoch(date(1970, 1, 1));
		return (boost::posix_time::microsec_clock::universal_time() - epoch).total_milliseconds();
	}
} ;


struct utc_to_local {
	public: 
		utc_to_local() {} 
		ptime operator () (const boost::posix_time::ptime &pt) 
		{
			typedef boost::date_time::c_local_adjustor<ptime> local_adj;
			return local_adj::utc_to_local(pt);
		}

		static ptime to_local(const boost::posix_time::ptime &pt) 
		{
			typedef boost::date_time::c_local_adjustor<ptime> local_adj;
			return local_adj::utc_to_local(pt);
		}
}; 

struct ptime_from_milliseconds_since_epoch {
	public: 
		ptime_from_milliseconds_since_epoch() {
		}
		
		static ptime get_ptime (const time_duration::tick_type &totalms) 
		{
			static boost::gregorian::date const epoch(1970, 1, 1);
			return ptime(epoch, milliseconds(totalms)) ;
		}	


		static ptime get_ptime_local (const time_duration::tick_type &totalms) 
		{
			static boost::gregorian::date const epoch(1970, 1, 1);
			return utc_to_local::to_local(ptime(epoch, milliseconds(totalms))) ;
		}	

		static std::string get_simple_string (const time_duration::tick_type &totalms) 
		{
			return to_simple_string(get_ptime(totalms)) ;
		}
		static std::string get_iso_string (const time_duration::tick_type &totalms) 
		{
			return to_iso_string(get_ptime(totalms)) ;
		}


		static std::string get_simple_string_local (const time_duration::tick_type &totalms) 
		{
			return to_simple_string(get_ptime_local(totalms)) ;
		}
		static std::string get_iso_string_local (const time_duration::tick_type &totalms) 
		{
			return to_iso_string(get_ptime_local(totalms)) ;
		}
};


}

#endif

