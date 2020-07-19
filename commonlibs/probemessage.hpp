///	\brief Implementation of the communication based on HDLC frame 
///	\brief Liping Zhang , PATH, UCB

#ifndef _PROBEMESSAGE_H
#define _PROBEMESSAGE_H


#include <vector>
#include <iostream>
#include <boost/asio.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/thread.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <queue>

#include "boost/spirit.hpp"
#include "boost/spirit/include/classic.hpp"

using boost::lexical_cast ;
using boost::bad_lexical_cast ;
using namespace std ;

namespace commonlibs {

enum specialChars
{
	HEADTERM = 0x7E, 
	MAGICBYTE =0x7D, 
	CLONEMAGIC = 0x5D ,
	CLONEMAGIC7E = 0x5E ,
	COMACK = 0xFE ,
	COMCOM = 0xFD ,
	COMTIMEDATE = 0xFF ,
	COMGPS = 0xCC ,
	COMSTAT = 0xBB, 
	COMMODEQUERY = 0x33, 
	COMDESC = 0xEE,
	COMNOTSET =0x55,
	COMSET =0xAA
} ;


template <class Tc, class Tcontainer> 
class basic_charmessage {
public: 
	basic_charmessage() {
	} 
public:  
	basic_charmessage(const Tc * T_ , int s_,  bool b_quiet = false) {} 
	typedef Tc Tchar ;
	const  Tcontainer & getmsg() const {return msg ; } 
protected: 
	enum {MAX_MSG_LENGTH = 65536 * 100 } ;
	Tcontainer msg ; 	
}; 


template <> 
class basic_charmessage<unsigned char, std::vector<unsigned char> >  {
public: 
	basic_charmessage() {
	} 
	
public: 
	basic_charmessage(const unsigned char * T_ , int s_,  bool b_quiet = false)
	{
		try {
			msg.reserve(s_ ) ;
			msg.resize(0) ;
			for(unsigned int i_ = 0 ; i_ < ((static_cast<unsigned int>(MAX_MSG_LENGTH) <=
				static_cast<unsigned int>(s_))? static_cast<unsigned int>(MAX_MSG_LENGTH) : static_cast<unsigned int>(s_));
				++ i_)
			{
				msg.push_back(T_[i_]) ;
			}
		}//try
		catch(const std::exception &e_)
		{
			std::cerr << "probemessage() , system error: " << e_.what() <<endl ;
		}
	}
	const  std::vector<unsigned char> & getmsg() const {return msg ; } 
protected: 
	enum {MAX_MSG_LENGTH = 65536 * 100 } ;
	std::vector<unsigned char> msg ; 
}; 
template <> 
class basic_charmessage<char, std::vector<char> >  {
public: 
	basic_charmessage() {
	} 
	
public: 
	basic_charmessage(const char * T_ , int s_,  bool b_quiet = false)
	{
		try {
			msg.reserve(s_ ) ;
			msg.resize(0) ;
			for(unsigned int i_ = 0 ; i_ < ((static_cast<unsigned int>(MAX_MSG_LENGTH) <=
				static_cast<unsigned int>(s_))? static_cast<unsigned int>(MAX_MSG_LENGTH) : static_cast<unsigned int>(s_));
				++ i_)
			{
				msg.push_back(T_[i_]) ;
			}
		}//try
		catch(const std::exception &e_)
		{
			std::cerr << "probemessage() , system error: " << e_.what() <<endl ;
		}
	}
	const  std::vector<char> & getmsg() const {return msg ; } 
protected: 
	enum {MAX_MSG_LENGTH = 65536 * 100 } ;
	std::vector<char> msg ; 
}; 

template <> 
class basic_charmessage<unsigned char, std::basic_string<unsigned char> >  {
public: 
	basic_charmessage() {
	} 
	
public: 
	basic_charmessage(const unsigned char * T_ , int s_,  bool b_quiet = false)
	{
		try {
			msg = std::basic_string<unsigned char>(T_ , static_cast<size_t>(s_)) ;
			/*for(unsigned int i_ = 0 ; i_ < ((static_cast<unsigned int>(MAX_MSG_LENGTH) <=
				static_cast<unsigned int>(s_))? static_cast<unsigned int>(MAX_MSG_LENGTH) : static_cast<unsigned int>(s_));
				++ i_)
			{
				msg.push_back(T_[i_]) ;
			} */ 
		}//try
		catch(const std::exception &e_)
		{
			std::cerr << "probemessage() , system error: " << e_.what() <<endl ;
		}
	}
	const  std::basic_string<unsigned char> & getmsg() const {return msg ; } 
protected: 
	std::basic_string<unsigned char> msg ; 
}; 

template <> 
class basic_charmessage<char, std::basic_string<char> >  {
public: 
	basic_charmessage() {
	} 
	
public: 
	basic_charmessage(const char * T_ , int s_,  bool b_quiet = false)
	{
		try {
			msg = std::basic_string<char>(T_ , static_cast<size_t>(s_)) ;
			/*for(unsigned int i_ = 0 ; i_ < ((static_cast<unsigned int>(MAX_MSG_LENGTH) <=
				static_cast<unsigned int>(s_))? static_cast<unsigned int>(MAX_MSG_LENGTH) : static_cast<unsigned int>(s_));
				++ i_)
			{
				msg.push_back(T_[i_]) ;
			} */ 
		}//try
		catch(const std::exception &e_)
		{
			std::cerr << "probemessage() , system error: " << e_.what() <<endl ;
		}
	}
	const  std::basic_string<char> & getmsg() const {return msg ; } 
protected: 
	std::basic_string<char> msg ; 
}; 

//	void insert_c(Tc c_)
//	{
//		msg.push_back(c_) ;
//	} 
	/// \brief probemessage constructor 
	/// \brief this is for constructing the  message  from received data
	/// \brief parsing the data , judge the type and fill into fields
	/// \param T_ : input char buffer, s_: length, INPUT
	


template<class Tc, class Tcontainer>
class basic_probemessage : public basic_charmessage<Tc, Tcontainer >
{
public:
	basic_probemessage() : basic_charmessage<Tc, Tcontainer > () 
	{}
	basic_probemessage(const Tc *p_ , unsigned int length, bool b =false) : 
		basic_charmessage<Tc, Tcontainer > (p_ , length,b) 
	{}
	typedef Tc Tchar ;
	const Tcontainer & getmsg()  const
	{
		return basic_charmessage<Tc, Tcontainer >::
			getmsg () ;
	}
} ;



typedef basic_probemessage<unsigned char, std::vector<unsigned char>  >probemessage ;
typedef basic_probemessage<char, std::vector<char>  > probemessage_c ;

typedef basic_probemessage<unsigned char, std::basic_string<unsigned char>  >strmessage_uc ;
typedef basic_probemessage<char, std::basic_string<char>  > strmessage ;

template <class T_message, class Tparser_implementor>
class basic_messageparser 
{
public:
	basic_messageparser ()  {} 
	int parse2Message(typename T_message::Tchar *p , unsigned int size_ ,
		std::vector<T_message> &vmsg)
	{
		return static_cast<Tparser_implementor *>(this)->parse2Message_imp(
			p, size_, vmsg) ;
	}
} ;

/// \brief rawbinParser:  Do nothing, patch each package into one message
template <class Tc, class Tmessage> 
class basic_rawbinParser : public basic_messageparser <Tmessage,  basic_rawbinParser<Tc, Tmessage> > 
{
public: 
	basic_rawbinParser()  
	{ 
	} 
	/// \brief Parsing a serialized sequence of chars into sentences
	/// \brief 
	/// \param inbound_:  INPUT DATA, size_ : the length of read data 
	/// \param msgbuf: OUTPUT, contains the parsed messages when success
	/// \param returns > 0 of success
	int parse2Message_imp(const Tc* inbound_ ,unsigned int size_ , 
		std::vector<Tmessage> & msgbuf, bool b_quiet = false)
	{
		Tmessage pm(inbound_ , size_) ;
		msgbuf.push_back(pm) ;
		return (int)msgbuf.size() ;
	}

};

typedef basic_rawbinParser<unsigned char , probemessage> rawbinParser ;
typedef basic_rawbinParser<char , probemessage_c> rawbinParser_c ;
typedef basic_rawbinParser<char , strmessage> rawbinStrParser ;
typedef basic_rawbinParser<unsigned char , strmessage_uc> rawbinStrParser_uc ;

/// \brief textlineParser:  assume text messages, separate into vector of probemssages based on eol char.
template <class Tc, class Tmessage, Tc sep> 
class basic_textlineParser : public basic_messageparser <Tmessage,  basic_textlineParser<Tc, Tmessage, sep > > 
{
public: 
	basic_textlineParser()  
	{ 
	} 
	/// \brief Parsing a serialized sequence of chars into sentences
	/// \brief 
	/// \param inbound_:  INPUT DATA, size_ : the length of read data 
	/// \param msgbuf: OUTPUT, contains the parsed messages when success
	/// \param returns > 0 of success
	int parse2Message_imp(const Tc* inbound_ , unsigned int size_ , 
		std::vector<Tmessage> & msgbuf, bool b_quiet = false)
	{
		unsigned int index = 0 , ind_begin = 0 ;
		while(index < size_)
		{
			ind_begin = index ;
			while(index < size_ && inbound_[index] != sep)
			{
				index ++ ;
			}
			Tmessage pm(inbound_ + ind_begin,  index - ind_begin) ;
			index ++ ;
			msgbuf.push_back(pm) ;
		}
		return (int)msgbuf.size() ;
	}
};

typedef basic_textlineParser<unsigned char, probemessage, '\n'> textlineParser ;
typedef basic_textlineParser<char, probemessage_c, '\n'> textlineParser_c ;
typedef basic_textlineParser<char, strmessage, '\n'> textlineStrParser ;
typedef basic_textlineParser<unsigned char, strmessage_uc, '\n'> textlineStrParser_uc ;

///\brief HDLC message parser
class messageParser : public basic_messageparser <probemessage , messageParser>  
{
public:
	enum rcvStatus {
		RCV_IDLE, 
		RCV_CONFIRMING,
		RCV_CONFIRMED,
		RCV_PAYLOAD
	} ;
	enum charStatus{
		cs_IGNORE,
		cs_NOTIGNORE
	} ;
	
