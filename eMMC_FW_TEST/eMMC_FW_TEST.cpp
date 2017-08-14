/**********************************************************
Description	:		emmc W/R
Author		:		Caden_liu
Data		:		2017.7.18
**********************************************************/
// eMMC_FW_TEST.cpp : Defines the exported functions for the DLL application.
#include "stdafx.h"
#include "FirmwareProcParam.h"
#include "DataDef_eMMC.h"
#include "eMMC_FW_TEST.h"
#include "Ftl_GC.h"
#include "Ftl_sys.h"
#include "DMAC.h"
#include "List.h"
#include "Ftl_GC.h"


PGetBuffer g_pGetBuffer = NULL;
PSetBuffer g_pSetBuffer = NULL;
PGetRegister g_pGetRegister = NULL;
PSetRegister g_pSetRegister = NULL;
PCheckExitFW g_pCheckExit = NULL;

PGetSegmentBeginPos g_pGetSegmentBeginPos = NULL;
PGetSegmentEndPos   g_pGetSegmentEndPos = NULL;
PGetSegmentSize		g_pGetSegmentSize = NULL;
PWriteSegment		g_pWriteSegment = NULL;
PBoot2Flycode       g_pBoot2Flycode = NULL;

U8 BeReadTapData(U8 *buffer, U32 Index,sim_nftl_part_t* part,U8 IsReadMap);

// 接口初始化
void InitInterface(LPVOID _lpParam)
{
	assert(_lpParam);

	// 初始化线程参数
	PFIRMWARE_PARAMS pParam = (PFIRMWARE_PARAMS)_lpParam;
	if (pParam == NULL)
		return;

	g_pGetBuffer = pParam->pGetBuffer;
	g_pSetBuffer = pParam->pSetBuffer;
	g_pGetRegister = pParam->pGetRegister;
	g_pSetRegister = pParam->pSetRegister;
	g_pCheckExit = pParam->pCheckExitFW;
	g_pGetSegmentBeginPos = pParam->pGetSegmentBeginPos;
	g_pGetSegmentEndPos = pParam->pGetSegmentEndPos;
	g_pGetSegmentSize = pParam->pGetSegmentSize;
	g_pWriteSegment = pParam->pWriteSegment;
	g_pBoot2Flycode = pParam->pBoot2Flycode;

}


DWORD WINAPI PlugDevice(LPVOID lpParam)
{
	ftl_init();
	return True;
}

DWORD WINAPI LowVoltage(U32 _uParam)
{
	return 1;
}

// 启动FTL
DWORD WINAPI LaunchFTL(LPVOID lpParam)
{	
	// 接口初始化
	InitInterface(lpParam);
	g_pMemBaseAddr	= ((U8*)g_pGetSegmentBeginPos(0));
	SDC_Main();
	return 1;
}

//This Function will Scan Temp L2p Table
//If the lpa's map exist, the pPhyAddr will be set to the physical Address
void SearchTempL2p(U32 lpa, U32 *pPhyAddr,sim_nftl_part_t* part)
{
	U16 num = part->l2p_map_tap_index;
	while(num)
	{
		num--;
		if (lpa == part->l2p_map_tap[num].lpa)
		{
			*pPhyAddr = part->l2p_map_tap[num].ppa.addr32;
			break;
		}
	}
}

U32 MapBuf[MAP_PAGE_LPA_NUM];
U32 GetLpaMap(U32 lpa,sim_nftl_part_t* part)
{
	U32 map = 0;
	U32 mapIndex;
	U32 offset;

	//If do not check we will Over wirte the LPA in temp buffer with old data
	SearchTempL2p(lpa,&map,part);
	if(map)
	{
		return map;
	}

	mapIndex = lpa / MAP_PAGE_LPA_NUM;
	offset = lpa % MAP_PAGE_LPA_NUM;

	BeReadTapData((U8 *)MapBuf, mapIndex, part, READ_MAP);

	return MapBuf[offset];

}

