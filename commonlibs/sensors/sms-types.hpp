#ifndef __SMS_TYPES_HPP
#define __SMS_TYPES_HPP

namespace commonlibs{ 
#pragma pack(push)
#pragma pack(1)  // required to pack bytes to match definition of the hardware 
	
	typedef struct {
		uint16_t CAN_ID;	
		uint8_t data[8];	enum { payload_size= sizeof(uint16_t) + 8 * sizeof(uint8_t)} ;
	} smsmsg_canpayload_t ;
	
	typedef struct {
		char trainseq[9];   enum { offset_train = 0} ; enum { offset_header = 0} ;
		uint8_t version;	enum { offset_version = offset_train + 9 * sizeof(char)} ;  
		uint8_t numbytes;	enum { offset_numbytes =offset_version + sizeof(uint8_t)} ;
		uint8_t ip[4];		enum { offset_ip =offset_numbytes + sizeof(uint8_t)} ;
		uint8_t  lasterr;   enum { offset_lasterr = offset_ip + sizeof(uint8_t) * 4} ; 
		uint16_t errcnt;	enum { offset_errcnt = offset_lasterr + sizeof(uint8_t)} ; 
		uint8_t nosynccnt;	enum { offset_nosynccnt = offset_errcnt + sizeof(uint16_t)} ;
		uint16_t diag;		enum { offset_diag = offset_nosynccnt + sizeof(uint8_t)} ;
		uint8_t can2err;	enum { offset_can2err = offset_diag + sizeof(uint16_t)} ;
		uint8_t can3err;	enum { offset_can3err = offset_can2err + sizeof(uint8_t)} ;
		uint8_t last_umrr;	enum { offset_lastumrr = offset_can3err + sizeof(uint8_t)} ;
		uint8_t unused[3];	enum { offset_unused = offset_lastumrr + sizeof(uint8_t)} ;
		uint16_t crc;		enum { offset_crc = offset_unused + 3 * sizeof(uint8_t)} ;
	}  smsmsg_ethernet_train_header_t ; 

	enum { smsmsg_length_min =sizeof(smsmsg_ethernet_train_header_t) + sizeof(smsmsg_canpayload_t) + sizeof(uint8_t) * 9} ;

	typedef struct {
		uint8_t nobjs ;
		uint8_t nmsgs ;
		uint8_t t_scanms ;
		uint8_t mode ;    enum {mode_normal = 0 , mode_simulation = 4 }  ;
		uint32_t n_cyclecnt ;
	} sms_candata_objcontrol_t ; // 0x501

	typedef struct {
		uint8_t source_id ;
		uint8_t can_status ;  enum { noerror = 0 , invalid_umrr = 1 , unused = 2 , invalidsensorid = 4 , unused2 = 8, wrongtargetnumber = 0x10 , 
			canhw_reinit = 0x20 , sensor_com_incomplete=0x40 , proc_duration_too_long = 0x80} ;

		uint16_t ether_sensor_status ;  enum { notconnect = 0 , connecting = 1 , isconnect = 2 , relieve_error = 3 , 
			error_state = 4 , wait_of_repeat_data = 5} ;
		uint32_t timestamp ;
		const uint16_t get_sensor_present() const {
			return (ether_sensor_status & 0xFFF) ;
		}
		const uint8_t get_ethernet_status() const {
			return ((ether_sensor_status & 0xF000) >> 12) ;
		}
	} sms_candata_sensorcontrol_t ; // 0x500

	typedef struct {
		double x ;
		double y ;
		double vx, vy ;
		double length_obj ;
		uint32_t n_cyclecnt ;
		uint8_t objid ;
	} sms_candata_objdata_t ;  


	typedef struct
	{
   	 
	  uint8_t cmdtarg; enum { cmdtarg_Bumber =  0, cmdtarg_Sensor = 1} ;
	  uint8_t data[4]; 
						enum{action_high_level_software_reset = 1, 
							action_low_level_software_reset =2 , 
							action_bumper_parameter_change = 64, 
							action_tracking_parameter_change = 65 , 
							address_UMRR_sensor_within_tracking_base = 72 , 
							program_parameters_into_flash =100, 
							pass_on_parameter_to_sensor = 200} ;

						int value;		enum {simu_disable =0 , simu_enable = 4} ;
	} sndmsg_t;

	typedef struct
	{
	  char trainseq[9];
	  uint8_t numpkts;
	  uint8_t pktno;
	  uint8_t ip[4];
	  uint8_t canno;
	  uint8_t CAN_ID_high_byte;
	  uint8_t CAN_ID_low_byte;
	  sndmsg_t cmd_data;
	  char tailseq[9];
	} smscmd_t;

#pragma pack(pop)

	typedef struct parser_status {
			uint64_t n_total_packets_rcved, n_total_errs, n_total_criticalerrs, n_canerrs ; 
			uint64_t n_ts_ms_last_valid_packet, n_ts_ms_last_error , n_ts_ms_last_sensor_control, n_ts_ms_last_obj_control, n_ts_ms_last_objdata , n_ts_ms_last_critical_error;
			uint32_t e_last_error ;
			parser_status( ) { 
				n_total_packets_rcved = 0 ;
				n_total_errs = 0 ;
				n_total_criticalerrs = 0 ;
				n_canerrs = 0; 
				n_ts_ms_last_valid_packet = 0 ;
				n_ts_ms_last_error = 0;
				n_ts_ms_last_sensor_control = 0 ; 
				n_ts_ms_last_obj_control = 0 ; 
				n_ts_ms_last_objdata  = 0 ;
				n_ts_ms_last_critical_error = 0 ;
				e_last_error = 0 ;
			}
	} parser_status  ;

	typedef struct sms_package {
		parser_status ps ;
		sms_candata_objcontrol_t candata ;
		sms_candata_objcontrol_t objcontrol ;
		std::vector<sms_candata_objdata_t> v_obj ;
	} sms_package ;


}
#endif 

