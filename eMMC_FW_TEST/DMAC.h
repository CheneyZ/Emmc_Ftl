#ifndef _DMAC_H
#define _DMAC_H
#include "DataDef_eMMC.h"
#include <TypeDef.h>
#include "FirmwareProcParam.h"

extern PGetRegister g_pGetRegister;
//extern PSetRegister g_pSetRegister;

/**********common base***********/

extern U8 * g_pMemBaseAddr;
#define MEM_BASE_ADDR        ((U32)g_pMemBaseAddr)

#define   SECTOR_SIZE            (512U)

//RAM基地址
#define REG_BASE_ADDR		(MEM_BASE_ADDR)

//Host DMABuf1 地址
#define MEM_DMABUF1_ADDR        (MEM_BASE_ADDR)
#define MEM_DMABUF1_SIZE        (1024*8)

//Flash DMABuf2 地址
#define MEM_DMABUF2_ADDR        (MEM_DMABUF1_ADDR+MEM_DMABUF1_SIZE)
#define MEM_DMABUF2_SIZE        (1024*8)

/************DMAC**************/
#define  DMAC_REG_ABS_ADDR				   (0x000E2000+REG_BASE_ADDR)
#define  rDMAC_DMA1_INDEX                  (*((U8_V *)(DMAC_REG_ABS_ADDR+0x02)))
#define  rDMAC_DMA1_TOTAL_SECT             (*((U16_V *)(DMAC_REG_ABS_ADDR+0x38)))

//DMA传输方向
typedef enum 
{
	//host->buffer
	HOST_TO_BUFFER = 0,
	//buffer->host
	BUFFER_TO_HOST
}E_DMA_DIRECTION;

#define SDC_DATA_PACKET_STOP  ((g_pGetRegister(INT2_STS) == INT2_READ_STOP) || (g_pGetRegister(INT2_STS) == INT2_WRITE_STOP) )
#define SDC_dDatPktStopEnd    (SDC_DATA_PACKET_STOP)

U32 DMAC_Dma1Start(E_DMA_DIRECTION bMode,U8 * memAddr ,U8 bSecOffs,U16 wSecCnt);

#endif