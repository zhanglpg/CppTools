
#ifndef __SMS_RADAR_PARSER_HPP
#define __SMS_RADAR_PARSER_HPP
// ~~~~~~~~~~~~~~
// Author: Liping Zhang
/// Copy Right: Regents of the University of California
/// 2012

#include <string>
#include "commonlibs/connection_new.hpp"
#include <ctime>
#include <boost/cstdint.hpp>
#include <cstring>
#include "commonlibs/posixtime_util.hpp"
#include "commonlibs/textlogger.hpp"
#include "commonlibs/sleeprelative.hpp"
#include "commonlibs/uniqueptr.hpp"
#include "boost/thread.hpp"
#include "commonlibs/sensors/sms-SPaT.pb.h"
#include "commonlibs/sensors/sms-types.hpp"
#include "commonlibs/sensors/sms-to-proto.hpp"
#include "boost/algorithm/string/find.hpp"
#include "boost/noncopyable.hpp" 
#include "boost/lexical_cast.hpp"

namespace commonlibs 
{
	
	// CAN / Ethernet msg types definitions: adapted from PATH db code (pathdb/sensors/sms/smsparse.h) 


	class smsparser_cmd  {
	public: 
		smsparser_cmd()  : 
			b_sensor_control_inited(false), b_obj_control_inited(false) 
		{
				v_objdata.clear() ;
				v_objdata.reserve(max_obj) ;
				t_ms_rcv_epoch = 0 ;
		} 

		enum parseerror { 
			ok = 0 , 
			size_error = 1, // size < 48 
			headerseq = 2 , // critical 
			tailseq = 3 ,   // critical
			unknown_msgid = 4, // critical 
			truncated_data = 5, 
			critical_error = 6, 
			canbus = 6,  // can bus status error
			sensor_present = 7,  // not enough sensor present
		} ;
		
		enum msg_canid {
			sensorcontrol = 0x500, 
			objectcontrol = 0x501,
			highbyte_obj_min = 0x510, 
			highbyte_obj_max = 0x5FF, 
			getparam = 0x3F2, 
			object_data = 0xAA05
		} ;

		void clear_cycle_data () 
		{
			::memset( &objc, sizeof(objc), 0) ;
			::memset( &sensorc, sizeof(sensorc), 0) ;
			

			b_sensor_control_inited = false ; 
			b_obj_control_inited = false ;
			//v_objdata.clear() ;
			//v_objdata.reserve(max_obj) ;
			v_objdata.resize(0) ;
			n_targets= 0 ; 
			t_ms_rcv_epoch = 0 ;
		}
	
		const sms_candata_objcontrol_t & get_obj_control () const {
			return objc; 
		}
		const sms_candata_sensorcontrol_t & get_sensor_control  ()const {
			return sensorc ;
		}
		const uint64_t & get_t_ms_rcv_epoch() const { 
			return t_ms_rcv_epoch ;
		}
	
		const std::vector<sms_candata_objdata_t> & get_vector_obj_data  () const{
			return v_objdata ;
		}
		const parser_status & get_parser_status () const {
			return ps ;
		}

		bool is_cycle_end () const {
			if (b_sensor_control_inited == true && 
				b_obj_control_inited == true) 
			{
			//	if (v_objdata.size() == n_targets) 
					return true ; // cycle_end
			}
			return false ;
		}


	// each package is composed by: a training sequence, a header, one or multiple CAN messages (10 bytes each), then the trailer. 
	int parsemsg( enum parseerror &error, unsigned char *buf ,size_t size)
			//std::vector<std::string> &v_s_output)
	{
		
		int consumed_length = 0;
		int r = 0 ;
		while(r == 0 && consumed_length < (int)size) {
			int tlength = consumed_length ;
			r = parsemsg_imp(error, buf + consumed_length, size - consumed_length, //v_s_output,
				tlength) ;
			consumed_length += tlength ;
		}
		
		return r ;
	}

	private:
	
