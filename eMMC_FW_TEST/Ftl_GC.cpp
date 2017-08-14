/**********************************************************
Description	:		NAND flash GC
Author		:		Caden_liu
Data		:		2017.7.18
**********************************************************/
#include "stdafx.h"
#include <Log/ILog.h>
#include "Ftl_GC.h"
#include"stdio.h"
#include <TypeDef.h>
#include "List.h"
#include "Ftl_sys.h"
#include "PhyManage.h"

/**********************************************************************
Function: 
Description: 
Input: 
Output: 
Return: 
Others: 
Modify Date:
Author :
************************************************************************/
extern void BeFlushMap(U8 *buffer, U32 Index,sim_nftl_part_t* part, U8 IsFlushTap);
extern U8 BeReadTapData(U8 *buffer, U32 Index,sim_nftl_part_t* part,U8 IsReadMap);
extern void GetMlcWRPoint(sim_nftl_part_t* part);
extern void Update_TmpTab(U32 lpa,U32 phy_addr,sim_nftl_part_t* part);
extern void UpdateVpc(U32 lpa, U32 newAddr,sim_nftl_part_t* part);
extern void decreaseVpc(U32 block,sim_nftl_part_t* part);
extern void increaseVpc(U32 block,sim_nftl_part_t* part);
extern void Flush_Templ2p(sim_nftl_part_t* part);
extern void FlushCkpt(sim_nftl_part_t* part);
extern U32 GetPSTap(U16 block,sim_nftl_part_t* part, U8 * buf);
extern void Flush_PS(sim_nftl_part_t* part);

void sysBlockRecycling(sim_nftl_part_t* part)
{
	U32 buffer[MAP_PAGE_LPA_NUM];
	U32 buffer2[MAP_PAGE_LPA_NUM];
	U32 mapIndex;

	PFA_T pfa,pfa2;
	U16 block;
	
	U16 i;
	U8 page;

	while(part->free_sys_block_num <= SYS_BLOCK_RECYCLE_THREHOLD)
	{
		block = pop_from_list_head(&part->sys_data_blk_head);
		ASSERT(INVALID_U16 != block);

		if(part->vpc[block] == 0)
		{
			insert_data_into_double_link(&part->sys_free_blk_head,block);
			part->free_sys_block_num++;
			break;
		}

		//confirm page num valid
		pfa.addr32 = part->TapIndexAddr.TapIndex_Writer_Point.addr32;
		page = pfa.addrBit.page;
		pfa.addrBit.page = page-1;
		
		FW_ReadFlash((U8 *)buffer, pfa.addr32,SEC_OF_PAGE, MAPINDEX_LOGIC_NUM,TAP_DATA,MAPINDEX_LOGIC_NUM);

		pfa.addr32 = 0;
		pfa.addrBit.block = block;

		pfa2.addr32 = 0;
		pfa2.addrBit.block = block +1;
		
		for(i = 0;i < MAX_INDEX_NUM;i++)
		{
			//confirm the map or PS valid
			if(buffer[i] >= pfa.addr32 && buffer[i] < pfa2.addr32)
			{
				mapIndex = FW_ReadFlash((U8 *)buffer2, buffer[i],SEC_OF_PAGE, MAP_LOGIC_NUM,TAP_DATA, MAP_LOGIC_NUM);
				BeFlushMap((U8 *)buffer2, mapIndex, part, FLUSH_MAP);
			}
		}

		for(i = MAX_INDEX_NUM; i<MAX_INDEX_NUM + PS_PAGE_CNT;i++)
		{
			//confirm the map or PS valid
			if(buffer[i] >= pfa.addr32 && buffer[i] < pfa2.addr32)
			{
				mapIndex = FW_ReadFlash((U8 *)buffer2, buffer[i],SEC_OF_PAGE, PS_LOGIC_NUM,TAP_DATA, PS_LOGIC_NUM);
				BeFlushMap((U8 *)buffer2, mapIndex, part, FLUSH_PS);
			}
		}
		insert_data_into_double_link(&part->sys_free_blk_head,block);
		part->free_sys_block_num++;
		break;	
	}
}

U16 searchMinVpcFromDataBlockList(sim_nftl_part_t* part)
{
	U16 block;
	U16 num = 1;

	U16 minVpc = INVALID_U16;
	U16 minVpcBlock = INVALID_U16;

	block = part->mlc_data_blk_head->blkNO_in_chip;
	ASSERT(INVALID_U16 != block);
	while(block < BLOCK_OF_LUN)
	{	
		num++;
		if(minVpc > part->vpc[block])
		{
			minVpc = part->vpc[block];
			minVpcBlock = block;
		}
		block = locate_link_by_pos(part->mlc_data_blk_head,num);
	}
	
	return minVpcBlock;
}

