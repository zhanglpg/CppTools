
#ifndef __SENSYS_SENSOR_HPP
#define __SENSYS_SENSOR_HPP

#include <string>
#include "commonlibs/probemessage.hpp"
#include <ctime>

namespace commonlibs 
{
	typedef struct sensys_raw_event
	{
		//rule_s rl ;
		unsigned int u_id ; 
		double		 d_tickcount ;
		unsigned int u_event ;

		friend ostream & operator << (ostream &out , const sensys_raw_event &s)
		{
			out << std::fixed << std::setw(5) << s.u_id << " " << 
				std::setprecision(3) <<
				s.d_tickcount <<  " " << 
				s.u_event ; 
			return out ;				
		}
	} sensys_raw_event;



	using namespace boost::spirit ;

	class sensys_textmessage : public commonlibs::textline_message<sensys_textmessage, sensys_raw_event> 
	{
	public:
		void build_rule() 
		{
			rdelim =
				( ch_p(',') | +blank_p) ;
			rhexprefix = (!str_p("0x") | ! str_p("0X")) ;
			r = 
				*(blank_p) >>   
					(!ch_p('"')) >> rhexprefix >> hex_p[assign_a(u_id)] >> (!ch_p('"')) >> rdelim >> 
					real_p[assign_a(d_tick)] >> rdelim >> 
					uint_p[assign_a(u_event)] >> 
					(*blank_p) >> (*eol_p) ; 
		}
		sensys_textmessage()  {
			build_rule() ;
		}

		int parse_message_imp(const probemessage &pm_)
		{
			bool full = 0 ;
			const std::vector<unsigned char>& vc = pm_.getmsg()  ;
			
			//echo<iterator_vc>  ecur(vc.begin()) ;
			iterator_vc first = static_cast<iterator_vc>(vc.begin()) ;
			iterator_vc last = static_cast<iterator_vc>(vc.end()) ;

			 
			boost::spirit::parse_info<iterator_vc> info = 
				boost::spirit::parse(first, last, r);
			
			if(info.hit && info.full)
			{
				pl.u_id = u_id ;
				pl.d_tickcount = d_tick ;
				pl.u_event= u_event ;
				return 0 ;
			}
			
			return -1 ;
		}
	private: 
		rule_vc r, rdelim , rhexprefix ;
		unsigned int u_id  ;
		double d_tick ; 
		unsigned int u_event ;
	};

}

#endif