	int parsemsg_imp(enum parseerror& error, unsigned char *buf, size_t size, 
			//std::vector<std::string> &v_s_output, 
			int& consumed_length) 
		{
			uint64_t n_ts_ms_current = utc_tools::milliseconds_since_epoch_from_now() ;
			ps.n_total_packets_rcved ++ ;
			int npayloads = 0 ;
			consumed_length = check_integrity_payloads(error, buf, size, npayloads) ;
			ps.e_last_error = error ;
			if(error != ok)
			{
				ps.n_total_errs ++ ;
				ps.n_ts_ms_last_error = n_ts_ms_current ;
			}
			else {
				ps.n_ts_ms_last_valid_packet = n_ts_ms_current ;
			}
			if(iscritical(error)) { 
				ps.n_total_criticalerrs ++ ;
				ps.n_ts_ms_last_critical_error = n_ts_ms_current ;
				consumed_length = 0 ;
				return -1 ;
			}

			b_sensor_control_inited = false ;
			b_obj_control_inited = false; 
			n_targets = 0 ;
			//v_objdata.clear() ;
			//v_objdata.reserve(max_obj) ;
			v_objdata.resize(0) ;
			t_ms_rcv_epoch = n_ts_ms_current ;
			for (int i = 0 ; i < npayloads ; ++ i) 
			{
				
			//	std::stringstream ss ;
				const smsmsg_canpayload_t& smst = * reinterpret_cast<smsmsg_canpayload_t *>(&buf[sizeof(smsmsg_ethernet_train_header_t) + i * 
					sizeof(smsmsg_canpayload_t)]) ;
				uint16_t CAN_ID = switch_bytes(smst.CAN_ID) ;
				
				
				//	std::cerr  << "CANID == " << std::hex << CAN_ID << std::dec << std::endl; 
			
				if (CAN_ID == sensorcontrol) 
				{
					sensorc = *reinterpret_cast<const sms_candata_sensorcontrol_t *>(&smst.data[0]) ;
				//	print_log(sensorcontrol, ss, smst) ;
					ps.n_ts_ms_last_sensor_control = n_ts_ms_current ;
					b_sensor_control_inited= true ;
				}
				else if(CAN_ID == objectcontrol) 
				{
					objc =  *reinterpret_cast<const sms_candata_objcontrol_t *>(&smst.data[0]) ;
			//		print_log(objectcontrol, ss, smst) ;
					ps.n_ts_ms_last_obj_control = n_ts_ms_current ;
					b_obj_control_inited= true; 
			//		v_objdata.clear() ;
					n_targets = (int)objc.nobjs ;
				}
				else if(CAN_ID == getparam)
				{
					/// 
				}
				else if(CAN_ID >= highbyte_obj_min && CAN_ID <= highbyte_obj_max)
				{
					//::memset( &obj, sizeof(obj), 0) ;
				
					obj.objid = (uint8_t) (((smst.data[7]) >> 2) & 0X3F);
					 // doesn't matter if smsarr is ever cleared,
					 // only object IDs in the object list arrays
					 // are valid, and new data is written only for them 
				
					// x and y are in meters. 
					obj.x = 0.1 * (double) (((smst.data[0]) + ((smst.data[1] & 0X3F) << 8)) - 8192);
					obj.y = 0.1 * (double) (((((smst.data[1] & 0XC0) >> 6) + (smst.data[2] << 2) + ((smst.data[3] & 0X0F) << 10)) - 8192));

					// vx and vy are in meters/ second
					obj.vx =0.1 *  (double) (((((smst.data[3] & 0XF0) >> 4) + (((smst.data[4] & 0X7F)) << 4)) - 1024));
					obj.vy = 0.1 * (double) (((((smst.data[4] & 0X80) >> 7) + (((smst.data[5])) << 1) + (((smst.data[6] & 0X03)) << 9)) - 1024));

					// length is in meters
					obj.length_obj = 0.2 * (double) (((smst.data[6] >> 2) & 0X3F) + ((smst.data[7] << 6) & 0XC0));
					obj.n_cyclecnt = objc.n_cyclecnt ;
					v_objdata.push_back(obj) ;
					ps.n_ts_ms_last_objdata = n_ts_ms_current ;
				//	print_log(object_data, ss, smst) ;
				}
			//	v_s_output.push_back(ss.str()) ;
			}
			
			return 0 ;
		}

