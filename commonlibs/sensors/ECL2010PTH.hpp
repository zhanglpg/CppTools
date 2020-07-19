#ifndef __ECL2010_PTH_CARD_HPP
#define __ECL2010_PTH_CARD_HPP
#include <memory>
#include <boost/cstdint.hpp>
#include <string>
#include <boost/asio.hpp>
#include <boost/system/error_code.hpp>
#include <boost/array.hpp>
#include "commonlibs/posixtime_util.hpp" 
#include "commonlibs/uniqueptr.hpp"

using namespace boost::asio::ip ;

namespace commonlibs {

#pragma pack(push)
#pragma pack(1) 

	typedef struct ecl2010pth_msg_t {
		uint8_t n_format ; enum {format_1 = 1} ;
							enum{nchannels = 16 };  
		uint8_t ngreen[nchannels] ;	enum {channel_off = 0, channel_on = 1 , channel_unknown = 0xFF} ;
		uint8_t nyellow[nchannels] ;
		uint8_t nred[nchannels] ;      
		uint8_t n_cabinet_status_bit ; enum {cabinet_normal = 0, cabinet_flash = 1} ; 

		ecl2010pth_msg_t () {
			n_format = format_1 ;
		} ;
	} ecl2010pth_msg_t ;

#pragma pack(pop)

	// intersection mapping tables
	class ecl_channel_to_phase{
	public: 
		// intersection id
		enum {
			sanpablo_brighton = 0 , 
			test_inter,
			ninters  
		} ;
		static std::string get_inter_name (int id) {
			switch(id)
			{
			case sanpablo_brighton: 
				return "San Pablo Ave. and Brighton Ave." ;
			case test_inter:
				return "Test intersection" ;
			default: 
				return "" ;
			}
		}
		static uint8_t get_phase(uint8_t ch, int i_id) 
		{
			static const uint8_t map_table_channel[ninters][ecl2010pth_msg_t::nchannels]
				= { {1,2,3,4,5,6,7,8,0,0,0,0,0,0,0,0}, {1,2,3,4,5,6,7,8,0,0,0,0,0,0,0,0}} ;
			if(i_id >= ninters || ch >= ecl2010pth_msg_t::nchannels) {
				// unknown intersection id
				return 0 ;
			}
			else {
				return map_table_channel[i_id][ch] ;
			}
		}
	} ;
	


	class ecl2010pth_interface {
	public: 
		ecl2010pth_interface(boost::asio::io_service &ios_, const int &bind_port_, int verbose_):  
		  bindport(bind_port_) , verbose(verbose_), ios(ios_), 
			  p_socket(new udp::socket(ios_, udp::endpoint(udp::v4(), bind_port_)))
		{
			
			last_error_status = ok ;
			n_ms_last_error = 0 ; 
			n_ms_last_update = 0 ;
			boost::asio::socket_base::non_blocking_io command(true);
			p_socket->io_control(command);
		}

		enum eclstatus {
			ok = 0, udp_cannot_bind = 1 , udp_error = 2 ,
			error_package_format =4 ,unknown_error  
		}; 

		// returns -1 for error, get_last_error for code
		int receive_spat(bool& has_new_package) {
			
			int ret = -1 ;
			has_new_package = false; 
			boost::system::error_code err ;
			//udp::socket socket(ios, udp::endpoint(udp::v4(), bindport));
			if(!p_socket->is_open()) 
			{
				p_socket.reset(new udp::socket(ios, udp::endpoint(udp::v4(), bindport))) ;
				boost::asio::socket_base::non_blocking_io command(true);
				p_socket->io_control(command);
			}
			int s = 0 ;
//			if(p_socket->is_open())
				try {
					//p_socket->non_blocking(true) ;
					s=p_socket->receive(boost::asio::buffer(receive_buf), 0 , err) ;
					if (err && err != boost::asio::error::message_size && err != boost::asio::error::would_block) {
						throw boost::system::system_error(err);
					}

					if (s < packet_size && !err) 
					{
						// buf size too small 
						last_error_status = error_package_format ;
					}
					else if (s >= packet_size && !err){
						eclmsg = *reinterpret_cast<const ecl2010pth_msg_t *>(receive_buf.elems) ;
						if((int)eclmsg.n_format != ecl2010pth_msg_t::format_1)
						{
							last_error_status = error_package_format ;
						}
						else {
							ret = 0; 
							has_new_package = true ;
							eclmsg.n_cabinet_status_bit = (eclmsg.n_cabinet_status_bit & 1) ;
						}
					}
					else { // err == message size or would_block
						ret = 0 ;
					}
					
				}
				catch(const std::exception &e) 
				{
					last_error_status = udp_error ;
					std::cerr << "ecl2010pth_interface::receive_spat error: " << e.what() << std::endl ;
				}
			//socket.set_option(
			uint64_t tnow =  commonlibs::utc_tools::milliseconds_since_epoch_from_now() ;

			if(ret != 0){
				n_ms_last_error = tnow ;
			}
			else if(!err)// ret == 0 : could be "would_block, or successul update'
			{
				n_ms_last_update = tnow ;
			}
			return ret  ;
		}
		const ecl2010pth_msg_t & get_latest_eclmsg() const {
			return eclmsg ;
		}

		const uint64_t & get_ms_last_update() const {
			return this->n_ms_last_update ;
		}
		const uint64_t & get_ms_last_error () const {
			return this->n_ms_last_error ;
		}
		const eclstatus & get_last_error_status () const {
			return this->last_error_status ;
		}
	private:

		

		enum {packet_size = 49, buf_size = packet_size * 1024 } ;
		boost::array<unsigned char, buf_size> receive_buf  ;  

		uint64_t n_ms_last_update , n_ms_last_error ;
		eclstatus last_error_status  ;
		int bindport;
		int verbose ;
		boost::asio::io_service &ios ;
		
		ecl2010pth_msg_t eclmsg ;
		UNIQUE_PTR<udp::socket> p_socket ;
		
	} ;


}


#endif 