void EMMCRead(sim_nftl_part_t* part)
{
	U32 uLogAddr;
	U32 logic_page;
	PFA_T pfa;
	U32 uSector_cnt = 0;
	//U16 block;
	//DOUBLE_LINK_NODE* pNode;
	
	uLogAddr = g_pGetRegister(LOG_ADDR);
	
	do
	{
		logic_page = uLogAddr >> 4;
		pfa.addr32 = GetLpaMap(logic_page,part);
		//LOG_INFO("LPA %d's New Addr is %d\n", logic_page, pfa.addr32);
		
		FW_ReadFlash(g_pMemBaseAddr, pfa.addr32,SEC_OF_PAGE, logic_page,LOGIC_DATA, DATA_LOGIC_NUM);

		uSector_cnt = DMAC_Dma1Start(BUFFER_TO_HOST, g_pMemBaseAddr, 0, SEC_OF_PAGE); //sector数
		ASSERT(uSector_cnt == SEC_OF_PAGE);
		uLogAddr += SEC_OF_PAGE;
	}while(g_pGetRegister(INT2_STS)!= INT2_READ_STOP);
	
	// 复位寄存器
	g_pSetRegister(RB_STS, RB_HIGH);
	g_pSetRegister(INT2_STS, INT2_DEFAULT);
}

void Emmc_Erase(U16 block, sim_nftl_part_t* part)
{
	PFA_T pfa;
	
	pfa.addr32 = 0;
	pfa.addrBit.block = block; //(block >> 1);
	//function Erase need check
	FW_EarseFlash(pfa.addr32);
	part->erase_cnt[block]++;
	part->vpc[block] = 0;
}

void Update_TmpTab(U32 lpa,U32 phy_addr,sim_nftl_part_t* part)
{
	U8 i;
	ASSERT(part->l2p_map_tap_index != PAGE_OF_BLOCK);

	if(part->l2p_map_tap_index != 0)
	{
		for(i = 0;i < part->l2p_map_tap_index; i++)
		{
			if(part->l2p_map_tap[part->l2p_map_tap_index].lpa == lpa)
			{
				part->l2p_map_tap[part->l2p_map_tap_index].ppa.addr32 = phy_addr;//newPfa.addr32;
				return ;
			}
		}
	}

	part->l2p_map_tap[part->l2p_map_tap_index].lpa = lpa;
	part->l2p_map_tap[part->l2p_map_tap_index].ppa.addr32 = phy_addr;	
	part->l2p_map_tap_index++;
}

void increaseVpc(U32 block,sim_nftl_part_t* part)
{
	if(block >= BLOCK_OF_LUN)
	{
		LOG_ERROR("inValid Block is to change VPC\n");
	}

	if(block < BLOCK_OF_LUN)
	{
		if(part->vpc[block] > PAGE_OF_BLOCK)  
		{
			LOG_ERROR("VPC of %d is already get to MAX\n", block);
		}
		else
		{
			part->vpc[block]++;
		}
	}
	
}

void decreaseVpc(U32 block,sim_nftl_part_t* part)
{
	if(block >= BLOCK_OF_LUN)
	{
		LOG_ERROR("inValid Block is to change VPC\n");
	}
	if(block < BLOCK_OF_LUN)
	{
		if(part->vpc[block] == 0) 
		{
			LOG_ERROR("VPC of %d is already zero\n", block);
		}
		else
		{
			part->vpc[block]--;
		}
	}
}


U8 BeReadTapData(U8 *buffer, U32 Index,sim_nftl_part_t* part,U8 IsReadMap)
{
	U32 addr = INVALID_U32;
	if(READ_MAP == IsReadMap)
	{
		addr = part->TapIndexAddr.mapIndex[Index].addr32;
		memset(buffer, 0xFF, CAP_OF_PAGE);
		if(INVALID_U32 == addr)
		{
			//LOG_INFO("Map Not Exist for index %d\n", Index);
			return 1;
		}
		FW_ReadFlash(buffer, addr, SEC_OF_PAGE, Index, TAP_DATA, MAP_LOGIC_NUM);
	}
	else  //read PS
	{
		addr = part->TapIndexAddr.PS_index[Index].addr32;
		memset(buffer, 0x0, CAP_OF_PAGE);
		if(INVALID_U32 == addr)
		{
			//LOG_INFO("PSAddr Not Exist for index %d\n", Index);
			return 1;
		}
		FW_ReadFlash(buffer, addr, SEC_OF_PAGE, Index, TAP_DATA, PS_LOGIC_NUM);
	}
	return 0;	
}

