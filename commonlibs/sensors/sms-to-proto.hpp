
///\brief header file to convert SMS and ECL physical interface to protobuf data.
///\brief 
///\brief 02/04/2012 original coding
///	Liping Zhang, Regents of UC

#ifndef _SMS_TO_PROTO_HPP
#define _SMS_TO_PROTO_HPP

#include "commonlibs/sensors/sms-types.hpp"
#include "boost/foreach.hpp"
#include "boost/date_time.hpp"
#include "commonlibs/sensors/sms-SPaT.pb.h"
#include "commonlibs/sensors/ECL2010PTH.hpp"
#include "commonlibs/uniqueptr.hpp"
#include "commonlibs/protoarray.hpp"

namespace commonlibs {
	

// signal phase interface:  use a UDP based ecl2010path_card
class ECL_to_proto {
public:
	enum {max_phase =16} ;
	static void init_SignalPhase (::common_sensors::SignalPhase *p_sp) {
		p_sp->set_status(::common_sensors::SignalPhase::UNKNOWN) ; 
		p_sp->clear_phase_status() ;
		p_sp->set_t_ms_rcv_epoch(0) ;
		for(int i = 0 ; i < max_phase; ++ i) {
			::common_sensors::SignalPhase::PhaseColor pc = ::common_sensors::SignalPhase::COLOR_UNKNOWN ;
			::common_sensors::SignalPhase::PhaseStatus *ps =  p_sp -> add_phase_status() ;
			ps->set_phase_id(i+1) ;
			ps->set_color(pc) ;
		}
	}
	static void init_ECLCommStatus(::common_sensors::ECLCommStatus *p_eclcomm) {
		p_eclcomm->set_last_error_status(::common_sensors::ECLCommStatus::UNKNOWN_ERROR) ;
		p_eclcomm->set_t_ms_last_update(0) ;
		p_eclcomm->set_t_ms_last_error(0) ;
	}

	static void convert_to_ECLCommStatus(const uint64_t &t_ms_last_update, 
		const uint64_t &t_ms_last_error ,  
		const commonlibs::ecl2010pth_interface::eclstatus& ecls, 
		::common_sensors::ECLCommStatus *p_eclcomm) 
	{
		//p_eclcomm.reset(new ECLCommStatus) ;
		::common_sensors::ECLCommStatus::CommStatus cs ; 
		switch(ecls) {
			case commonlibs::ecl2010pth_interface::ok: 
				cs = ::common_sensors::ECLCommStatus::OK ;
				break ;
			case commonlibs::ecl2010pth_interface::udp_cannot_bind: 
				cs = ::common_sensors::ECLCommStatus::UDP_BIND_ERROR ;
				break ;
			case commonlibs::ecl2010pth_interface::udp_error: 
				cs = ::common_sensors::ECLCommStatus::UDP_OTHER_ERROR ;
				break; 
			case commonlibs::ecl2010pth_interface::error_package_format: 
				cs = ::common_sensors::ECLCommStatus::ERROR_PACKET_FORMAT ;
				break; 
			default:	
				cs = ::common_sensors::ECLCommStatus::UNKNOWN_ERROR ;
				break ;
		}
		p_eclcomm->set_last_error_status(cs) ;
		p_eclcomm->set_t_ms_last_error(t_ms_last_error) ;
		p_eclcomm->set_t_ms_last_update(t_ms_last_update) ;
	}


	static void convert_to_SignalPhase (
		const uint64_t & t_ms_rcv_epoch,
		const commonlibs::ecl2010pth_msg_t& ecl_msg, 
		const int &ninterid,
		::common_sensors::SignalPhase *p_sp){

		// read-write mutex
		p_sp->set_t_ms_rcv_epoch(t_ms_rcv_epoch) ;
		if (ecl_msg.n_cabinet_status_bit == commonlibs::ecl2010pth_msg_t::cabinet_normal) {
			p_sp->set_status(::common_sensors::SignalPhase::NORMAL) ;
		}
		else if(ecl_msg.n_cabinet_status_bit == commonlibs::ecl2010pth_msg_t::cabinet_flash) {
			p_sp->set_status(::common_sensors::SignalPhase::FLASHING) ;
		}
		else {
			// shouldnt go here
			p_sp->set_status(::common_sensors::SignalPhase::UNKNOWN) ;
		}

		for(int i = 0 ; i < commonlibs::ecl2010pth_msg_t::nchannels; ++ i) {
			int phase = commonlibs::ecl_channel_to_phase::get_phase(i, ninterid) ;
			if(phase > 0 ) { // valid phase
				::common_sensors::SignalPhase::PhaseColor pc = ::common_sensors::SignalPhase::COLOR_UNKNOWN ;
				if(ecl_msg.ngreen[i] == commonlibs::ecl2010pth_msg_t::channel_on) {
					pc  = ::common_sensors::SignalPhase::GREEN ;
				}
				else if(ecl_msg.nyellow[i] == commonlibs::ecl2010pth_msg_t::channel_on) {
					pc  = ::common_sensors::SignalPhase::YELLOW ;
				}else if(ecl_msg.nred[i] == commonlibs::ecl2010pth_msg_t::channel_on) {
					pc  = ::common_sensors::SignalPhase::RED ;
				}
				else if(ecl_msg.ngreen[i] == commonlibs::ecl2010pth_msg_t::channel_off && 
					ecl_msg.nyellow[i] == commonlibs::ecl2010pth_msg_t::channel_off) // all off
				{
					pc  = ::common_sensors::SignalPhase::RED ;
				}
				else if(ecl_msg.nyellow[i] == commonlibs::ecl2010pth_msg_t::channel_off && 
					ecl_msg.nred[i] == commonlibs::ecl2010pth_msg_t::channel_off) // all off
				{
					pc  = ::common_sensors::SignalPhase::GREEN ;
				}
				else if(ecl_msg.ngreen[i] == commonlibs::ecl2010pth_msg_t::channel_off && 
					ecl_msg.nred[i] == commonlibs::ecl2010pth_msg_t::channel_off) // all off
				{
					pc  = ::common_sensors::SignalPhase::YELLOW ;
				}
				// otherwise, it remains unknown
				
				::common_sensors::SignalPhase::PhaseStatus *ps =  p_sp -> add_phase_status() ;
				ps->set_phase_id(phase) ;
				ps->set_color(pc) ;
			}
		}
	} 	
} ;
	


class sms_to_proto
{
public:
		static void generate_parser_status(const commonlibs::parser_status &ps, ::common_sensors::SMSStatus* p_smsstatus)
		{
				p_smsstatus->Clear() ;
				p_smsstatus ->set_n_total_packets(ps.n_total_packets_rcved) ;
				p_smsstatus->set_n_total_errors(ps.n_total_errs) ;
				p_smsstatus->set_n_total_critical_errors (ps.n_total_criticalerrs) ;

				p_smsstatus->set_last_error(ps.e_last_error); 
				p_smsstatus->set_t_ms_last_critical_error(ps.n_ts_ms_last_critical_error) ; 
				p_smsstatus->set_t_ms_last_error(ps.n_ts_ms_last_error) ;
				p_smsstatus->set_t_ms_last_valid_packet(ps.n_ts_ms_last_valid_packet) ;
				p_smsstatus->set_t_ms_last_obj_control(ps.n_ts_ms_last_obj_control) ;
				p_smsstatus->set_t_ms_last_sensor_control(ps.n_ts_ms_last_sensor_control) ;
				p_smsstatus->set_t_ms_last_obj_data(ps.n_ts_ms_last_objdata) ;
		} ; 

