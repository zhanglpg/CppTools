///\brief TSA_MSG.hpp
///\brief For parsing the messages (HDLC frame based) based message frames 
///	Revision History
///	2008-05-28 
///	Original Coding	
///	2009-03-19 migrate into commonlibs
///		Liping Zhang
#ifndef __HDLC_MSG_HPP
#define __HDLC_MSG_HPP
#include <string>
#include <boost/lexical_cast.hpp>
#include <boost/array.hpp>
#include <cstdlib>
#include "commonlibs/probemessage.hpp"

extern unsigned short _fcstab[256] ; 

namespace commonlibs {

typedef unsigned char UBYTE ;

typedef struct _struct_imple
{
	int iAddr ;
} struct_imple ;



#define FRAME_FLAG1					0x7e	// Frame beginning and ending flag
#define FRAME_FLAG2					0x7d	// Frame beginning and ending flag mask
#define MASK_FRAME_7E				0x5e	// Mask to eliminate flag 0x7E
#define MASK_FRAME_7D				0x5d	// Mask to eliminate flag 0x7D
#define BROADCAST_ADDRESS			0xff	// All-station address value
#define OFFSET_ADDRESS				0		// UBYTE offset of Address in string
#define OFFSET_CONTROL				1		// UBYTE offset of Control Field in string
#define OFFSET_IPI					2		// UBYTE offset of IPI in string
#define OFFSET_MSGTYPE				3		// UBYTE offset of Message Type in string
#define OFFSET_INFO					4		// UBYTE offset of Info Field in string
#define CNTRL_FIELD_UIS				0x13	// Control field for UI - pole set
#define CNTRL_FIELD_UPS				0x33	// Control field for UP - pole set
#define CNTRL_FIELD_UINS			0x03	// Control field for UI - pole not set
#define NTCIP_CLASS_B				0xc0	// Class B communication for IPI
#define ERROR_CODE					-1


#define MSGTYPE_IMPL				0xFF



template<typename implement_type, typename payload_type>
class CMsg 
{
protected:
	//////////////////////////////////////////////////////////////////////////
	// construct & destruct
	//
	CMsg() {} 
public:

	static int GetTypeFromMessage(
			const commonlibs::probemessage & pm_,
			unsigned char  & b_type) 
	{
		int retcode = -1 ;
		unsigned char uc_temp[1024] ;
		if(-1 != CMsg::getMessageTypeByte(pm_ , b_type , uc_temp , 1024))
		{
			retcode = 0 ;
		}
		
		return retcode;
	}

	static void SetHDLCFrame(unsigned char* puzFrame, int nFrameLength, payload_type &pl_ )
	{
		memcpy(reinterpret_cast<unsigned char *>(&pl_) ,puzFrame + OFFSET_INFO,  sizeof(pl_)) ;
		return ;
	}

	static int getPayloadFromPacket(const commonlibs::probemessage &pm_ , payload_type &payload_) 
	{
		unsigned char uc_temp_ [2048] ; 
		int rawlen ; 
		int retcode = -1; 
		if(-1 != (rawlen = 
			CMsg::getRawFrame(pm_ , uc_temp_ , 2048)))
		{
			if(CMsg::Cast(uc_temp_ , (unsigned int *)&rawlen))
			{
				implement_type::SetHDLCFrame(uc_temp_ , rawlen, payload_) ;
				retcode = 0 ;
			}
		}
		return retcode ;
	}


	///  return the length of legal uc_temp_ message on success
	///  otherwise, return -1 
	static int getMessageTypeByte(const commonlibs::probemessage &pm_ ,
		unsigned char &b_type , unsigned char *uc_temp_ ,  unsigned int size_limit_)
	{
	
		int rawlen ; 
		int retcode = -1; 
		if(-1 != (rawlen = 
			CMsg::getRawFrame(pm_ , uc_temp_ , size_limit_)))
		{
			if(CMsg::Cast(uc_temp_ , (unsigned int*)&rawlen))
			{
				b_type = uc_temp_[OFFSET_MSGTYPE] ;
				retcode =rawlen ;
			}
		}
		return retcode ;
	}

	/////////////////////////////////////////////////////////////////////////////
	// Function name:	GetHDLCFrame
	// Description	:	Get the raw frame bytes for serial transmission.
	//					Note that the actual framing ocurrs here, with the help
	//					of some base class functions.
	// Return type	:	const unsigned -- the raw frame bytes.
	// Argument	OUT	:	Length of the generated frame, -1 when failed
	/////////////////////////////////////////////////////////////////////////////
	static int GetHDLCFrame(const payload_type &pl_, unsigned char *puzFrame)
	{
		
		//unsigned char puzFrame [CMsg::MSG_BUF_SIZE];
		unsigned int nFrameLength = 0 ;
		// address "0" for master always, we are talking to the master for
		// this function.  Don't address the local directly.
	//	*(puzFrame + OFFSET_ADDRESS)	= 0x01;
		*(puzFrame + OFFSET_ADDRESS)	= GetModifiedAddress( 
			reinterpret_cast<const struct_imple *> (&pl_)->iAddr) ;
		*(puzFrame + OFFSET_CONTROL)	= CNTRL_FIELD_UPS;
		*(puzFrame + OFFSET_IPI)		= NTCIP_CLASS_B;
		*(puzFrame + OFFSET_MSGTYPE)	= implement_type::GetMsgType()  ;
		memcpy(puzFrame + OFFSET_INFO , &pl_ , sizeof(pl_)) ;
		nFrameLength = 4 + sizeof(pl_) ;
		if(nFrameLength >= MSG_BUF_SIZE)
			return -1 ;
		StuffFrameFCS(puzFrame, &nFrameLength);
		return nFrameLength;
	}


