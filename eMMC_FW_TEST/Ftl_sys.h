/**********************************************************
Description	:		FTL init
Author		:		Caden_liu
Data		:		2017.7.18
**********************************************************/
#ifndef _FTL_SYS_H
#define _FTL_SYS_H

#include <TypeDef.h>
#include "PhyManage.h"
#include "List.h"

#define MULTI_PLANE_ENABLED
#define  MAX_LOGIC_MAP_NUM   2

typedef struct{
    U16  Page_NO;
    U16  blkNO_in_chip;
}_nand_page;  //U32 represent 


//BITs Width in the Physical Flash Address(PFA)
#if 0
#define PFA_OFFSET_START_BIT (0)
#define PFA_OFFSET_BITS      (14)
#define PFA_OFFSET_MASK      ((1<<PFA_OFFSET_BITS) - 1)
#endif

#define PFA_PAGE_START_BIT   (0)
#define PFA_PAGE_BITS        (8)
#define PFA_PAGE_MASK        ((1<<PFA_PAGE_BITS) - 1)

#define PFA_PLANE_START_BIT  (PFA_PAGE_START_BIT + PFA_PAGE_BITS)
#define PFA_PLANE_BITS       (1)
#define PFA_PLANE_MASK       ((1<<PFA_PLANE_BITS) - 1)

#define PFA_BLOCK_START_BIT  (PFA_PAGE_START_BIT + PFA_PAGE_BITS) //(PFA_PLANE_START_BIT + PFA_PLANE_BITS)
#define PFA_BLOCK_BITS       (12)  //11
//#define PFA_BLOCK_MASK       ((1<<PFA_BLOCK_BITS) - 1)
#define PFA_BLOCK_MASK       ((1<<PFA_BLOCK_BITS) - 1)

#define PFA_CE_LUN_START_BIT (PFA_BLOCK_START_BIT + PFA_BLOCK_BITS)
#define PFA_CE_LUN_BITS      (1)
#define PFA_CE_LUN_MASK      ((1<<PFA_CE_LUN_BITS) - 1)

//#define OFFSET_FROM_PFA(pfa)  (((pfa)>>PFA_OFFSET_START_BIT) & PFA_OFFSET_MASK)

#define PAGE_FROM_PFA(pfa)    (((pfa)>>PFA_PAGE_START_BIT) & PFA_PAGE_MASK)
#define PLANE_FROM_PFA(pfa)   (((pfa)>>PFA_PLANE_START_BIT) & PFA_PLANE_MASK)
//#define BLOCK_FROM_PFA(pfa)   (((pfa)>>PFA_BLOCK_START_BIT) & PFA_BLOCK_MASK)
#define BLOCK_FROM_PFA(pfa)   (((pfa)>>PFA_PLANE_START_BIT) & PFA_BLOCK_MASK)
#define CE_LUN_FROM_PFA(pfa)  (((pfa)>>PFA_CE_LUN_START_BIT) & PFA_CE_LUN_MASK)

#define PAGES_OF_BYTE  8
#define WRITE_POINT_LEN 12

#define SETBIT(x,y) ((x)|=(1<<(y)))
#define CLRBIT(x,y)	((x)&= ~(1<<(y)))
#define READ_MAP    1
#define READ_PS     0
#define FLUSH_MAP   1
#define FLUSH_PS    0

//Physical Flash Address
typedef union PFA_S
{
	struct
	{
		U32 page   : PFA_PAGE_BITS;   //Page Number
		//U32 plane  : PFA_PLANE_BITS;  //Plane Number
		U32 block  : PFA_BLOCK_BITS;  //Block Number
		U32 ceLun  : PFA_CE_LUN_BITS; //Ce and Lun
		U32 reserv : 32-PFA_PAGE_BITS-PFA_PLANE_BITS-PFA_BLOCK_BITS-PFA_CE_LUN_BITS;
	} addrBit;
	U32 addr32;
}PFA_T, *pPFA_T;

typedef struct
{
	//_nand_page  srp_blk_addr;
	PFA_T   srp_blk_addr;
	U16     srp_used_count;	  //for Debug
}sim_part_srp;

typedef struct {
    U16   num;
    U16   time:	 15;
    U16   status: 1;
}Zone_status;

#if 0
typedef struct 
{
	U16 erase_cnt : 15;
	U16 function : 1; /*0: free, 1b:data blk */	
}blk_info_t;
#endif

typedef enum {
    FREE_BLK = 0,
    DATA_BLK
}BLOCK_ATTR;

typedef struct L2P_ENTRY_S
{
	U32 lpa;
	PFA_T ppa;
}L2P_ENTRY_T, *pL2P_ENTRY_T;


typedef struct TAP_INDEX_S{
    PFA_T           mapIndex[MAX_INDEX_NUM];            //The first mapping table
    PFA_T           PS_index[PS_PAGE_CNT];
    PFA_T           sys_Writer_Point;
    PFA_T           mlc_Writer_Point;
    PFA_T           TapIndex_Writer_Point;   
}TAP_INDEX_T, *pTAP_INDEX_T;

#define TAPINDEX_LEN sizeof(TAP_INDEX_T)  //1096

typedef struct{
    U16 block;
    U8  PSstatus[32];
}PAGE_STATUS_T;

typedef struct{
    PFA_T           fw[2];
    TAP_INDEX_T     TapIndexAddr;
    
    L2P_ENTRY_T     l2p_map_tap[PAGE_OF_BLOCK];      // temp mapping table
 
    U16             l2p_map_tap_index;
    U16             free_sys_block_num;
    U16             free_data_block_num;
    U16             erase_cnt[BLOCK_OF_LUN];
    U16             vpc[BLOCK_OF_LUN];
    
    PAGE_STATUS_T   TempPS[2];      //TempPS[0]:old, TempPS[1]:new
    
    //4 linked list
    DOUBLE_LINK_NODE*  sys_free_blk_head;
    DOUBLE_LINK_NODE*  sys_data_blk_head;
    DOUBLE_LINK_NODE*  mlc_free_blk_head;
    DOUBLE_LINK_NODE*  mlc_data_blk_head;
}sim_nftl_part_t;   //11688

typedef struct BLOCK_ALLOC_TABLE_S
{
 	PFA_T  fw[2];
	U16 sysRblkNum;          //Num of RBLKs used as SLC
	U16 sysRblk[SYS_BLK_NUM];   //RBLKs used for SLC,
}BLOCK_ALLOC_TABLE_T, *pBLOCK_ALLOC_TABLE_T;

extern sim_nftl_part_t* sim_nftl_part;
void ftl_init(void);
U32 GetNextPhyBlock(PFA_T pfa);
U32 GetBlockPhyAddress(U16 block);
U32 GetNextPhyAddress(U32 addr);

#endif