		bool iscritical(const int& error)  // critical errors are discarded. 
		{
			return (error < (int)critical_error) && error != ok ;
		}
		int check_integrity_payloads(enum parseerror &error, const unsigned char *buf, const size_t& size, 
			int& n_payloads) 
		{
			static const char *p_tail = "EDEDEDEDE" ;  enum {tail_length = 9} ;
			static const char *p_train ="AFAFAFAFA" ; enum {train_length = 9} ;

			error = ok ;
			int consumed_length = 0;
			if(size < sizeof(smsmsg_length_min))
			{
				error = size_error ;
			}
			else {
				// size ok, 
				if ( ::strncmp((char *)&buf[smsmsg_ethernet_train_header_t::offset_train], p_train, train_length)  )
				{
					error = headerseq ;
				}
				else  {
					// find the payload , first search for tail data
					
					
					//size_t tailstart = size - tail_length ;
					size_t result = 0 ;
					for (result = train_length ; result <= size - tail_length ;  ++ result) {
						if(! ::strncmp((char *)&buf[result], p_tail, tail_length)) {
							break ;
						}
					}
					//size_t found = str.find_first_of(p_tail, train_length, tail_length) ;
					//if(::strncmp( (char *) &buf[tailstart], p_tail, tail_length)) 
					if(result> size - tail_length)
					{
						error = tailseq ;
					}
					else { 
						
						size_t bytes_in_payload = result -sizeof(smsmsg_ethernet_train_header_t) ; // size - tail_length - sizeof(smsmsg_ethernet_train_header_t) ;
						if(bytes_in_payload < sizeof(smsmsg_canpayload_t) || 
							(bytes_in_payload % sizeof(smsmsg_canpayload_t)) != 0)
							
						{
							std::cerr << " Error truncation of data : tailstart = " << result <<" , bytes_in_payload = " << bytes_in_payload 
								<< ", overall size = " << size << " (header size = " <<sizeof(smsmsg_ethernet_train_header_t) << ")  \n" ;

							error = truncated_data;  // not a multiple of smsmsg_canpayload_t
						}
						else {
							// check if canid is known
							n_payloads = bytes_in_payload / sizeof(smsmsg_canpayload_t) ;
							for(int i = 0 ; i < n_payloads ; ++ i) 
							{
								const uint16_t canid = *reinterpret_cast<const uint16_t*>(&buf[sizeof(smsmsg_ethernet_train_header_t) + i * sizeof(smsmsg_canpayload_t)]) ;

								uint16_t n_canid =  switch_bytes(canid); // switch byte sequence
								if (n_canid != sensorcontrol && n_canid != objectcontrol && 
									(n_canid < highbyte_obj_min || n_canid > highbyte_obj_max) && 
									n_canid != getparam) {
									error = unknown_msgid ;
									
								}
							} // find wrong msg id
							consumed_length = result + tail_length ;
						} // size inside payload
					} // find tail 
				} //train seq
			} // check size 
			return consumed_length ;
		}
		

	                     //end of print_log

		uint16_t switch_bytes(const uint16_t &n) {
			return ( (n& 0xFF) << 8) + ((n & 0xFF00) >> 8) ;
		}

		

		uint32_t n_cyclecnt, u_prev_cyclecnt ;

		parser_status ps ; 
			
		bool b_sensor_control_inited , b_obj_control_inited ;
		sms_candata_objcontrol_t objc ;
		sms_candata_sensorcontrol_t sensorc ;
		sms_candata_objdata_t obj ;

		enum {max_obj = 256 } ;
		uint32_t n_targets ; // current number of targets ;
		std::vector<sms_candata_objdata_t> v_objdata ;

		uint64_t t_ms_rcv_epoch ; 

	} ;

	 	


class smsinterface : private boost::noncopyable {
		public:
			smsinterface(boost::asio::io_service& io_service, 
				const std::string &shost, const int &port_, int verbose_)
				: 
				con_p(new commonlibs::connection_tcp_server(io_service, port_)), 
				smshost(shost), smsport(port_), verbose(verbose_)
			
			{
				con_p->setverbose_level(verbose) ;
				connected = false ;
				finished = false ;
				b_has_new_data  = false ;

				boost::asio::ip::tcp::resolver resolver(io_service);	
				boost::asio::ip::tcp::resolver::query query(shost, boost::lexical_cast< std::string, int>(port_));		
				
				boost::asio::ip::tcp::endpoint endp = * resolver.resolve(query);
				address_bumper_box = endp.address().to_v4() ;

				p_thread_smsi.reset(new boost::thread(boost::bind(& smsinterface::start_async, this)) ) ;
			}

			~smsinterface() { 
				set_finished(true) ;	
			}	
			bool isconnected() const { return connected ; } 
			void set_finished(bool b)  {
				finished = b ;
			}


			const parser_status &get_sms_parser_status()  {
				boost::lock_guard<boost::mutex> L_(m_mutex) ;
				return smsp.get_parser_status() ;
			}


			bool has_new_data() const {
				return b_has_new_data  ;
			}
			