	///	\brief generate the output Probemessage from the payload
	///	\para: out_pm_: OUTPUT, 
	///	\paraL payload_ : INPUT, the payload data, 
	static int generatePacketFromPayload(commonlibs::probemessage &out_pm_ , const payload_type& payload_)
	{
		int retcode = -1 ;
		//SetMasterID((short)uMasterID) ;
		//SetLocalAddress((short)uLocalAddress) ;
		unsigned char puzFrame[CMsg::MSG_BUF_SIZE] ;
		int size_ ;
		if(0 < (size_ =
			implement_type::GetHDLCFrame(payload_ , puzFrame)))
		{
			if(size_ < CMsg::MSG_BUF_SIZE) 
			{
				out_pm_ = commonlibs::probemessage(
					//commonlibs::probemessage::msg_SC, 
					puzFrame,size_ ) ;
		 		retcode = 0 ;
			}
		}
		return retcode ;
	}

	static bool Cast(unsigned char * puzFrame, unsigned int * pnFrameLength)
	{

		CMsg::UnStuffFrame(puzFrame, pnFrameLength);
		if (!CMsg::VerifyFCS(puzFrame, *pnFrameLength))
		{
			return false;
		}
		return true;
	}

	///	\brief getRawFrame 
	///	\param: pm_: INPUT, the HDLC message 
	///			pdest_ : OUTPUT pointer to unsigned char
	///			sizelimit: INPUT, 
	///	\brief PRECONDITION:
	///			the raw message contains valid data
	///			POST CONDITION:
	///				the pdest_ points to the stuffed frame (from the first character after the 0x7E to the one before 
	///				terminating 0x7E)
	static int getRawFrame(const commonlibs::probemessage& pm_ , unsigned char *pdest_ , unsigned int sizelimit) 
	{
		if(pm_.getmsg().size() >= sizelimit)
			return -1 ;
		const vector<unsigned char> &v_m_ = 
			pm_.getmsg() ;
		for(unsigned int j_ = 1 ;  j_ < v_m_.size() - 1 ; ++j_)
		{
			pdest_[j_-1] = v_m_[j_] ;
		}
		return (int)v_m_.size() -2 ;
	}

	static int GetMsgType() 
	{
		return implement_type::GetMsgType() ;	
	}

	bool GetInited() 
	{
		return b_initialized ; 
	}
	//////////////////////////////////////////////////////////////////////////
	// HDLCE framing helper functions
	//
protected:
	bool b_initialized ; 
	int iAddr ;
	// Description	:	Performs UBYTE stuffing, adds 2-UBYTE FCS, adds HDLC flags.
	//					This code is lifted from Mike Robinson's HDLC C code.
	// Return type	:	void 
	// Argument	I/O	:	unsigned char *puzFrame -- buffer of raw frame UBYTEs.
	// Argument	I/O	:	int *pnFrameLength -- length of buffer.
	//
	static void StuffFrameFCS(unsigned char *puzFrame, unsigned  int *pnFrameLength, 
		unsigned int limit = 256)
	{
		unsigned char modframe[CMsg::MAX_CRC_UBYTES];
		unsigned int loopcount, count = 0, temp;
		unsigned short oldfcs = CMsg::PPPINITFCS, newfcs;
		temp = *pnFrameLength;
		if(temp > limit) {
			*pnFrameLength = 0 ; 
			return ;
		}
		newfcs = CMsg::pppfcs(oldfcs, puzFrame, *pnFrameLength);	// get the FCS
		*(puzFrame + temp++) = (char) (~newfcs);			// get ms UBYTE of FCS
		*(puzFrame + temp++) = (char) (~(newfcs >> 8));		// get ls UBYTE of FCS
		*pnFrameLength = temp;
		if (CMsg::VerifyFCS(puzFrame, *pnFrameLength))			// check for valid FCS
		{
			loopcount = 0;
			CMsg::CopyFrame(modframe, puzFrame, *pnFrameLength);	// copy frame into modframe
			modframe[count++] = FRAME_FLAG1;
			do
			{
				switch ((char) *(puzFrame+loopcount))
				{
				case FRAME_FLAG1:							// check for 0x7e
					modframe[count++] = FRAME_FLAG2;
					modframe[count++] = MASK_FRAME_7E;
					loopcount++;
					break;
				case FRAME_FLAG2:							// check for 0x7d
					modframe[count++] = FRAME_FLAG2;
					modframe[count++] = MASK_FRAME_7D;
					loopcount++;
					break;
				default:									// else copy data
					modframe[count++] = *(puzFrame + loopcount++);
					break;
				}
			} while (loopcount < *pnFrameLength);
			modframe[count++] = FRAME_FLAG1;				// copy ending flag
			*pnFrameLength = count;
			CMsg::CopyFrame(puzFrame, modframe, *pnFrameLength);	// copy modframe into frame
		   *(puzFrame + count + 1) = '\0';
		}
		else
		{
			*pnFrameLength = 0 ;					// error in FCS
		}
		return;
	}