void BeFlushMap(U8 *buffer, U32 Index,sim_nftl_part_t* part, U8 IsFlushTap)
{
	U16 block;
	U32 addr1;
	PFA_T pfa;

	addr1 = part->TapIndexAddr.sys_Writer_Point.addr32;
	if (INVALID_U32 == addr1)//System block is full
	{
		block = pop_from_list_head(&part->sys_free_blk_head);
		ASSERT(INVALID_U16 != block);
		part->free_sys_block_num--;
	
		Emmc_Erase(block,part);

		pfa.addr32 = 0;
		pfa.addrBit.block = block;		
		part->TapIndexAddr.sys_Writer_Point.addr32 = pfa.addr32;
		addr1 = part->TapIndexAddr.sys_Writer_Point.addr32;
	}

	//write Map
	if(FLUSH_MAP == IsFlushTap)
	{
		PhyWritePage(buffer, addr1, SEC_OF_PAGE, Index,	TAP_DATA,MAP_LOGIC_NUM);
	}
	else
	{
		PhyWritePage(buffer, addr1, SEC_OF_PAGE, Index,	TAP_DATA,PS_LOGIC_NUM);
	}
	
	block = BLOCK_FROM_PFA(addr1);
	increaseVpc(block,part);

	if(FLUSH_MAP == IsFlushTap)
	{
		if(INVALID_U32 != part->TapIndexAddr.mapIndex[Index].addr32)
		{
			block = BLOCK_FROM_PFA(part->TapIndexAddr.mapIndex[Index].addr32);
			decreaseVpc(block,part);
		}
		part->TapIndexAddr.mapIndex[Index].addr32 = addr1; 
	}
	else
	{
		if(INVALID_U32 != part->TapIndexAddr.PS_index[Index].addr32)
		{
			block = BLOCK_FROM_PFA(part->TapIndexAddr.PS_index[Index].addr32);
			decreaseVpc(block,part);
		}
		part->TapIndexAddr.PS_index[Index].addr32 = addr1; 
	}


	part->TapIndexAddr.sys_Writer_Point.addr32 = GetNextPhyAddress(addr1);
	if(INVALID_U32 == part->TapIndexAddr.sys_Writer_Point.addr32)
	{
		block = BLOCK_FROM_PFA(addr1);
		insert_data_into_double_link(&part->sys_data_blk_head,block);
	}	
}

#if 0
void UpdateVpc(U32 lpa, U32 newAddr,sim_nftl_part_t* part)
{
	U32 mapIndex;
	U16 offset;
	U32 oldAddr;
	PFA_T pfa;
	U32 block;
	

	mapIndex = lpa / MAP_PAGE_LPA_NUM;
	offset =  lpa % MAP_PAGE_LPA_NUM;

	BeReadTapData((U8 *)MapBuf, mapIndex, part, READ_MAP);

	oldAddr = MapBuf[offset];
	if( INVALID_U32 !=oldAddr)
	{
		pfa.addr32 =oldAddr;
		block = BLOCK_FROM_PFA(pfa.addr32);
		decreaseVpc(block,part);
	}

	if( INVALID_U32 !=newAddr)
	{
		pfa.addr32 = newAddr;
		block = BLOCK_FROM_PFA(pfa.addr32);
		increaseVpc(block,part);
	}
	return;
}
#else

U32 GetPSTap(U16 block,sim_nftl_part_t* part, U8 * buf)
{
	U32 PSBlkOffset;
	U32 PSBase;

	U8 PSBlkIndex;
	//U8 pageIndex;
	//U8 pageOffset;
	U8 ReadPSBuf[CAP_OF_PAGE]={0};

	//first scan tempPS
	if(block == part->TempPS[0].block)
	{
		buf = part->TempPS[0].PSstatus;
		return 0;
	}
	
	if(block == part->TempPS[1].block)
	{
		buf = part->TempPS[1].PSstatus;
		return 0;
	}
	
	PSBlkIndex = block / PS_BLKCNT_IN_ONEPAGE;
	PSBlkOffset = block % PS_BLKCNT_IN_ONEPAGE;
	PSBase = PSBlkOffset * BLK_OCCUPY_BYTES_IN_PS;
				
	BeReadTapData((U8 *)ReadPSBuf, PSBlkIndex, part, READ_PS);
	memcpy(buf,&ReadPSBuf[PSBase],BLK_OCCUPY_BYTES_IN_PS);
	return 0;

}

