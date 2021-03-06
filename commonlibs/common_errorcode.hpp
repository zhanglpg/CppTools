

#ifndef COMMON_ERROR_CODE_HPP
#define COMMON_ERROR_CODE_HPP

namespace commonlibs {

typedef enum  common_error_code {
	STATUS_OK = 0 ,
    STATUS_GENERAL_ERROR = 1002 ,
    STATUS_MESSAGE_ERROR  = 1003 ,
    STATUS_COMM_ERROR  = 1004,
	STATUS_ID_ERROR = 1005, 
	STATUS_CRITICAL_ERROR = 10000, 
	STATUS_COMM_OUTAGE_ERROR = STATUS_CRITICAL_ERROR+ 1, 
	STATUS_DATABASE_ERROR = STATUS_CRITICAL_ERROR + 2
} common_error_code;


}


#endif 