			void get_sms_data (::common_sensors::SMSPackage* p_smspackage, ::common_sensors::SMSStatus *p_smsstatus)
					//std::vector<std::string> &v_s_out) 
			{
				p_smspackage->Clear() ;
				boost::lock_guard<boost::mutex> L_(m_mutex) ;
				commonlibs::sms_to_proto::generate_proto(
					smsp.get_t_ms_rcv_epoch() ,
					smsp.get_obj_control(), smsp.get_sensor_control(), 
					smsp.get_vector_obj_data(), p_smspackage) ;
				commonlibs::sms_to_proto::generate_parser_status(smsp.get_parser_status() , p_smsstatus) ;
			//	v_s_out= v_s_txt ;
				b_has_new_data = false ; // clear the has new data flag
			}


			

			void start_async() {
				finished = false ;
				while(!finished)  {
					try {
						int timeoutms = 5000 ;
						boost::system::error_code ec; 
						getconnected(ec, timeoutms, true) ;
						while (!finished){
							int n = 0 ;
							if(isconnected())  {
								n = read_message_raw() ;
							}
							if (n < 0 || ! isconnected()) {
								// sleep a little bit before trying to reconnect
								commonlibs::sleep_relative_ms(2000) ;
								close_and_reconnect(timeoutms, true) ;
							}

							if (n <= 0) {
								// only sleeps when there is no data available or has been an error
								commonlibs::sleep_relative_ms(10) ;
							}
						}	
					}
					catch(const std::exception &e)
					{
						//cbreak = true ;		
						std::cerr << " Exception -- " << e.what() << std::endl ;
					}
				}
			}

			int sms_set_simu_mode(bool simu) {
				uint8_t devID = 0 ;
				uint8_t parm = 0 ;
				uint8_t parm_type = 0 ;
				uint8_t action = ::commonlibs::sndmsg_t::action_bumper_parameter_change ;
				int value = (int) ::commonlibs::sndmsg_t::simu_disable;
				if(simu) {
					value = (int) ::commonlibs::sndmsg_t::simu_enable;
				}
				return smscommand(::commonlibs::sndmsg_t::cmdtarg_Bumber, 
						devID , parm, parm_type ,action, value) ;
			}

			int smscommand(
				int targ,  // enum, 0 for bumper , 1 for sensor
				const uint8_t devID, const uint8_t& parm, const uint8_t &parm_type, const uint8_t& action,
				const int& value)
			{
			   ::commonlibs::smscmd_t command ;
			   ::memset(&command.trainseq[0], 0xC4, 9) ;
			   ::memset(&command.tailseq[0], 0xE7, 9) ;
			   command.numpkts = 1 ; // one packet at a time
			   command.pktno = 0 ;
			   boost::asio::ip::address_v4::bytes_type bs = 
				   address_bumper_box.to_bytes() ;
			   command.ip[0] = bs[0] ;
			   command.ip[1] = bs[1] ;
			   command.ip[2] = bs[2] ;
			   command.ip[3] = bs[3] ;
			   command.canno = 0 ;
			   command.CAN_ID_high_byte = 0x3 ;
			   command.CAN_ID_low_byte = 0xF2 ;
			   command.cmd_data.cmdtarg = (uint8_t) targ ;
			   command.cmd_data.data[0] = devID;
			   command.cmd_data.data[1] = parm;
			   command.cmd_data.data[2] = parm_type;
			   command.cmd_data.data[3] = action;
			   command.cmd_data.value = value ;
			   boost::system::error_code err; 
			   
				try {
					boost::lock_guard<boost::mutex> L_(m_mutex_socket) ;
					con_p->write_msg(err, &command, sizeof(command)) ;
					if(err) 
						throw boost::system::system_error(err) ;
				}
				catch(const std::exception &e) {
					std::cerr << "smsinterface:: Write command to SMS failed: " << e.what() << std::endl ;
					return -1 ; 
				}

			   return 0;
			}


private:
		int close_and_reconnect(int timeoutms = 1000, bool async = false) 
			{
				boost::lock_guard<boost::mutex> L_(m_mutex_socket) ;
				boost::system::error_code ec; 
				boost::asio::io_service& io_s = con_p->io_service() ;
				con_p.reset(new commonlibs::connection_tcp_server(io_s, smsport)) ;
				getconnected(ec, timeoutms , async) ;
				if(!ec) 
				{
					con_p->set_option_keepalive(true) ;
					connected = true ;
					return 0 ; // good 
				}
				else {
					connected = false; 
					std::cerr << "smsinterface::close_and_reconnect_async: " <<ec.message() << std::endl ;
				}
				return -1 ;
			}
			