U8 TempPSIndex = INVALID_U8;
U16 TempPSoffset = INVALID_U16;

void UpdateVpc(U32 lpa, U32 newAddr,sim_nftl_part_t* part)
{
	U32 mapIndex;
	
	U32 oldAddr;
	PFA_T pfa;
	U32 PSBase;
	U32 pageIndex;
	U16 block;
	U16 PSBlkOffset;
	U16 offset;

	U8 PSBlkIndex;
	
	U8 pageOffset;
	//U8 PSbit[32];
	U8 PS_Cache[CAP_OF_PAGE]={0};

	mapIndex = lpa / MAP_PAGE_LPA_NUM;
	offset =  lpa % MAP_PAGE_LPA_NUM;
	BeReadTapData((U8 *)MapBuf, mapIndex, part, READ_MAP);
	oldAddr = MapBuf[offset];
	
	if( INVALID_U32 !=oldAddr)
	{
		pfa.addr32 =oldAddr;
		block = BLOCK_FROM_PFA(pfa.addr32);
		decreaseVpc(block,part);	

		/***********update temp ps[0]*******************/	
		PSBlkIndex = block / PS_BLKCNT_IN_ONEPAGE;
		PSBlkOffset = block % PS_BLKCNT_IN_ONEPAGE;
		if((TempPSIndex != PSBlkIndex) || (TempPSoffset != PSBlkOffset))
		{   
			if((TempPSIndex != INVALID_U8) ||(TempPSoffset != INVALID_U16))
			{
				// flush Tempps[0]
				PSBlkIndex = part->TempPS[0].block / PS_BLKCNT_IN_ONEPAGE;
				PSBlkOffset = part->TempPS[0].block % PS_BLKCNT_IN_ONEPAGE;
				PSBase = PSBlkOffset * BLK_OCCUPY_BYTES_IN_PS;
				
				BeReadTapData((U8 *)PS_Cache, PSBlkIndex, part, READ_PS);//
				memcpy(&PS_Cache[PSBase],part->TempPS[0].PSstatus,BLK_OCCUPY_BYTES_IN_PS);
				BeFlushMap((U8 *)PS_Cache, PSBlkIndex, part, FLUSH_PS);
			}

			PSBlkIndex = block / PS_BLKCNT_IN_ONEPAGE;
			PSBlkOffset = block % PS_BLKCNT_IN_ONEPAGE;
			//memset(part->TempPS[0].PSstatus, 0, sizeof(part->TempPS[0].PSstatus));
			//BeReadTapData((U8 *)PS_Cache, PSBlkIndex, part, READ_PS);
			GetPSTap(block,part,part->TempPS[0].PSstatus);
			TempPSIndex = PSBlkIndex;
			TempPSoffset = PSBlkOffset;
		}

		part->TempPS[0].block = block;
		pageIndex = pfa.addrBit.page / PAGES_OF_BYTE;
		pageOffset = pfa.addrBit.page % PAGES_OF_BYTE;

		PSBlkOffset = block % PS_BLKCNT_IN_ONEPAGE;
		PSBase = PSBlkOffset * BLK_OCCUPY_BYTES_IN_PS;

		//memset(part->TempPS[0].PSstatus, 0, sizeof(part->TempPS[0].PSstatus));
		//memcpy(part->TempPS[0].PSstatus,&PS_Cache[PSBase],BLK_OCCUPY_BYTES_IN_PS);

		CLRBIT(part->TempPS[0].PSstatus[pageIndex], pageOffset);
		/***************end***************/
	}

	if( INVALID_U32 !=newAddr)
	{
		pfa.addr32 = newAddr;
		block = BLOCK_FROM_PFA(pfa.addr32);
		increaseVpc(block,part);

		/***********update temp ps[1]*******************/
		part->TempPS[1].block = block;
		pageIndex = pfa.addrBit.page / PAGES_OF_BYTE;
		pageOffset = pfa.addrBit.page % PAGES_OF_BYTE;

		//memset(part->TempPS[1].PSstatus, 0, sizeof(part->TempPS[1].PSstatus));
		SETBIT(part->TempPS[1].PSstatus[pageIndex],pageOffset);
		/***************end**********************/
	}
	//return;
}
#endif

