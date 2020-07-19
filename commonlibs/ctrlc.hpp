#ifndef __CTRL_C_HPP
#define __CTRL_C_HPP

#include <csignal>
#include "singleton.hpp"

namespace commonlibs { 


void terminator(int s) ;

class CTRL_C_handler : public commonlibs::Singleton<CTRL_C_handler>
{ 

friend class Singleton<CTRL_C_handler>;

public:
     void init() { 
		ref_signal = false ;
		signal(SIGINT , terminator) ;
	 }
	 void set_status() {
		ref_signal = true ;
	 }
	 bool get_interrupt() const  {
		return ref_signal ;
	 }
private:
	CTRL_C_handler() {} 
	bool ref_signal ;
};

void terminator(int s)
{
	CTRL_C_handler::instance().set_status() ;
}

}
#endif 