		static int generate_proto(
								const uint64_t &t_ms_rcv_epoch, 
								const commonlibs::sms_candata_objcontrol_t & objc, 
								const commonlibs::sms_candata_sensorcontrol_t & sensorc, 
								const std::vector<commonlibs::sms_candata_objdata_t> &v_objdata,
								::common_sensors::SMSPackage* p_smspackage) 
			{
				//SMSPackage * p_smspackage = p.mutable_sms_package() ;
				p_smspackage->Clear() ;
				p_smspackage->set_t_ms_rcv_epoch(t_ms_rcv_epoch) ;
				p_smspackage->set_nobjs(objc.nobjs) ;
				p_smspackage->set_ntscanms(objc.t_scanms) ;
				if(objc.mode == commonlibs::sms_candata_objcontrol_t::mode_normal)
					p_smspackage->set_mode(::common_sensors::SMSPackage::NORMAL) ; 
				else if(objc.mode == commonlibs::sms_candata_objcontrol_t::mode_simulation)
					p_smspackage->set_mode(::common_sensors::SMSPackage::SIMU) ; 

				p_smspackage->set_ncyclecnt(objc.n_cyclecnt); 
				::common_sensors::SMSPackage::SensorStatus* p_sensor_status = 
					p_smspackage->mutable_sensor_status() ;
				p_sensor_status->set_source_device(sensorc.source_id) ;
				
				p_sensor_status->set_can_status((int)sensorc.can_status) ;

				switch(sensorc.get_ethernet_status())
				{
				case commonlibs::sms_candata_sensorcontrol_t::notconnect: 
					p_sensor_status->set_ethernet_status(::common_sensors::SMSPackage::SensorStatus::NOTCONNECT) ;
					break ;
				case commonlibs::sms_candata_sensorcontrol_t::connecting: 
					p_sensor_status->set_ethernet_status(::common_sensors::SMSPackage::SensorStatus::CONNECTING) ;
					break ;
				case commonlibs::sms_candata_sensorcontrol_t::isconnect: 
					p_sensor_status->set_ethernet_status(::common_sensors::SMSPackage::SensorStatus::ISCONNECT) ;
					break ;
				case commonlibs::sms_candata_sensorcontrol_t::relieve_error: 
					p_sensor_status->set_ethernet_status(::common_sensors::SMSPackage::SensorStatus::RELIEVE_ERROR) ;
					break ;
				case commonlibs::sms_candata_sensorcontrol_t::error_state: 
					p_sensor_status->set_ethernet_status(::common_sensors::SMSPackage::SensorStatus::ERROR_STATE) ;
					break ;
				case commonlibs::sms_candata_sensorcontrol_t::wait_of_repeat_data: 
					p_sensor_status->set_ethernet_status(::common_sensors::SMSPackage::SensorStatus::WAIT_OF_REPEAT_DATA) ;
					break ;
				default:
					p_sensor_status->set_ethernet_status(::common_sensors::SMSPackage::SensorStatus::UNKNOWN_ERROR) ;
					break ;

				}
				p_sensor_status->set_sensorpresent(sensorc.get_sensor_present()) ;
				p_sensor_status->set_sensor_timestamp(sensorc.timestamp) ;

				BOOST_FOREACH(const commonlibs::sms_candata_objdata_t & objdata, v_objdata)
				{
					::common_sensors::SMSPackage::ObjData * p_objdata = 
						p_smspackage->add_obj_data() ;
					p_objdata->set_obj_id(objdata.objid) ;
					p_objdata->set_x(objdata.x) ;
					p_objdata->set_y(objdata.y) ;
					p_objdata->set_vx(objdata.vx) ;p_objdata->set_vy(objdata.vy) ;
					p_objdata->set_length_obj(objdata.length_obj) ;
				}
				return 0 ;
			}

	} ;

}; 

#endif 