U32 gLastMlcAddr = INVALID_U32;
void DogarGarbageCollection(U8 GCpagecnt,sim_nftl_part_t* part)
{
	PFA_T Source_pfa;
	PFA_T Dest_pfa;
	U32 PSBase;
	U32 logic_page;
	
	U16 block;
	U16 page = 0;
	U16 PSOffset;
	U8 PSIndex;
	U8 pageIndex;
	U8 pageOffset; 
	U8 buffer[CAP_OF_PAGE];
	U8 i ,j;
	U8 pageValidMap;
	U8 page_bit;
	
	Source_pfa.addr32 = gLastMlcAddr;
	block = Source_pfa.addrBit.block;
	page = Source_pfa.addrBit.page;

	PSIndex = block / PS_BLKCNT_IN_ONEPAGE;
	PSOffset = block % PS_BLKCNT_IN_ONEPAGE;
	PSBase = PSOffset * BLK_OCCUPY_BYTES_IN_PS;

	pageIndex = page / PAGES_OF_BYTE;
	pageOffset = page % PAGES_OF_BYTE;

	GetPSTap(block,part,part->TempPS[0].PSstatus);
	
	//BeReadTapData((U8 *)PSTap, PSIndex, part, READ_PS);  //read temp
	// 1.judge page valid
	for(i = pageIndex;i < BLK_OCCUPY_BYTES_IN_PS;i++)
	{
		//pageValidMap = PSTap[PSBase+i];
		pageValidMap = part->TempPS[0].PSstatus[i];
		if(0 == pageValidMap)
		{
			page += 8;
			pageOffset = 0;	
			continue;
		}
		
		for(j = pageOffset;j < PAGES_OF_BYTE;j++)
		{
			if(j > 0)
			{
				pageValidMap >>= j;
			}
			// 1.judge page valid
			page_bit = pageValidMap & 0x01;
			if(page_bit == 1)  //valid
			{
				Source_pfa.addrBit.page = page;
				// read valid page
				logic_page = FW_ReadFlash(buffer, Source_pfa.addr32, SEC_OF_PAGE, 0, TAP_DATA, DATA_LOGIC_NUM);
				// Get WRPoint
				GetMlcWRPoint(part);
				Dest_pfa.addr32 = part->TapIndexAddr.mlc_Writer_Point.addr32;
				// update Map  VPC
				Update_TmpTab(logic_page, Dest_pfa.addr32,part);
				UpdateVpc(logic_page, Dest_pfa.addr32, part);
				//decreaseVpc(Source_pfa.addrBit.block,part);
				//increaseVpc(Dest_pfa.addrBit.block,part);
				// write valid page
				PhyWritePage(buffer, Dest_pfa.addr32, SEC_OF_PAGE, logic_page, LOGIC_DATA, DATA_LOGIC_NUM);
				GCpagecnt--;
				
				part->TapIndexAddr.mlc_Writer_Point.addr32 = GetNextPhyAddress(part->TapIndexAddr.mlc_Writer_Point.addr32);
				if (INVALID_U32 == part->TapIndexAddr.mlc_Writer_Point.addr32)////Get to Block End
				{
					block = BLOCK_FROM_PFA(Dest_pfa.addr32);
					insert_data_into_double_link(&part->mlc_data_blk_head,block);

					Flush_Templ2p(part);
					part->l2p_map_tap_index = 0;
					Flush_PS(part);
					FlushCkpt(part);
					sysBlockRecycling(part);
				}
			}
			pageValidMap >>= 1;
			page++;
			if((0 == GCpagecnt) || (PAGE_OF_BLOCK == page))  //once GC over or blk GC over
			{
				if(PAGE_OF_BLOCK == page) //blk GC over
				{
					gLastMlcAddr = INVALID_U32;
					insert_data_into_double_link(&part->mlc_free_blk_head,Source_pfa.addrBit.block);
					part->free_data_block_num++;
					//return;
				}
				else
				{
					Source_pfa.addrBit.page = page;
					gLastMlcAddr = Source_pfa.addr32;
				}
				return;
			}			
		}
		pageOffset = 0;	
	}	
}


void DataBlockRecycling(sim_nftl_part_t* part)
{
	PFA_T pfa;
	U16 block;
	U16 blockVpc;
	U8 GCPageNum;
	
	if(part->free_data_block_num <= DATA_BLOCK_RECYCLE_THREHOLD)
	{
		if(INVALID_U32 == gLastMlcAddr)
		{
			//1.find VPCmin
			block = searchMinVpcFromDataBlockList(part);
			if((block < FW_BLK_NUM + SYS_BLK_NUM) || (block >= BLOCK_OF_LUN))
			{
				LOG_ERROR("No Data blk can GC\n");	
			}
			delete_data_from_double_link(&part->mlc_data_blk_head,block);
			pfa.addr32 = 0;
			pfa.addrBit.block = block;
			gLastMlcAddr = pfa.addr32;
		}
		else   //last GC blk are no empty
		{
			pfa.addr32 = gLastMlcAddr;
			block = pfa.addrBit.block;
		}

		blockVpc = part->vpc[block];
		if(blockVpc == 0)
		{
			insert_data_into_double_link(&part->mlc_free_blk_head,block);
			part->free_data_block_num++;
			gLastMlcAddr = INVALID_U32;
			return;
		}

		// 2.confirm GC page
		if(part->free_data_block_num <= DATA_BLOCK_MIN)
		{
			GCPageNum = GC_PAGE_NUM_level1;
		}
		else if(blockVpc >= VPC_RECYCLE_THREHOLD)
		{
			GCPageNum = GC_PAGE_NUM_level2;
		}
		else
		{
			GCPageNum = GC_PAGE_NUM_level3;
		}
		DogarGarbageCollection(GCPageNum,part);
	}
}