#if 0
U8 PSBlk_Zone = INVALID_U8;
U8 PSCache[CAP_OF_PAGE]={0};
U8 newPSTap[CAP_OF_PAGE]={0};
void Flush_Templ2p(sim_nftl_part_t* part)   //flush P2L and PS
{
	U32 mapIndex;
	U32 mapOffset, PSBlkOffset; //
	U32 newAddr;
	U16 pos = 0;
	U16 count;
	U32 MapCache[MAP_PAGE_LPA_NUM];

	U32 PSBase;
	PFA_T pfa_old;
	U16 block;

	U16 old_page_num;
	U16 new_page_num;
	U16 i;
	
	U8 PSBlkIndex;
	U8 pageIndex;
	U8 pageOffset;
	U8 Flush_PSBlk_flag = 0;
	
	count = part->l2p_map_tap_index;
	while (pos < count)
	{
		///We found the first valid LPA
		for (i = pos; i<count; i++)
		{
			if(INVALID_U32 == part->l2p_map_tap[i].lpa)
			{
				continue;
			}
			else
			{
				break;
			}
		}

		if(i == count)
		{
			break;///All LPA Map Updated
		}

		pos = i;
		////Read Map Table to be updated
		mapIndex = part->l2p_map_tap[i].lpa / MAP_PAGE_LPA_NUM;
		BeReadTapData((U8 *)MapCache, mapIndex, part, READ_MAP);

		for(i = pos; i<count; i++)
		{
		    if(INVALID_U32 == part->l2p_map_tap[i].lpa)
			{
				continue;
			}

			if (i != pos)
			{
				if ((part->l2p_map_tap[i].lpa / MAP_PAGE_LPA_NUM) != mapIndex )
				{
					continue;//LPA Not in the updating Zone, go for next
				}
			}

			mapOffset = part->l2p_map_tap[i].lpa % MAP_PAGE_LPA_NUM;
			newAddr = part->l2p_map_tap[i].ppa.addr32;

			/******************update PS*************************/
			// 1.old_addr
			pfa_old.addr32 = MapCache[mapOffset];
			if(INVALID_U32 != pfa_old.addr32)
			{
				block = pfa_old.addrBit.block;
				old_page_num = pfa_old.addrBit.page;
				PSBlkIndex = block / PS_BLKCNT_IN_ONEPAGE;
				if(PSBlk_Zone != PSBlkIndex)
				{
				/*
					if(INVALID_U8 != PSBlk_Zone)
					{
						//flush  old PS
						Flush_PSBlk_Index = PSBlk_Zone;
						BeFlushMap((U8 *)PSCache, PSBlk_Zone, part, FLUSH_PS); //update old ps
					}
				*/
					BeReadTapData((U8 *)PSCache, PSBlkIndex, part, READ_PS);
					PSBlk_Zone = PSBlkIndex;
				}
				
				Flush_PSBlk_flag = 1;
				PSBlkOffset = block % PS_BLKCNT_IN_ONEPAGE;
				PSBase = PSBlkOffset * BLK_OCCUPY_BYTES_IN_PS;

				pageIndex = old_page_num / PAGES_OF_BYTE;
				pageOffset = old_page_num % PAGES_OF_BYTE;
				
				if(0 != PSCache[PSBase + pageIndex])
				{
					CLRBIT(PSCache[PSBase + pageIndex], pageOffset);	
				}
			}

			// 2.new_addr
			block = part->l2p_map_tap[i].ppa.addrBit.block;
			new_page_num = part->l2p_map_tap[i].ppa.addrBit.page;

			PSBlkIndex = block / PS_BLKCNT_IN_ONEPAGE;
			PSBlkOffset = block % PS_BLKCNT_IN_ONEPAGE;
			PSBase = PSBlkOffset * BLK_OCCUPY_BYTES_IN_PS;
			pageIndex = new_page_num / PAGES_OF_BYTE;
			pageOffset = new_page_num % PAGES_OF_BYTE;
			
			SETBIT(newPSTap[PSBase + pageIndex],pageOffset);
			
			/************************PS end****************************/
			
			MapCache[mapOffset] = newAddr;
	
			part->l2p_map_tap[i].lpa = INVALID_U32;	//Mark as updated

		}
		pos++;
		BeFlushMap((U8 *)MapCache, mapIndex, part, FLUSH_MAP);
		BeFlushMap((U8 *)newPSTap, PSBlkIndex, part, FLUSH_PS);   //flush new ps
		if(1 == Flush_PSBlk_flag)
		{
			BeFlushMap((U8 *)PSCache, PSBlk_Zone, part,FLUSH_PS);
			Flush_PSBlk_flag = 0;
		}
	}
}
#else
void Flush_Templ2p(sim_nftl_part_t* part)   //flush P2L and PS
{
	U32 mapIndex;
	U32 mapOffset; //
	U32 newAddr;
	U16 pos = 0;
	U16 count;
	U32 MapCache[MAP_PAGE_LPA_NUM];

	U16 i;
	
	count = part->l2p_map_tap_index;
	while (pos < count)
	{
		///We found the first valid LPA
		for (i = pos; i<count; i++)
		{
			if(INVALID_U32 == part->l2p_map_tap[i].lpa)
			{
				continue;
			}
			else
			{
				break;
			}
		}

		if(i == count)
		{
			break;///All LPA Map Updated
		}

		pos = i;
		////Read Map Table to be updated
		mapIndex = part->l2p_map_tap[i].lpa / MAP_PAGE_LPA_NUM;
		BeReadTapData((U8 *)MapCache, mapIndex, part, READ_MAP);

		for(i = pos; i<count; i++)
		{
		    if(INVALID_U32 == part->l2p_map_tap[i].lpa)
			{
				continue;
			}

			if (i != pos)
			{
				if ((part->l2p_map_tap[i].lpa / MAP_PAGE_LPA_NUM) != mapIndex )
				{
					continue;//LPA Not in the updating Zone, go for next
				}
			}

			mapOffset = part->l2p_map_tap[i].lpa % MAP_PAGE_LPA_NUM;
			newAddr = part->l2p_map_tap[i].ppa.addr32;
			
			MapCache[mapOffset] = newAddr;
	
			part->l2p_map_tap[i].lpa = INVALID_U32;	//Mark as updated

		}
		pos++;
		BeFlushMap((U8 *)MapCache, mapIndex, part, FLUSH_MAP);
	}
}
#endif