			int getconnected(boost::system::error_code &ec, int timeoutms = 1000, bool async = false)
			{
				boost::lock_guard<boost::mutex> L_(m_mutex_socket) ;
				if(verbose > 1) 
					std::cout << "Start connecting to host " << smshost << ":" << smsport << std::endl ;
				//boost::system::error_code ec; 
				if (async)
					con_p->async_accept_host(smshost,timeoutms, ec); 
				else 
					con_p->sync_accept_host(smshost,  ec); 
					
				if(!ec) 
				{
					if(verbose)
					{
						std::cout << "smsinterface::getconnected("<<async <<"): Connected" << std::endl ; 
					}
					if(verbose > 1)
						std::cout << "smsinterface::getconnected set options\n" ; 	
					con_p->set_option_keepalive(true) ;
					con_p->set_option_nonblock(true) ;
					connected = true ;
					return 0 ; // good 
				}
				else {
					connected = false ; 
					std::cout << "smsinterface::getconnected("<<async <<") failed :" << ec.message() << std::endl ;
				}
				return -1 ;
			}
			
			
			int read_message_raw() 
			{
				int n= 0 ; 
				try {
					boost::system::error_code ec;
					do 
					{
						boost::lock_guard<boost::mutex> L_(m_mutex_socket) ;
						n = con_p->sync_read_raw(ec, buf.elems, MAX_BUFFER_SIZE) ;
					}
					while(false) ;
					//boost::asio::streambuf::mutable_buffers_type bufs = inputbuf_.prepare(MAX_BUFFER_SIZE) ;
					//n = con.sync_read_raw(ec, bufs) ;
					if(ec && ec!= boost::asio::error::would_block &&
						ec!= boost::asio::error::timed_out) 
					{
						std::cerr <<  "smsinterface::read_message_raw: " <<ec.message() << std::endl ;
						return -1 ;
					}
					else {
						enum commonlibs::smsparser_cmd::parseerror error = commonlibs::smsparser_cmd::ok ;
						if (n > 0) {
							if(verbose >2 ) 
							{
								std::cout << "size (" << n << "):" ;
								std::cout.setf ( std::ios::hex, std::ios::basefield ); 
								std::cout.setf ( std::ios::showbase );    
								
								for (int i = 0 ; i< n ; ++ i){
									std::cout << std::hex << (unsigned int)buf.elems[i] << "," ; 
								}
								std::cout << std::dec << std::endl  ;
							}
							
							boost::lock_guard<boost::mutex> L_(m_mutex) ;
							smsp.clear_cycle_data() ;
							//v_s_txt.clear() ;
							int r = smsp.parsemsg(error, (unsigned char *)buf.elems, n) ; //, v_s_txt) ;
							if (r == 0)
							{
							//	lc_txt.log_string(s_output) ;
							}
							else if (verbose){
								std::cerr << "Parse error! " <<  std::endl ;
								if(verbose >1 )
								{
									std::cerr << " Error was " << error << std::endl ;
									for (int i = 0 ; i< n ; ++ i){
										std::cerr << std::hex << (unsigned int)buf.elems[i] << "," ; 
									}
									std::cerr << std::dec << std::endl  ;
								}

							}
							if (r == 0 && error == commonlibs::smsparser_cmd::ok) // successful 
							{
								b_has_new_data = true ;
							}
							
						}

					}
				}
				catch(const std::exception &e_)
				{
					std::cerr << "smsinterface::read_message_raw - Exception: " << e_.what() << std::endl ;
					n = -1; 
				}
				return n ;
			}

		//	int sms_get_target_list(std::vector<int> &v_targets, int par) ;
		//	int sms_get_raw_data(std::vector<int> &v_tardata, int tarid = -1) ;
		private: 
				

			enum {MAX_BUFFER_SIZE= 65536*100} ;
			boost::array<unsigned char , MAX_BUFFER_SIZE> buf ;
			//boost::asio::streambuf inputbuf_ ;
			//boost::asio::streambuf::mutable_buffers_type bufs ;
			std::string smshost ;
			int smsport ;
			int verbose;

			UNIQUE_PTR<commonlibs::connection_tcp_server> con_p ;
			//connection_tcp con; 
			commonlibs::smsparser_cmd      smsp ;
			bool connected  ;
			bool finished ; 
			UNIQUE_PTR<boost::thread> p_thread_smsi ;
			boost::mutex  m_mutex; 

			boost::mutex  m_mutex_socket; 

			// flag of whether data has been set / read
			bool b_has_new_data  ; 
			
			
			boost::asio::ip::address_v4 address_bumper_box ;
			
			//std::vector<std::string> v_s_txt ;
	} ;
	

}

#endif
