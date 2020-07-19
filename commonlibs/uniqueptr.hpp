#ifndef __UNIQUE_PTR_HPP_
#define __UNIQUE_PTR_HPP_


#ifdef __GNUC__ 
#  include <features.h>
#  if __GNUC_PREREQ(4,4)
	#define _HAS_UNIQUE_PTR
	#include <memory>
	#define UNIQUE_PTR std::unique_ptr
#  else
	#include <boost/scoped_ptr.hpp>
	#define UNIQUE_PTR boost::scoped_ptr
//       Else
#  endif
#elif _MSC_VER > 1500
//	assume windows has c++0x support (come on, install it!) 
	#define _HAS_UNIQUE_PTR 
	#include <memory>
	#define UNIQUE_PTR std::unique_ptr
#else

	#include <boost/scoped_ptr.hpp>
	#define UNIQUE_PTR boost::scoped_ptr
#endif




#endif 
