#include "stdafx.h"
#include "DMAC.h"
#include "FirmwareProcParam.h"

extern PGetBuffer g_pGetBuffer;
extern PSetBuffer g_pSetBuffer;

/**********************************************************************
Function: DMAC_Dma1Start
Description: Data transmission, receiving host data or sending data to host
Input: 
Output: 
Return: 
Others: 
Modify Date:
Author :
************************************************************************/
U8 * g_pMemBaseAddr;
U32 DMAC_Dma1Start(E_DMA_DIRECTION bMode,U8 * memAddr ,U8 bSecOffs,U16 wSecCnt)
{
	U8 ret =0;
	U32 WRSecNum = 0;

	rDMAC_DMA1_TOTAL_SECT = wSecCnt;//*((U16 *)(rDMAC_DMA1_TOTAL_SECT))=wSecCnt;
	rDMAC_DMA1_INDEX=bSecOffs;

	while (1)
	{
		if(wSecCnt >0)
		{
			if (bMode == HOST_TO_BUFFER)
			{
				ret = g_pGetBuffer((U8 *)(memAddr+bSecOffs*SECTOR_SIZE), SECTOR_SIZE);
			}
			else
			{
				ret = g_pSetBuffer((U8 *)(memAddr+bSecOffs*SECTOR_SIZE), SECTOR_SIZE);
			}
			if (ret)
			{
				bSecOffs++;
				wSecCnt--;
				WRSecNum++;
			}
			else
			{
				//数据已经接收完成，激励还没有发送stop，需要等待
				while(!SDC_dDatPktStopEnd)
				{
					break;//host数据已经处理完成	
				}
			}
		}
		else
		{
			break;
		}

		if(SDC_dDatPktStopEnd)
		{
			break;
		}
	}

	rDMAC_DMA1_TOTAL_SECT=wSecCnt;
	rDMAC_DMA1_INDEX=bSecOffs;

	return WRSecNum;
}
