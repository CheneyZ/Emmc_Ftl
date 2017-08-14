#ifndef _EMMC_FW_TEST_H
#define _EMMC_FW_TEST_H
//#pragma once
#include <TypeDef.h>

#include <windows.h>
#include <InnerFlash/InnerFlash.h>
#include <Log/ILog.h>
#include <assert.h>
#include <string>
#include <vector>
#include "PhyManage.h"
#include "Ftl_sys.h"


void SDC_Main(void);

void FW_ReadBuffer_eMMC(U8 *_pCache, U32 _addr, U32 _nLen);
void FW_WriteBuffer_eMMC(U8 *_pCache, U32 _addr, U32 _nLen);

void EMMCRead(sim_nftl_part_t* part);
void EMMCWrite(sim_nftl_part_t* part);

#endif
