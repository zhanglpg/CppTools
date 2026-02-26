#pragma once
#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <cstring>
#include <iostream>
#include <memory>

using namespace boost::interprocess;

#define SM_READWRITE read_write
#define SM_READONLY  read_only

enum objecttype {
    IP_SHARED_MEM       = 0xF010000,
    IP_MEM_MAPPED_FILE  = 0xF0200000
};

enum creationType {
    IP_SERVERMODE = 0xE0100000,
    IP_CLIENTMODE = 0xE0200000
};

/// Creates a named shared memory object.
/// \param name_p  Name of the shared object.
/// \param size    Size of the shared memory (server mode only).
/// Returns 0 on success, -1 on failure.
template <typename PointerType,
          enum objecttype  ObjType,
          enum creationType CType = IP_SERVERMODE>
class named_shared_object {
private:
    const char *cstr_name;
    std::size_t u_memsize;
    PointerType p_mem;
    std::unique_ptr<shared_memory_object> p_sm;
    std::unique_ptr<mapped_region>        p_reg;

public:
    named_shared_object(char *name_p, unsigned int size = 0)
        : cstr_name(name_p), u_memsize(size),
          p_mem(static_cast<PointerType>(nullptr))
    {
        if (CType == IP_SERVERMODE)
            shared_memory_object::remove(name_p);
    }

    int open() {
        try {
            if (CType == IP_CLIENTMODE) {
                p_sm  = std::make_unique<shared_memory_object>(open_only, cstr_name, read_only);
                p_reg = std::make_unique<mapped_region>(*p_sm, read_only);
                p_mem = static_cast<PointerType>(p_reg->get_address());
                u_memsize = static_cast<std::size_t>(p_reg->get_size());
                return 0;
            } else if (CType == IP_SERVERMODE) {
                if (u_memsize == 0) {
                    std::cerr << "Size invalid\n";
                    return -1;
                }
                p_sm = std::make_unique<shared_memory_object>(create_only, cstr_name, read_write);
                p_sm->truncate(static_cast<offset_t>(u_memsize));
                p_reg = std::make_unique<mapped_region>(*p_sm, read_write);
                p_mem = static_cast<PointerType>(p_reg->get_address());
                u_memsize = static_cast<std::size_t>(p_reg->get_size());
                std::memset(p_reg->get_address(), 0, p_reg->get_size());
            }
        } catch (interprocess_exception &ex) {
            shared_memory_object::remove(cstr_name);
            std::cerr << ex.what() << '\n';
            return -1;
        }
        return 0;
    }

    PointerType get_address() const { return p_mem; }
    std::size_t get_size()    const { return u_memsize; }

    ~named_shared_object() {
        if (cstr_name != nullptr && CType == IP_SERVERMODE) {
            shared_memory_object::remove(cstr_name);
            std::cout << "destructed\n";
        }
    }
};

typedef named_shared_object<char *,       IP_SHARED_MEM, IP_SERVERMODE> named_shared_mem_server;
typedef named_shared_object<const char *, IP_SHARED_MEM, IP_CLIENTMODE> named_shared_mem_client;
