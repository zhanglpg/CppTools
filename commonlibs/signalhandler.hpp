#ifndef __CTRL_C_HPP
#define __CTRL_C_HPP

#include <csignal>
#include "singleton.hpp"

namespace commonlibs { 


void terminator(int s) ;

class signal_handler : public commonlibs::Singleton<signal_handler>
{ 

friend class Singleton<signal_handler>;

public:
	void init(unsigned int _S) { 
		ref_signal = false ;
		signal(_S , terminator) ;
	 }
	 void set_status() {
		ref_signal = true ;
	 }
	 bool get_interrupt() const  {
		return ref_signal ;
	 }
private:
	signal_handler() {} 
	bool ref_signal ;
};

void terminator(int s)
{
	signal_handler::instance().set_status() ;
}

}
#endif 