#ifndef __INTER_P
#define __INTER_P

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <memory>
#include <iostream>
#include <stdexcept>
#include <cerrno>
#include <cstring>

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
	int fd_ ;
	void* mapped_addr_ ;
public:
	named_shared_object (char *name_p, unsigned int size = 0) {
		cstr_name = NULL ;
		cstr_name = name_p ;
		u_memsize = size ;
		cstr_name = name_p ;
		p_mem = static_cast<PointerType>(NULL) ;
		fd_ = -1 ;
		mapped_addr_ = MAP_FAILED ;
		//Erase previous shared memory
		if(CType == IP_SERVERMODE)
			shm_unlink(name_p);
	}
	int open ()
	{
		try{
			if(CType == IP_CLIENTMODE)
			{
				fd_ = shm_open(cstr_name, O_RDONLY, 0);
				if(fd_ == -1)
					throw std::runtime_error(std::string("shm_open(read_only) failed: ") + std::strerror(errno));

				struct stat sb;
				if(fstat(fd_, &sb) == -1)
					throw std::runtime_error(std::string("fstat failed: ") + std::strerror(errno));
				u_memsize = static_cast<std::size_t>(sb.st_size);

				mapped_addr_ = mmap(nullptr, u_memsize, PROT_READ, MAP_SHARED, fd_, 0);
				if(mapped_addr_ == MAP_FAILED)
					throw std::runtime_error(std::string("mmap(read_only) failed: ") + std::strerror(errno));

				p_mem = static_cast<PointerType>(mapped_addr_);
				return 0;
			}
			else if(CType == IP_SERVERMODE)
			{
				if(u_memsize == 0) {
					std::cerr << "Size invalid" << std::endl ;
					return -1 ;
				}
				fd_ = shm_open(cstr_name, O_CREAT | O_RDWR, 0666);
				if(fd_ == -1)
					throw std::runtime_error(std::string("shm_open(create) failed: ") + std::strerror(errno));

				if(ftruncate(fd_, static_cast<off_t>(u_memsize)) == -1)
					throw std::runtime_error(std::string("ftruncate failed: ") + std::strerror(errno));

				mapped_addr_ = mmap(nullptr, u_memsize, PROT_READ | PROT_WRITE, MAP_SHARED, fd_, 0);
				if(mapped_addr_ == MAP_FAILED)
					throw std::runtime_error(std::string("mmap(read_write) failed: ") + std::strerror(errno));

				p_mem = static_cast<PointerType>(mapped_addr_);
				std::memset(mapped_addr_, 0, u_memsize);
			}
		}
		catch(const std::runtime_error &ex){
			if(fd_ != -1) { ::close(fd_); fd_ = -1; }
			if(CType == IP_SERVERMODE) shm_unlink(cstr_name);
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
		if(mapped_addr_ != MAP_FAILED)
			munmap(mapped_addr_, u_memsize);
		if(fd_ != -1)
			::close(fd_);
		if(cstr_name != NULL && CType == IP_SERVERMODE) {
			shm_unlink(cstr_name);
			std::cout << "destructed" << std::endl ;
		}
	}

}  ;

typedef named_shared_object<char * , IP_SHARED_MEM , IP_SERVERMODE> named_shared_mem_server ;

typedef named_shared_object<const char * , IP_SHARED_MEM , IP_CLIENTMODE> named_shared_mem_client ;


#endif
