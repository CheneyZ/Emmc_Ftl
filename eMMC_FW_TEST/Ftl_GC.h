/**********************************************************
Description	:		NAND flash GC
Author		:		Caden_liu
Data		:		2017.7.18
**********************************************************/
#ifndef _FTL_GC_H
#define _FTL_GC_H
#include "Ftl_sys.h"
#define SYS_BLOCK_RECYCLE_THREHOLD (5)
#define DATA_BLOCK_RECYCLE_THREHOLD (25)  //need confirm
#define DATA_BLOCK_MIN   8
#define VPC_RECYCLE_THREHOLD    (86)

#define GC_PAGE_NUM_level1 32
#define GC_PAGE_NUM_level2 16
#define GC_PAGE_NUM_level3 8

void sysBlockRecycling(sim_nftl_part_t* part);
void DataBlockRecycling(sim_nftl_part_t* part);

#endif