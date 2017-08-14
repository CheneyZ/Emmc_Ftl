/**********************************************************
Description	:		FTL init
Author		:		Caden_liu
Data		:		2017.7.18
**********************************************************/
#include "stdafx.h"
#include "Ftl_sys.h"
#include <stdlib.h>
#include"stdio.h"
#include "List.h"

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
sim_nftl_part_t* sim_nftl_part;

U32 GetNextPhyBlock(PFA_T pfa)
{
	if((LUN_OF_CHIP-1) == pfa.addrBit.ceLun)//Next Block
	{
		pfa.addrBit.block++;
		pfa.addrBit.ceLun = 0;
	}
	else
	{
		//Go for next lun
		pfa.addrBit.ceLun++;
	}
	return pfa.addr32;

/*#ifdef MULTI_PLANE_ENABLED
	if(pfa.addrBit.plane != (PLANE_NUM-1)) //Assuem there is 2 Plane
	{
		pfa.addrBit.plane++;//Go for next plane
	}
	else
	{
		pfa.addrBit.plane = 0;
#endif
		if((LUN_OF_CHIP-1) == pfa.addrBit.ceLun)//Next Block
		{
			pfa.addrBit.block++;
			pfa.addrBit.ceLun = 0;
		}
		else
		{
			//Go for next lun
			pfa.addrBit.ceLun++;
		}
	}
	return pfa.addr32;
*/
}

U32 GetBlockPhyAddress(U16 block)
{
	PFA_T pfa;

	pfa.addr32 = 0;
	pfa.addrBit.block = block;
	return pfa.addr32;
}

U32 GetNextPhyAddress(U32 addr)
{
	U32 maxPages;
	PFA_T pfa;
	pfa.addr32 = addr;
	maxPages = (PAGE_OF_BLOCK-1);

//	ASSERT(BOUND_OF_ADDR >= addr);
/*	if(BOUND_OF_ADDR <= addr)
    {
        return addr; //out of bounds
    }
*/

	if(pfa.addrBit.page >= maxPages)////This block is used over
	{

		pfa.addr32 = INVALID_U32;
		//pfa.addr32 = GetNextPhyBlock(pfa);
		//pfa.addrBit.page = 0;
	}
	else
	{
		pfa.addrBit.page++;
	}
	return pfa.addr32;
}

U8 ScanBb(void)
{
	return 0;
}

void Init_Block_Lists(sim_nftl_part_t* part)
{
	//U16 i;
	//U16 num;
	U16 blk;

	for(blk = FW_BLK_NUM;blk < (SYS_BLK_NUM +FW_BLK_NUM);blk++)
	{
		insert_data_into_double_link(&part->sys_free_blk_head,blk);
		part->free_sys_block_num++;
	}

	for(blk = SYS_BLK_NUM +FW_BLK_NUM;blk < BLOCK_OF_LUN;blk++)
	{
		insert_data_into_double_link(&part->mlc_free_blk_head,blk);
		part->free_data_block_num++;
	}		
}

extern void Emmc_Erase(U16 block, sim_nftl_part_t* part);
void Format_Ftl(sim_nftl_part_t* sim_part)
{
	PFA_T pfa;
	U16 block;
	//DOUBLE_LINK_NODE* pNode;
	//1.scan BB
	ScanBb();
	
	pfa.addr32 = 0; //from the first blkaddr
	sim_part->fw[0].addr32 = pfa.addr32;
	sim_part->fw[1].addr32 = GetNextPhyBlock(sim_part->fw[0]);
	//sim_part->fw[2].addr32 = GetNextPhyBlock(sim_part->fw[1]);

	//no consider write the info to nand
	sim_part->sys_data_blk_head= NULL;
	sim_part->sys_free_blk_head = NULL;
	sim_part->mlc_free_blk_head = NULL;
	sim_part->mlc_data_blk_head = NULL;
	
	
	memset(sim_part->TapIndexAddr.mapIndex,0xff,sizeof(sim_part->TapIndexAddr.mapIndex));
	memset(sim_part->TapIndexAddr.PS_index,0xff,sizeof(sim_part->TapIndexAddr.PS_index));

	memset(sim_part->l2p_map_tap,0xff,sizeof(sim_part->l2p_map_tap));
	memset(sim_part->vpc, 0, sizeof(sim_part->vpc));   // init 0

	sim_part->TempPS[0].block = INVALID_U16;
	memset(sim_part->TempPS[0].PSstatus,0,sizeof(sim_part->TempPS[0].PSstatus));
	sim_part->TempPS[1].block = INVALID_U16;
	memset(sim_part->TempPS[1].PSstatus,0,sizeof(sim_part->TempPS[1].PSstatus));

	sim_part->l2p_map_tap_index = 0;
	sim_part->free_sys_block_num = 0;
	sim_part->free_data_block_num = 0;

	for(block = 0;block < BLOCK_OF_LUN;block++)
	{
		sim_part->erase_cnt[block] = 0;
	}
	
	Init_Block_Lists(sim_part);

	block = pop_from_list_head(&sim_part->sys_free_blk_head);
	sim_part->free_sys_block_num--;
	ASSERT(INVALID_U16 != block);
	Emmc_Erase(block,sim_part);

	pfa.addr32 = 0;
	pfa.addrBit.block = block;
	sim_part->TapIndexAddr.TapIndex_Writer_Point.addr32 = pfa.addr32;  //init  TapIndex_Writer_Point

	block = pop_from_list_head(&sim_part->sys_free_blk_head);
	sim_part->free_sys_block_num--;
	ASSERT(INVALID_U16 != block);
	Emmc_Erase(block,sim_part);

	pfa.addr32 = 0;
	pfa.addrBit.block = block;
	sim_part->TapIndexAddr.sys_Writer_Point.addr32 = pfa.addr32;

	sim_part->TapIndexAddr.mlc_Writer_Point.addr32 = INVALID_U32;
}

void ftl_init(void)
{
	sim_nftl_part = (sim_nftl_part_t*)malloc(sizeof(sim_nftl_part_t));
	if(sim_nftl_part == NULL)
	{
		ASSERT(sim_nftl_part != NULL)
		return;
	}

	//1.初始化map表,NAND flash中的各个块信息，创立各种链表。
	Format_Ftl(sim_nftl_part);	
}

