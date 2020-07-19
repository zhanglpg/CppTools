#ifndef __INTER_P
#define __INTER_P

#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <memory>
#include <iostream>
using namespace boost::interprocess ;


#define SM_READWRITE read_write 
#define SM_READONLY  read_only 

enum objecttype {
	IP_SHARED_MEM = 0xF010000,
	IP_MEM_MAPPED_FILE =0xF0200000
} ;

enum creationType {
	IP_SERVERMODE = 0xE0100000, 
	IP_CLIENTMODE = 0xE0200000
} ;
///\brief created a named shared memory object. 
///\param name_p : INPUT, the name of the shared objects
///\param size : INPUT, the size of the shared mem object. 
///return value: 0, success; 
///return -1 : failed because of inter process exception 
template  <typename PointerType, enum objecttype  ObjType, enum creationType CType=IP_SERVERMODE>
class named_shared_object{
private:
	char *cstr_name ;
	std::size_t u_memsize ; 
	PointerType p_mem ;
	std::auto_ptr<shared_memory_object> p_sm ;
	std::auto_ptr<mapped_region> p_reg ;
public:
	named_shared_object (char *name_p, unsigned int size = 0) {
		cstr_name = NULL ;
		cstr_name = name_p ;
		u_memsize = size ;
		cstr_name = name_p ;
		p_mem = static_cast<PointerType>(NULL) ;
		//Erase previous shared memory
		if(CType == IP_SERVERMODE)
			shared_memory_object::remove(name_p);
	}
	int open () 
	{
		try{
			if(CType == IP_CLIENTMODE)
			{
				//shared_memory_object shm(open_only, cstr_name, read_only);	 
				//mapped_region region(shm, read_only);
				p_sm = std::auto_ptr<shared_memory_object>(new shared_memory_object(open_only, cstr_name, read_only)) ;
				p_reg = std::auto_ptr<mapped_region>(new mapped_region(*p_sm, read_only));
				p_mem = static_cast<PointerType>(p_reg->get_address()); 
				u_memsize = static_cast<std::size_t>(p_reg->get_size()) ;
				return  0;
			}
			else if(CType == IP_SERVERMODE)
			{
				if(u_memsize == 0) {
					std::cerr << "Size invalid" << std::endl ;
					return -1 ;
				}
				//Create a shared memory object.
				//shared_memory_object shm(create_only, cstr_name, read_write);	  
				//Set size
				p_sm = std::auto_ptr<shared_memory_object>(new shared_memory_object(create_only, cstr_name, read_write)) ;
				p_sm->truncate((offset_t)u_memsize);
				p_reg = std::auto_ptr<mapped_region>(new mapped_region(*p_sm, read_write));
				
				//Map the whole shared memory in this process
				//mapped_region region(shm, read_write);
				p_mem = static_cast<PointerType>(p_reg->get_address()); 
				u_memsize = static_cast<std::size_t>(p_reg->get_size()) ;
				//Write all the memory to 1
				std::memset(p_reg->get_address(), 0 , p_reg->get_size());
			}
		}
		catch(interprocess_exception &ex){
			shared_memory_object::remove(cstr_name);
			std::cerr << ex.what() << std::endl;
			return -1;
		}
		return 0 ;

	}
	
	PointerType get_address() const 
	{
		return p_mem ;
	}
	std::size_t get_size() const {
		return u_memsize ;
	}
	~named_shared_object ()
	{
		if(cstr_name != NULL && CType == IP_SERVERMODE) {
			shared_memory_object::remove(cstr_name);
			std::cout << "destructed" << std::endl ;
		}
	}

}  ;

typedef named_shared_object<char * , IP_SHARED_MEM , IP_SERVERMODE> named_shared_mem_server ; 

typedef named_shared_object<const char * , IP_SHARED_MEM , IP_CLIENTMODE> named_shared_mem_client ; 






#endif