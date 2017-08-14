#ifndef _PHYMANAGE_H
#define _PHYMANAGE_H
//#pragma once
#include <TypeDef.h>
#include <FlashController/FlashControllerDef.h>

#define PAGE_OF_BLOCK	256		
#define CAP_OF_PAGE		(8*1024)		 
#define SEC_OF_PAGE		16			
#define SEC_OF_BLOCK	4096		
#define BLOCK_OF_LUN	2128
#define MAP_PAGE_LPA_NUM (CAP_OF_PAGE >> 2)

#define LUN_OF_CHIP		1
#define	CHIP_OF_CHANNEL	1
#define CHANNEL_NUM		1
#define PLANE_NUM       2

#define FW_BLK_NUM    2
#define SYS_BLK_NUM   30   //sys blk :32 mlc blk:2096
#define LOGIC_SECTOR_MAX ((BLOCK_OF_LUN - SYS_BLK_NUM - FW_BLK_NUM) * PAGE_OF_BLOCK * SEC_OF_PAGE)
#define LOGIC_PAGE_MAX   ((BLOCK_OF_LUN - SYS_BLK_NUM - FW_BLK_NUM) * PAGE_OF_BLOCK )
#define MAX_INDEX_NUM    (LOGIC_PAGE_MAX >> 11) //262

#define BLK_OCCUPY_BYTES_IN_PS (PAGE_OF_BLOCK >> 3) //32
#define PS_BLKCNT_IN_ONEPAGE   (CAP_OF_PAGE >> 5)        //(CAP_OF_PAGE/BLK_OCCUPY_BYTES_IN_PS)=256
#define PS_PAGE_CNT        9  // CAP_OF_PAGE/32= 256blk  2128/256=8+80=9blk

#define DATA_LOGIC_NUM      LOGIC_PAGE_MAX        //536576
#define MAP_LOGIC_NUM       (LOGIC_PAGE_MAX+1)    //536577
#define PS_LOGIC_NUM        (LOGIC_PAGE_MAX+2)    //536578

#define ERASE_LOGIC_NUM     (LOGIC_PAGE_MAX+3)    //536579
#define VPC_LOGIC_NUM       (LOGIC_PAGE_MAX+4)    //536580
#define MAPINDEX_LOGIC_NUM  (LOGIC_PAGE_MAX+5)    //536581

#define TAP_DATA 1
#define LOGIC_DATA 0

//#define BOUND_OF_ADDR 0x85000
#define INVALID_U32 0xFFFFFFFF
#define INVALID_U16 0xFFFF
#define INVALID_U8 0xFF


void FW_EarseFlash(U32 _blk);
void FW_FlashWriteManage(U8 *_pCache, U32 _addr, U32 _curLen); 
void PhyWritePage(U8 *_pCache, U32 _addr, U32 _curLen, U32 logic_page_no,U8 IsTapData, U32 userFlag);
U32 FW_ReadFlash(U8 *_pCache, U32 _addr, U32 _curLen, U32 logic_page_no,U8 IsTapData, U32 userDataFlag);
void BlockPhyRead(U32 _blk, U8 *_pCache);
void BlockPhyWrite(U32 _blk, U8 *_pCache);
void AddData2Host(U8 *_pCache, U8 *_pSrcData, U32 _addr, U32 _curLen); //spell the current data in the Cache
U32 GetOffsetInBlock(U32 _logicAddr);
U32 GetBlkAddrFromLogicAddr(U32 _logicAddr);

#endif
