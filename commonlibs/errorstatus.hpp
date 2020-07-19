 
 
 #ifndef  ERROR_STATUS_COMMONLIBS_HPP
 #define ERROR_STATUS_COMMONLIBS_HPP
 
 #include "commonlibs/common_errorcode.hpp"
 namespace  commonlibs 
 {
 template <class saver_implementer>
 class errorStatus_Saver 
 {
 protected :
	std::string s_errmsg; 
	unsigned int u_errorcode ;
	
public: 
	
	void set_errstatus(unsigned int u_c , const std::string &s) 
	{
		s_errmsg = s ;
		u_errorcode =u_c ;
		static_cast<saver_implementer *>(this)->save_msg(u_c, s) ;
	}
	unsigned int get_errcode(void)
	{
		return u_errorcode ;
	}
	const std::string & get_errmsg(void)
	{
		return s_errmsg ;
	}
 
 } ;
 
 
 class simple_saver : public errorStatus_Saver<simple_saver> 
 {
  public: 
	void save_msg(unsigned int u_c ,const std::string &s) 
	{
		s_errmsg = s ;
		u_errorcode = u_c ;
	}
 } ;
 
 }
 
 
 #endif