void Flush_PS(sim_nftl_part_t* part)
{
	U32 PSBase;
	U16 PSBlkOffset;
	U8 PSBlkIndex;
	
	//U8 pageIndex;
	//U8 pageOffset;
	
	U8 buf[CAP_OF_PAGE]={0};

	// flush Tempps[0]
	if(part->TempPS[0].block != INVALID_U16)
	{
		PSBlkIndex = part->TempPS[0].block / PS_BLKCNT_IN_ONEPAGE;
		PSBlkOffset = part->TempPS[0].block % PS_BLKCNT_IN_ONEPAGE;
		PSBase = PSBlkOffset * BLK_OCCUPY_BYTES_IN_PS;
		
		BeReadTapData((U8 *)buf, PSBlkIndex, part, READ_PS);
		memcpy(&buf[PSBase],part->TempPS[0].PSstatus,BLK_OCCUPY_BYTES_IN_PS);
		BeFlushMap((U8 *)buf, PSBlkIndex, part, FLUSH_PS);
	}


	//Flush Tempps[1]
	memset(buf, 0, sizeof(buf));
	
	PSBlkIndex = part->TempPS[1].block / PS_BLKCNT_IN_ONEPAGE;
	PSBlkOffset = part->TempPS[1].block % PS_BLKCNT_IN_ONEPAGE;
	PSBase = PSBlkOffset * BLK_OCCUPY_BYTES_IN_PS;
	
	BeReadTapData((U8 *)buf, PSBlkIndex, part, READ_PS);
	memcpy(&buf[PSBase],part->TempPS[1].PSstatus,BLK_OCCUPY_BYTES_IN_PS);
	BeFlushMap((U8 *)buf, PSBlkIndex, part, FLUSH_PS);

	//clear
	part->TempPS[0].block =INVALID_U16;
	part->TempPS[1].block =INVALID_U16;
	memset(part->TempPS[0].PSstatus, 0, sizeof(part->TempPS[0].PSstatus));
	memset(part->TempPS[1].PSstatus, 0, sizeof(part->TempPS[1].PSstatus));
	TempPSIndex = INVALID_U8;
	TempPSoffset = INVALID_U16;
}


