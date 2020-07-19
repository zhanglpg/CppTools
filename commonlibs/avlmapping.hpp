#ifndef COMMONLIBS_AVL_MAPPING_HPP

#define COMMONLIBS_AVL_MAPPING_HPP

#include "commonlibs/mysqldatabaseConn.hpp"
#include <boost/tr1/unordered_map.hpp>
namespace commonlibs {


sql_create_5(AVLportMapping, 3, 0, 
	std::string, route, 
	std::string, pattern_id, 
	mysqlpp::sql_int, vehicle_id, 
	std::string, mappedIP,
	mysqlpp::sql_int_unsigned, mappedPort)  



struct avlmap_hash
    : std::unary_function<AVLportMapping, std::size_t>
{
    std::size_t operator()(AVLportMapping const& r) const
    {
        std::size_t seed = 0;
        boost::hash_combine(seed, r.route);
		boost::hash_combine(seed, r.pattern_id);
		boost::hash_combine(seed, r.vehicle_id);
        return seed;
    }
};

struct avlmap_eqto
    : std::binary_function<AVLportMapping, AVLportMapping, bool>
{
    bool operator()(AVLportMapping const& r1,
        AVLportMapping const& r2) const
    {
        return r1.route == r2.route && 
			r1.pattern_id == r2.pattern_id && 
			r1.vehicle_id == r2.vehicle_id ;
    }
};

typedef std::tr1::unordered_map<AVLportMapping, unsigned int, 
	avlmap_hash, avlmap_eqto> AVLportmap_type ;

}

#endif 