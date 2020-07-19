
#ifndef __SMS_RADAR_PARSER_HPP
#define __SMS_RADAR_PARSER_HPP

#include <string>
#include "commonlibs/probemessage.hpp"
#include <ctime>

namespace commonlibs 
{
	typedef struct sms_radar_raw  {
		std::string s_timestamp ; 
		std::string s_date ;
		unsigned int u_id ;
		double x , y , vx, vy ;
		double veh_length ;
		unsigned int u_ethernet_status ; // 0-5 
		unsigned int u_can_status ;  // 0- 0x80
		unsigned int u_sensor_present ; // 0-0xffff
		unsigned int u_n_objs ;
		unsigned int u_n_msgs ;
		unsigned int u_tscan ;
		unsigned int u_mode ; // 0 for live, 4 for simulation 
		unsigned int u_cycle_count ; // 0-15
		double u_t ;
		double ax, ay ;
		friend ostream & operator << (ostream &out , const sms_radar_raw &s)
		{
			out << s.s_timestamp << " " << 
				s.s_date <<  " " << 
				s.u_id << " (" << s.x << "," << s.y << ") (" << 
				s.vx << "," << s.vy << ")" ; 
			return out ;				
		}
	}
	sms_radar_raw ;


	using namespace boost::spirit ;

	class sms_textmessage : public commonlibs::textline_message<sms_textmessage, sms_radar_raw> 
	{
	public:
		void build_rule() 
		{
			rdelim =
				( ch_p(',') | +blank_p) ;
			rtimestamp =
				limit_d(0u, 23u)[uint_p[assign_a(hh)]] >> ':'    //  Hours 00..23
				>>  limit_d(0u, 59u)[uint_p[assign_a(mm)]] >> ':'    //  Minutes 00..59
				>>  limit_d(0u, 59u)[uint_p[assign_a(ss)]] >> '.' 
				>>  limit_d(0u, 999u)[uint_p[assign_a(ms)]] ;
			rhexprefix = (!str_p("0x") | ! str_p("0X")) ;
			r = 
				*(blank_p) >>   
					rtimestamp >> rdelim >> 
					uint_p[assign_a(udate)] >> rdelim >> 
					uint_p[assign_a(pl.u_id)] >> rdelim >> 
					real_p[assign_a(pl.x)] >> rdelim >> 
					real_p[assign_a(pl.y)] >> rdelim >> 
					real_p[assign_a(pl.vx)] >> rdelim >> 
					real_p[assign_a(pl.vy)] >> rdelim >> 
					real_p[assign_a(pl.veh_length)] >> rdelim >> 
					rhexprefix >> hex_p[assign_a(pl.u_ethernet_status)] >> rdelim >> 
					rhexprefix >> hex_p[assign_a(pl.u_can_status)] >> rdelim >> 
					rhexprefix >> hex_p[assign_a(pl.u_sensor_present)] >> rdelim >> 
					uint_p[assign_a(pl.u_n_objs)] >> rdelim >> 
					uint_p[assign_a(pl.u_n_msgs)] >> rdelim >> 
					uint_p[assign_a(pl.u_tscan)] >> rdelim >> 
					uint_p[assign_a(pl.u_mode)] >> rdelim >> 
					uint_p[assign_a(pl.u_cycle_count)] >> 
					(*blank_p) >> (*eol_p) ; 
		}
		sms_textmessage()  {
			build_rule() ;
		}

		int parse_message_imp(const probemessage &pm_)
		{
			const std::vector<unsigned char>& vc = pm_.getmsg()  ;
			
			//echo<iterator_vc>  ecur(vc.begin()) ;
			iterator_vc first = static_cast<iterator_vc>(vc.begin()) ;
			iterator_vc last = static_cast<iterator_vc>(vc.end()) ;

			 
			boost::spirit::parse_info<iterator_vc> info = 
				boost::spirit::parse(first, last, r);
			
			if(info.hit && info.full)
			{
				pl.s_timestamp = 
					(hh >= 10 ? boost::lexical_cast<std::string, unsigned int>(hh) :
					(std::string("0") + boost::lexical_cast<std::string, unsigned int>(hh)))
					 +  
					(mm >= 10 ? boost::lexical_cast<std::string, unsigned int>(mm) :
					(std::string("0") + boost::lexical_cast<std::string, unsigned int>(mm)))
					+
					(ss >=10 ? boost::lexical_cast<std::string, unsigned int>(ss) :
					(std::string("0") + boost::lexical_cast<std::string, unsigned int>(ss)))
					+ "." + 
					boost::lexical_cast<std::string, unsigned int>(ms) ;
				pl.s_date = boost::lexical_cast<std::string, unsigned int>(udate) ; 
				pl.u_t = (double)hh*3600. + (double)mm * 60. +  (double)ss + (double)ms / 1000. ;
				return 0 ;
			}
			
			return -1 ;
		}
	private: 
		rule_vc r, rtimestamp , rdelim , rhexprefix ;
		unsigned int hh, mm , ss,ms ;
		unsigned int udate ;
	};

}

#endif