void FlushSysData(U8 *buffer,U32 logic_page_no,sim_nftl_part_t* part,U32 userFlag)
{
	U32 addr1;
	PFA_T pfa;
	U16 block;

	addr1 = part->TapIndexAddr.sys_Writer_Point.addr32;
	if (INVALID_U32 == addr1)//System block is full
	{
		block = pop_from_list_head(&part->sys_free_blk_head);
		ASSERT(INVALID_U16 != block);
		part->free_sys_block_num--;
		
		Emmc_Erase(block,part);

		pfa.addr32 = 0;
		pfa.addrBit.block = block;		
		part->TapIndexAddr.sys_Writer_Point.addr32 = pfa.addr32;
		addr1 = part->TapIndexAddr.sys_Writer_Point.addr32;
	}

	PhyWritePage(buffer, addr1, SEC_OF_PAGE, logic_page_no,TAP_DATA, userFlag);
	
	part->TapIndexAddr.sys_Writer_Point.addr32 = GetNextPhyAddress(addr1);
	if(INVALID_U32 == part->TapIndexAddr.sys_Writer_Point.addr32)
	{
		block = BLOCK_FROM_PFA(addr1);
		insert_data_into_double_link(&part->sys_data_blk_head,block);
	}
}

void FlushIndexAddr(U8 *buffer,U32 logic_page_no,sim_nftl_part_t* part,U32 userFlag)
{
	U32 addr1;
	PFA_T pfa;
	U16 block;

	addr1 = part->TapIndexAddr.TapIndex_Writer_Point.addr32;

	PhyWritePage(buffer, addr1, SEC_OF_PAGE, logic_page_no,TAP_DATA, userFlag);
	
	part->TapIndexAddr.TapIndex_Writer_Point.addr32 = GetNextPhyAddress(addr1);
	if(INVALID_U32 == part->TapIndexAddr.TapIndex_Writer_Point.addr32)
	{
		FW_ReadFlash(buffer, addr1,SEC_OF_PAGE, logic_page_no,TAP_DATA, userFlag);
		block = BLOCK_FROM_PFA(addr1);
		Emmc_Erase(block,part);

		pfa.addr32 = 0;
		pfa.addrBit.block = block ;		
		part->TapIndexAddr.TapIndex_Writer_Point.addr32 = pfa.addr32;
		PhyWritePage(buffer, part->TapIndexAddr.TapIndex_Writer_Point.addr32, SEC_OF_PAGE, logic_page_no,TAP_DATA, userFlag);
		part->TapIndexAddr.TapIndex_Writer_Point.addr32 = GetNextPhyAddress(part->TapIndexAddr.TapIndex_Writer_Point.addr32);
	}
}

//flush Ckpt no need update vpc
U16 write_Cache[MAP_PAGE_LPA_NUM*2];
void FlushCkpt(sim_nftl_part_t* part)
{	
	//flush pec
	memset(write_Cache, 0xFF, sizeof(write_Cache));
	memcpy(write_Cache,part->erase_cnt,sizeof(U16)*BLOCK_OF_LUN);
	FlushSysData((U8 *)write_Cache, ERASE_LOGIC_NUM,part,ERASE_LOGIC_NUM);

	//flush VPC
	memset(write_Cache, 0xFF, sizeof(write_Cache));
	memcpy(write_Cache,part->vpc,sizeof(part->vpc));
	FlushSysData((U8 *)write_Cache, VPC_LOGIC_NUM,part,VPC_LOGIC_NUM);

	//flush mapIndex & PS_index & writePoint 
	memset(write_Cache, 0xFF, sizeof(write_Cache));
	memcpy(write_Cache,&part->TapIndexAddr,sizeof(part->TapIndexAddr));
	FlushIndexAddr((U8 *)write_Cache,MAPINDEX_LOGIC_NUM,part,MAPINDEX_LOGIC_NUM);
}

