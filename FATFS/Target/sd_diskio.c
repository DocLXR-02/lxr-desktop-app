/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    sd_diskio.c
  * @brief   SD Disk I/O driver
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Note: code generation based on sd_diskio_template_bspv1.c v2.1.4
   as "Use dma template" is disabled. */

/* USER CODE BEGIN firstSection */
/* can be used to modify / undefine following code or add new definitions */
/* USER CODE END firstSection*/

#include "ff_gen_drv.h"
#include "sd_diskio.h"

/* Software milliseconds, not the SDMMC hardware data-timeout register.
 * Up to 128 sectors (64 KiB): allow 5 s for the polling transfer and another
 * 5 s for card programming/readiness. Conservative development policy, not a
 * guarantee for every card. Inner HAL command waits remain outside this bound.
 */
#define SD_TRANSFER_TIMEOUT_MS 5000u
#define SD_READY_TIMEOUT_MS 5000u
#define SD_MAX_SECTORS 128u
#define SD_DEFAULT_BLOCK_SIZE 512u

static volatile DSTATUS Stat = STA_NOINIT;
/* Cleared only by C runtime initialization at reboot; never by mount/init. */
static uint8_t Fault;

static int SD_ContextOK(void)
{
  /* HAL's millisecond tick must be able to preempt this foreground caller. */
  return __get_IPSR() == 0u && __get_PRIMASK() == 0u &&
         __get_BASEPRI() == 0u && __get_FAULTMASK() == 0u &&
         (SysTick->CTRL & (SysTick_CTRL_ENABLE_Msk | SysTick_CTRL_TICKINT_Msk)) ==
         (SysTick_CTRL_ENABLE_Msk | SysTick_CTRL_TICKINT_Msk);
}

static DRESULT SD_Fail(void)
{
  Fault = 1u;
  Stat |= STA_NOINIT;
  return RES_ERROR;
}

static DRESULT SD_Access(BYTE lun)
{
  if(lun != 0u) return RES_PARERR;
  if(!SD_ContextOK() || Fault || (Stat & STA_NOINIT)) return RES_NOTRDY;
  if(BSP_SD_IsDetected() != SD_PRESENT) {
    (void)SD_Fail();
    Stat |= STA_NODISK;
    return RES_NOTRDY;
  }
  return RES_OK;
}

static DRESULT SD_WaitReady(void)
{
  uint32_t start = HAL_GetTick();
  for(;;) {
    if(BSP_SD_IsDetected() != SD_PRESENT) {
      (void)SD_Fail();
      Stat |= STA_NODISK;
      return RES_NOTRDY;
    }
    if(BSP_SD_GetCardState() == MSD_OK) return RES_OK;
    if((uint32_t)(HAL_GetTick() - start) >= SD_READY_TIMEOUT_MS)
      return SD_Fail();
  }
}

DSTATUS SD_status(BYTE lun)
{
  if(lun != 0u || !SD_ContextOK()) return STA_NOINIT;
  if(Fault || (Stat & STA_NOINIT)) return Stat;
  if(SD_Access(lun) == RES_OK) (void)SD_WaitReady();
  return Stat;
}

DSTATUS SD_initialize(BYTE lun)
{
  if(lun != 0u || !SD_ContextOK()) return STA_NOINIT;
  if(Fault) return Stat;
  if(!(Stat & STA_NOINIT)) return SD_status(lun);
  /* An absent card at startup has not created an uncertain transfer. */
  if(BSP_SD_IsDetected() != SD_PRESENT) return (Stat = STA_NOINIT | STA_NODISK);
  Stat = STA_NOINIT;
  if(BSP_SD_Init() != MSD_OK) {
    (void)SD_Fail();
    return Stat;
  }
  if(SD_WaitReady() == RES_OK) Stat = 0u;
  return Stat;
}

static int SD_ValidRange(DWORD sector, UINT count)
{
  BSP_SD_CardInfo info;
  BSP_SD_GetCardInfo(&info);
  return count <= info.LogBlockNbr && sector <= info.LogBlockNbr - count;
}

DRESULT SD_read(BYTE lun, BYTE *buff, DWORD sector, UINT count)
{
  DRESULT result;
  if(!buff || count == 0u || count > SD_MAX_SECTORS) return RES_PARERR;
  result = SD_Access(lun);
  if(result != RES_OK) return result;
  if(!SD_ValidRange(sector, count)) return RES_PARERR;
  if(BSP_SD_ReadBlocks((uint32_t*)buff, sector, count, SD_TRANSFER_TIMEOUT_MS) != MSD_OK)
    return SD_Fail();
  return SD_WaitReady();
}

#if _USE_WRITE == 1
DRESULT SD_write(BYTE lun, const BYTE *buff, DWORD sector, UINT count)
{
  DRESULT result;
  if(!buff || count == 0u || count > SD_MAX_SECTORS) return RES_PARERR;
  result = SD_Access(lun);
  if(result != RES_OK) return result;
  if(!SD_ValidRange(sector, count)) return RES_PARERR;
  if(BSP_SD_WriteBlocks((uint32_t*)buff, sector, count, SD_TRANSFER_TIMEOUT_MS) != MSD_OK)
    return SD_Fail();
  return SD_WaitReady();
}
#endif

#if _USE_IOCTL == 1
DRESULT SD_ioctl(BYTE lun, BYTE cmd, void *buff)
{
  BSP_SD_CardInfo info;
  DRESULT result;
  if(cmd != CTRL_SYNC && cmd != GET_SECTOR_COUNT && cmd != GET_SECTOR_SIZE &&
     cmd != GET_BLOCK_SIZE) return RES_PARERR;
  if(cmd != CTRL_SYNC && !buff) return RES_PARERR;
  result = SD_Access(lun);
  if(result != RES_OK) return result;
  if(cmd == CTRL_SYNC) return SD_WaitReady();
  BSP_SD_GetCardInfo(&info);
  switch(cmd) {
  case GET_SECTOR_COUNT: *(DWORD*)buff = info.LogBlockNbr; break;
  case GET_SECTOR_SIZE: *(WORD*)buff = info.LogBlockSize; break;
  /* Preserve the existing block-size reporting convention. */
  case GET_BLOCK_SIZE: *(DWORD*)buff = info.LogBlockSize / SD_DEFAULT_BLOCK_SIZE; break;
  default: return RES_PARERR;
  }
  return RES_OK;
}
#endif

const Diskio_drvTypeDef SD_Driver = {
  SD_initialize, SD_status, SD_read,
#if _USE_WRITE == 1
  SD_write,
#endif
#if _USE_IOCTL == 1
  SD_ioctl,
#endif
};