	// Description	:	Removes UBYTE stuffing from the frame.
	//					This code is lifted from Mike Robinson's HDLC C code.
	// Return type	:	void 
	// Argument	I/O	:	unsigned char* puzFrame -- buffer of raw frame UBYTEs
	// Argument	I/O	:	int* pnLength -- length of buffer.
	//
	static void UnStuffFrame(unsigned char* puzFrame, unsigned  int* pnLength)
	{
		unsigned int  count, strcount = 0;
		for(count = 0; count < *pnLength; count++, strcount++)
		{
			if((*(puzFrame + count) == FRAME_FLAG2) &&
				(*(puzFrame + count + 1) == MASK_FRAME_7E))
			{
				*(puzFrame + strcount) = FRAME_FLAG1;
				count++;
			}
			else if((*(puzFrame + count) == FRAME_FLAG2) &&
					  (*(puzFrame + count + 1) == MASK_FRAME_7D))
			{
				*(puzFrame + strcount) = FRAME_FLAG2;
				count++;
			}
			else if (strcount < count)
				*(puzFrame + strcount) = *(puzFrame + count);
		}
		*pnLength = strcount;
	}

	

	// Description	:	Copy a frame of raw UBYTEs based on given length.  It is
	//					assumed that the destination buffer is equal to or
	//					greater than the source buffer, and that the buffer's
	//					memory has been allocated by the caller.
	//					This code is lifted from Mike Robinson's HDLC C code.
	// Return type	:	void
	// Argument	OUT	:	unsigned char* puzDest -- destination buffer.
	// Argument	IN	:	unsigned char* puzSrc -- source buffer
	// Argument	IN	:	int nLength -- length of buffers
	//
	static void CopyFrame(unsigned char* puzDest, unsigned char* puzSrc, int nLength)
	{
		for (int i = 0; i < nLength; i++)
		{
			*(puzDest + i) =  *(puzSrc + i);
		}
	}

	// Description	:	Check a frame for a valid FCS.
	//					This code is lifted from Mike Robinson's HDLC C code.
	// Return type	:	BOOL -- true if FCS is valid.  Otherwise false.
	// Argument	IN	:	unsigned char *puzFrame -- buffer of raw frame UBYTEs.
	// Argument	IN	:	int nFrameLength -- length of buffer.
	//
	static bool VerifyFCS(unsigned char *puzFrame, int nFrameLength)
	{
		// pppfcs is in fcs.h header file
		unsigned short oldfcs = PPPINITFCS, checkfcs;
		checkfcs = CMsg::pppfcs(oldfcs, puzFrame, nFrameLength);	// get the FCS
		if (checkfcs == CMsg::PPPGOODFCS)
		{
			return true;
		}
		else
		{
			return false;
		}
	}

	// Description	:	Tweak the address according to HDLC spec for use in frame.
	//					This code is lifted from Mike Robinson's HDLC C code.
	// Return type	:	unsigned char -- the modified address is returned.
	// Argument	IN	:	int nPlainAddress -- the plain address is passed in.
	//
	static unsigned char GetModifiedAddress(int nPlainAddress)
	{
		if (nPlainAddress == 63)	// broadcast
		{
			return BROADCAST_ADDRESS;
		}
		else	// individual address
		{
			return (unsigned char) (nPlainAddress << 2) | 0x01;
		}
	}
	enum {
		MAX_CRC_UBYTES = 1024, 
		MSG_BUF_SIZE = 1024 ,
		PPPINITFCS = 0xffff ,
		PPPGOODFCS =0xf0b8 
	} ;

	static unsigned short pppfcs(unsigned short fcs, unsigned char *cp, int len) 
	{
		while (len--)
			fcs = (fcs >> 8) ^ _fcstab[(fcs ^ *cp++) & 0xff];
		return (fcs);
	}
};
	
class CMsg_imple : public CMsg<CMsg_imple , struct_imple> 
{
private:
	CMsg_imple () {} 
public:
	static int GetMsgType()
	{
		return (int)MSGTYPE_IMPL ;
	}
	static int GetHDLCFrame(const struct_imple &, unsigned char *)
	{
		return -1 ;
	}
	static void SetHDLCFrame(unsigned char* , int , struct_imple&)
	{
		return ;
	}
} ;

typedef CMsg<CMsg_imple, struct_imple>  CMsg_unknown ;

}
#endif
