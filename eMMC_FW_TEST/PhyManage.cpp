/**********************************************************
Description	:		PhyManage.cpp
Author		:		Caden_liu
Data		:		2017.7.18
**********************************************************/
#include "StdAfx.h"
#include "InnerFlash/InnerFlash.h"
#include "PhyManage.h"
#include <Log/ILog.h>
#include "DMAC.h"

void FW_EarseFlash(U32 _blkaddr)
{
	CE_ID id = {0};
	OP_SEND_INFO FlashOpInfo = {0};
	OP_GET_INFO FlashRetInfo = {0};

	id.m_uChannelNo = 0;
	id.m_uChipNo = 0;
	
	FlashOpInfo.uSize = sizeof(FlashOpInfo);
	FlashOpInfo.uVirtualOpFlag = 0;
	FlashOpInfo.uUserDataLen = 8; //128;
	//FlashOpInfo.uRowAddr = (_blkaddr<<8); //need confirm
	FlashOpInfo.uRowAddr = _blkaddr; //need confirm
	
	while(False == CheckRBReady(id)){}
	SendCmd(id, OP_ERASE, &FlashOpInfo);	
	while(False == CheckRBReady(id)){}	
	ReadInfo(id, &FlashRetInfo);
}

void WriteUserData(OP_SEND_INFO &_FlashOpInfo,U32 _curLen,U32 logic_page_no,U32 userDataFlag)
{

	//在Userdata中存入起始位的lpa num
	U32 UserData[2];
	U8 t;
	t = sizeof(UserData);
	UserData[0] = userDataFlag * SEC_OF_PAGE;
	UserData[1] = logic_page_no;
	
	memcpy(_FlashOpInfo.arrUserData, UserData, sizeof(UserData));
}

void PhyWritePage(U8 *_pCache, U32 _addr, U32 _curLen, U32 logic_page_no,U8 IsTapData, U32 userFlag)
{
	CE_ID id = {0};
	OP_SEND_INFO FlashOpInfo = {0};
	OP_GET_INFO FlashRetInfo = {0};

	id.m_uChannelNo = 0;
	id.m_uChipNo = 0;
	
	FlashOpInfo.uSize = sizeof(FlashOpInfo);
	FlashOpInfo.uVirtualOpFlag = IsTapData;//0:logic Data;1:Tap Data
	FlashOpInfo.uRowAddr = _addr;
	FlashOpInfo.uUserDataLen = 8; 	  //128;

	WriteUserData(FlashOpInfo, _curLen, logic_page_no, userFlag);

	while(False == CheckRBReady(id)){}
	SendData(id, _pCache, 0, _curLen*SECTOR_SIZE);	
	while(False == CheckRBReady(id)){}
	SendCmd(id, OP_PROGRAM, &FlashOpInfo);	
	while(False == CheckRBReady(id)){}	
	ReadInfo(id, &FlashRetInfo);
	U32 SecError = FlashRetInfo.error;
	if (SecError != 0)
	{
		LOG_ERROR("Page: 0x%0x, Error: 0x%0x", _addr, FlashRetInfo.error);
	}
}

// return lpa/index
U32 ReadUserData(CE_ID _id, OP_GET_INFO &_FlashRetInfo, U32 _addr,   U32 logic_page_no, U32 userDataFlag)
{
	U32 UserData[2];
	memcpy(UserData, _FlashRetInfo.arrUserData, sizeof(UserData));
	
	if(UserData[0] != (userDataFlag * SEC_OF_PAGE))
	{			
		LOG_ERROR("UserData Error,write_in: 0x%0x,Read_out: 0x%0x", userDataFlag * SEC_OF_PAGE, UserData[0]);
		return INVALID_U32;
	} 	
	return UserData[1];
}

U32 FW_ReadFlash(U8 *_pCache, U32 _addr, U32 _curLen, U32 logic_page_no,U8 IsTapData, U32 userDataFlag)
{
	CE_ID id = {0};
	OP_SEND_INFO FlashOpInfo = {0};
	OP_GET_INFO FlashRetInfo = {0};
	U32 UserData = INVALID_U32;

	id.m_uChannelNo = 0;
	id.m_uChipNo = 0;
	
	FlashOpInfo.uSize = sizeof(FlashOpInfo);
	FlashOpInfo.uVirtualOpFlag = IsTapData;//0:logic Data;1:Tap Data
	FlashOpInfo.uRowAddr = _addr;
	FlashOpInfo.uUserDataLen = 8; //128;
	
    while(False == CheckRBReady(id)){}
	SendCmd(id, OP_READ, &FlashOpInfo);
	while(False == CheckRBReady(id)){}
	GetData(id, _pCache, 0, _curLen*SECTOR_SIZE);
	while(False == CheckRBReady(id)){}
	ReadInfo(id, &FlashRetInfo);
	U32 SecError = FlashRetInfo.error;
	if (SecError != 0)
	{
		LOG_ERROR("Page: 0x%0x, Error: 0x%0x", _addr, FlashRetInfo.error);
	}

	UserData = ReadUserData(id, FlashRetInfo, _addr, logic_page_no, userDataFlag);
	if (1 == FlashRetInfo.eECCStatus)
	{
		return INVALID_U32;
	}
	return UserData;
}