void GetMlcWRPoint(sim_nftl_part_t* part)
{
	U16 block;
	PFA_T pfa;
	if(INVALID_U32 == part->TapIndexAddr.mlc_Writer_Point.addr32)
	{
		block = pop_from_list_head(&part->mlc_free_blk_head);
		ASSERT(INVALID_U16 != block);
		part->free_data_block_num--;
		
		Emmc_Erase(block,part);
		
		pfa.addr32 = 0;
		pfa.addrBit.block = block ;		
		part->TapIndexAddr.mlc_Writer_Point.addr32 = pfa.addr32;
	}
}

void EMMCWrite(sim_nftl_part_t* part)
{
	U32 uLogAddr;
	U32 uSector_cnt = 0;
	PFA_T pfa;
	U16 block;
	U32 logic_page;
	U32 size,size1;

	size = sizeof(TAP_INDEX_T);
	size1 = sizeof(part->TapIndexAddr);

	uLogAddr = g_pGetRegister(LOG_ADDR);
	do 
	{
		//receive host data
		uSector_cnt = DMAC_Dma1Start(HOST_TO_BUFFER, g_pMemBaseAddr, 0, SEC_OF_PAGE); //sector数
		ASSERT(uSector_cnt == SEC_OF_PAGE);

		GetMlcWRPoint(part);
		pfa.addr32 = part->TapIndexAddr.mlc_Writer_Point.addr32;

		//update mapping
		logic_page = uLogAddr >> 4; 	//uLogAddr/SEC_OF_PAGE
		Update_TmpTab(logic_page, pfa.addr32,part);

		//写函入NAND
		PhyWritePage(g_pMemBaseAddr, pfa.addr32, SEC_OF_PAGE, logic_page, LOGIC_DATA, DATA_LOGIC_NUM);

		//更新vpc
		UpdateVpc(logic_page,pfa.addr32,part);

		uLogAddr += SEC_OF_PAGE;
		//catch the next page
		part->TapIndexAddr.mlc_Writer_Point.addr32 = GetNextPhyAddress(part->TapIndexAddr.mlc_Writer_Point.addr32);
		if (INVALID_U32 == part->TapIndexAddr.mlc_Writer_Point.addr32)////Get to Block End
		{
			block = BLOCK_FROM_PFA(pfa.addr32);
			insert_data_into_double_link(&part->mlc_data_blk_head,block);

			Flush_Templ2p(part);
			part->l2p_map_tap_index = 0;
			Flush_PS(part);
			FlushCkpt(part);
			sysBlockRecycling(part);
		}			

	} while (g_pGetRegister(INT2_STS)!= INT2_WRITE_STOP);
	
	g_pSetRegister(RB_STS, RB_HIGH);
	g_pSetRegister(INT2_STS, INT2_DEFAULT);	
}


void SDC_Main(void)
{	
	static U16 WR_cnt = 0;
	while(1)
	{
		INT2_VAL INT2Value;
		
		INT2Value = (INT2_VAL)g_pGetRegister(INT2_STS);

		// 检测是否退出
		if (g_pCheckExit())
		{
			return;
		}

 		switch(INT2Value)
		{
		case INT2_READ:
      		EMMCRead(sim_nftl_part);
			break;
		case INT2_WRITE:
			//LOG_INFO("write cnt %d\n", ++WR_cnt);
			EMMCWrite(sim_nftl_part);
			DataBlockRecycling(sim_nftl_part);
			break;
		case INT2_READ_STOP:
			break;
		case INT2_WRITE_STOP:
			break;
		case INT2_ERASE:
			break;
		default:
			break;
		}

	}
}