	messageParser()  
	{ 
		srindex = 0 ;
		rs_ = commonlibs::messageParser::RCV_IDLE ;
		cs_ = commonlibs::messageParser::cs_NOTIGNORE ;
	} 

	/// \brief Parsing a serialized sequence of chars into sentences
	/// \brief sentences begins and ends with 0x7E
	/// \param inbound_:  INPUT DATA, size_ : the length of read data 
	/// \param msgbuf: OUTPUT, contains the parsed messages when success
	/// \param returns > 0 of success
	int parse2Message_imp(unsigned char* inbound_ , int size_ , vector<commonlibs::probemessage> & msgbuf, bool b_quiet = false)
	{
		
		for(int i_ = 0 ; i_ < size_ ; ++i_)
		{
			unsigned char inbyte = inbound_[i_] ;
			int retcode = 0 ; // o stands for no sending , > 0 stands for sending out
			retcode = 0 ;
			if(srindex >= messageParser::BUFSIZE)
			{
					srindex = 0 ;
					rs_ = RCV_IDLE ;
					
					return 0 ;
			}
			// if encounters a magic byte , the 0x7E following it is not a control unsigned char
			if(inbyte == MAGICBYTE && cs_ == cs_NOTIGNORE)
			{
					srbuf[srindex ++] = inbyte ;
					cs_ = cs_IGNORE ; // ignore next unsigned char
					continue ;
			}		
			if(cs_ == cs_IGNORE)
			{
					srbuf[srindex ++] = inbyte ;
					if(inbyte != (unsigned char)commonlibs::MAGICBYTE)
					{
						cs_ = cs_NOTIGNORE ;
					}
					continue ;
			}
			switch(rs_)
			{
				case RCV_IDLE :// IDLE , waiting for 0x7E, 0x7E
		
					if(inbyte == (unsigned char)commonlibs::HEADTERM)
					{
						rs_ = RCV_CONFIRMING ; // CONFIRM begining; 
						srbuf[srindex ++] = inbyte ; // do not send this one
					}
					else 
					{
						rs_ = RCV_IDLE ; // WAITING MORE 
					}
					break ;
				case RCV_CONFIRMING: // COFIRMING: 
		
					if(inbyte == (unsigned char)commonlibs::HEADTERM) // confirmed 
					{
						if(srindex == 0)
							srbuf[srindex ++] = inbyte ;
						rs_ = RCV_CONFIRMED ;
					}
					else 
					{
						//error
						srindex = 0 ;
						rs_ = RCV_IDLE ; // IDLE
					}
					break ;
				case RCV_CONFIRMED: //waiting for begining:
		
					srbuf[srindex ++] = inbyte ;
					if(inbyte == (unsigned char)commonlibs::HEADTERM) // extra 0x7E , just waiting 
					{
						rs_ = RCV_CONFIRMED ;
					}
					else 
					{
						rs_ = RCV_PAYLOAD ; 
					}
					break ;
				case RCV_PAYLOAD: // RCVING:
		
					srbuf[srindex ++] = inbyte ;
					if (inbyte == (unsigned char)commonlibs::HEADTERM)  // end of normal packet
					{
						retcode  = srindex ;
						rs_ = RCV_CONFIRMING; // CONFIRMING ;		
						srindex = 0 ;
					} 
				default:
					break; 
			}
			if(retcode > 0)
			{
				probemessage p_((const unsigned char *)srbuf.elems , retcode , b_quiet) ;
				msgbuf.push_back(p_) ;
			}
		}
		return (int)msgbuf.size() ;
	}

private:
	int srindex ; 
	enum messageParser::rcvStatus rs_ ;
	enum messageParser::charStatus cs_ ; 
	enum {BUFSIZE = 8192} ;
	boost::array<unsigned char , messageParser::BUFSIZE> srbuf ;
}; 


template<typename MessageType>
class MessagePool
{
private:
	unsigned int pool_size ; 
public:
	const unsigned int size() const
	{
		return q_msg.size() ;
	}
	const unsigned int back_size() const 
	{
		return q_msgback.size() ;
	}
	void back_clear() 
	{
		try {
			boost::recursive_mutex::scoped_lock L_(m_mutex) ;
			if(!L_.owns_lock())
				return ; 
			while(!q_msgback.empty())
				q_msgback.pop() ;
		}
		catch(const boost::lock_error &)
		{
			std::cerr << "Conflict!" << endl ;
		}
		catch(const std::exception &e_)
		{
			std::cerr << e_.what() <<endl ;
		}
	}
	void clear() 
	{
		try {
			boost::recursive_mutex::scoped_lock L_(m_mutex) ;
			if(!L_.owns_lock())
				return ; 
			while(!q_msg.empty())
				q_msg.pop() ;
		}
		catch(const boost::lock_error &)
		{
			std::cerr << "Conflict!" << endl ;
		}
		catch(const std::exception &e_)
		{
			std::cerr << e_.what() <<endl ;
		}
	}
	explicit MessagePool(unsigned int size_=10) 
	{
		pool_size = size_ ;
	}
   // We only need to synchronize other for the same reason we don't have to
   // synchronize on construction!
	bool empty() const
	{
		try {
			boost::recursive_mutex::scoped_lock L_(m_mutex) ;
			//boost::get_system_time() + boost::posix_time::milliseconds(500)) ;
			if(!L_.owns_lock())
				return true; 
//			if(L_.locked())
				return q_msg.empty() ;
//			else 
//				return true ; // cannot lock the resource within one sec, return empty
		}
		catch(const boost::lock_error &)
		{
			std::cerr << "Conflict!" << endl ;
		}
		catch(const std::exception &e_)
		{
			std::cerr << e_.what() <<endl ;
		}
		return true;  // error occured, return empty
	}
	bool back_empty() const
	{
		try {
			boost::recursive_mutex::scoped_lock L_(m_mutexback) ;
				//boost::get_system_time() + boost::posix_time::milliseconds(500)) ;
			if(!L_.owns_lock())
				return true; 
			return q_msgback.empty() ;
		}
		catch(const boost::lock_error &)
		{
			std::cerr << "Conflict!" << endl ;
		}
		catch(const std::exception &e_)
		{
			std::cerr << e_.what() <<endl ;
		}
		return true;  // error occured, return empty
	}
	int addMessage(const MessageType &pm)
	{
		try {
			boost::recursive_mutex::scoped_lock L_(m_mutex) ;
			//	boost::get_system_time() + boost::posix_time::milliseconds(500)) ;
			if(!L_.owns_lock())
				return -1; 
			q_msg.push(pm) ;
			if(q_msg.size() > pool_size) // maximum
			{
				q_msg.pop() ;  // delete one
			}
			return 0 ; // 0.15 changed
		}
		catch(const boost::lock_error &)
		{}
		catch(const std::exception &e_)
		{
			std::cerr<< e_.what() <<endl ;
		}
		return -1;
	}
	int addBackMessage(const MessageType &pm)
	{
		try {
			boost::recursive_mutex::scoped_lock L_(m_mutexback) ;
				//boost::get_system_time() + boost::posix_time::milliseconds(500)) ;
			if(!L_.owns_lock())
				return -1; 

			q_msgback.push(pm) ;
			if(q_msgback.size() > pool_size) // maximum 10
			{
				q_msgback.pop() ;  // delete one
			}
			return 0 ; // 0.15 changed
		}
		catch(const boost::lock_error &)
		{}
		catch(const std::exception &e_)
		{
			std::cerr<< e_.what() <<endl ;
		}
		return -1;
	}
	int getBackwardMessage(MessageType &pm)
	{
		try {
			boost::recursive_mutex::scoped_lock L_(m_mutexback) ;
			//	boost::get_system_time() + boost::posix_time::milliseconds(500)) ;
			if(!L_.owns_lock())
				return -1; 
			if(!q_msgback.empty())
			{
				pm = q_msgback.front() ;
				q_msgback.pop() ;
				return 0 ;
			}
			else  
				return -1 ;
		}
		catch(const boost::lock_error &)
		{}
		catch(const std::exception &e_)
		{
			std::cerr<< e_.what() <<endl ;
		}
		return -1 ;
	}
	int getMessage(MessageType &pm)
	{

		try {
			boost::recursive_mutex::scoped_lock L_(m_mutex) ;
			//	boost::get_system_time() + boost::posix_time::milliseconds(500)) ;
			if(!L_.owns_lock())
				return -1; 
			if(!q_msg.empty())
			{
				pm = q_msg.front() ;
				q_msg.pop() ;
				return 0 ;
			}
			else  {
				return -1 ;
			}
		}
		catch(const boost::lock_error &)
		{}
		catch(const std::exception &e_)
		{
			std::cerr<< e_.what() <<endl ;
		}
		return -1 ;
	}
private:
   mutable boost::recursive_mutex  m_mutex;
   mutable boost::recursive_mutex  m_mutexback;
   std::queue<MessageType> q_msg; 
   std::queue<MessageType> q_msgback;
};

typedef std::vector<unsigned char>::const_iterator iterator_vc ;
typedef boost::spirit::scanner<iterator_vc>     scanner_vc;
typedef boost::spirit::rule<scanner_vc>         rule_vc;

typedef char const *			iterator_s ;
typedef boost::spirit::scanner<iterator_s>     scanner_s;
typedef boost::spirit::rule<scanner_s>         rule_s;


template < typename IteratorType>
class echo
{
public:
	echo(IteratorType &t_first) :current( t_first) 	{}
	void operator() (IteratorType first , const IteratorType &last) const
	{
		while(first!= last)
			 ++ first ;
		//current = first ;
		current = IteratorType(first) ;
	}
	IteratorType &current ;
} ;

	template<class Tparser, class Tpayload>
	class textline_message {
	public:
		textline_message () {}
		int parse_message(const probemessage &pm_) 
		{
			return static_cast<Tparser *>(this)->parse_message_imp(pm_) ;
		}
		const Tpayload &get_payload() 
		{
			return pl ;
		}
	protected: 
		
		Tpayload pl ;
	} ;


}
#endif 
