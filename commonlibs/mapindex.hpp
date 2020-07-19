#ifndef __COMMON_MAPINDEX_HPP

#define __COMMON_MAPINDEX_HPP

#include <boost/tr1/unordered_map.hpp>
namespace commonlibs {

struct mapindex2{
	int id1 ;
	int id2 ;
	mapindex2(int id1_, int id2_) : id1(id1_) , id2(id2_) {} 

} ;

struct mkeyhash2
    : std::unary_function<mapindex2, std::size_t>
{
    std::size_t operator()(mapindex2 const& r) const
    {
        std::size_t seed = 0;
		boost::hash_combine(seed, r.id1);
		boost::hash_combine(seed, r.id2);
        return seed;
    }
};

struct mkeyeqto2
    : std::binary_function<mapindex2, mapindex2, bool>
{
    bool operator()(mapindex2 const& r1,
        mapindex2 const& r2) const
    {
        return r1.id1 == r2.id1 && 
			r1.id2 == r2.id2 ;
    }
};


struct mapindex3{
	int id1 ;
	int id2 ;
	int id3 ;
	mapindex3(int id1_, int id2_, int id3_) : id1(id1_) , id2(id2_),
	id3(id3_){} 

} ;

struct mkeyhash3
    : std::unary_function<mapindex3, std::size_t>
{
    std::size_t operator()(mapindex3 const& r) const
    {
        std::size_t seed = 0;
		boost::hash_combine(seed, r.id1);
		boost::hash_combine(seed, r.id2);
		boost::hash_combine(seed, r.id3);
        return seed;
    }
};

struct mkeyeqto3
    : std::binary_function<mapindex3, mapindex3, bool>
{
    bool operator()(mapindex3 const& r1,
        mapindex3 const& r2) const
    {
        return r1.id1 == r2.id1 && 
			r1.id2 == r2.id2  &&
			r1.id3 == r2.id3  ;
    }
};


struct mapindexsii{
	std::string s ;
	int id1 ;
	int id2 ;
	mapindexsii(const std::string &s_ ,int id1_, int id2_) : s(s_) , id1(id1_) , id2(id2_) {} 

} ;

struct mkeyhashsii
    : std::unary_function<mapindexsii, std::size_t>
{
    std::size_t operator()(mapindexsii const& r) const
    {
        std::size_t seed = 0;
		boost::hash_combine(seed, r.s);
		boost::hash_combine(seed, r.id1);
		boost::hash_combine(seed, r.id2);
        return seed;
    }
};

struct mkeyeqtosii
    : std::binary_function<mapindexsii, mapindexsii, bool>
{
    bool operator()(mapindexsii const& r1,
        mapindexsii const& r2) const
    {
        return 
			r1.s == r2.s && 
			r1.id1 == r2.id1 && 
			r1.id2 == r2.id2  ;
			
    }
};



}
#endif 