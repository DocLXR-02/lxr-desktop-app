/* Native register model: tests actual extracted bodies, never physical SD. */
#include <stdint.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "constants.h"
#ifndef BLOCKSIZE
#define BLOCKSIZE 512u
#endif
#ifndef HAL_SD_CARD_TRANSFER
#define HAL_SD_CARD_TRANSFER 4u
#define HAL_SD_CARD_SENDING 5u
#define HAL_SD_CARD_RECEIVING 6u
#define HAL_SD_CARD_PROGRAMMING 7u
#define HAL_SD_CARD_ERROR 255u
#endif
typedef uint32_t HAL_SD_CardStateTypeDef;
typedef enum { HAL_OK, HAL_ERROR, HAL_BUSY, HAL_TIMEOUT } HAL_StatusTypeDef;
enum {HAL_SD_STATE_RESET,HAL_SD_STATE_READY,HAL_SD_STATE_TIMEOUT,HAL_SD_STATE_BUSY,HAL_SD_STATE_ERROR=15};
typedef struct { volatile uint32_t STA,ICR,DCTRL,RESPCMD,RESP1; } SDMMC_TypeDef;
typedef struct { uint32_t DataTimeOut,DataLength,DataBlockSize,TransferDir,TransferMode,DPSM; } SDMMC_DataInitTypeDef;
typedef struct { uint32_t Argument,CmdIndex,Response,WaitForInterrupt,CPSM; } SDMMC_CmdInitTypeDef;
typedef struct { SDMMC_TypeDef *Instance;uint32_t State,ErrorCode,Context;struct {uint32_t LogBlockNbr,CardType,RelCardAdd,CardVersion;} SdCard; } SD_HandleTypeDef;
static struct { uint32_t CTRL; } systick={3};
#define SysTick (&systick)
#define SysTick_CTRL_ENABLE_Msk 1u
#define SysTick_CTRL_TICKINT_Msk 2u
static uint32_t ipsr,primask,basepri,faultmask;
#define __IO volatile
#define __get_IPSR() ipsr
#define __get_PRIMASK() primask
#define __get_BASEPRI() basepri
#define __get_FAULTMASK() faultmask
static uint32_t tick,remaining,commands,status_error,status_response,command_error,stop_fail;
static uint32_t voltage_response;
static unsigned transfer_mode; /* 0 timeout, 1 FIFO, 2 CRC, 3 hardware timeout, 4 read FIFO tail. */
static SDMMC_TypeDef regs;
SD_HandleTypeDef hsd1;
static uint32_t HAL_GetTick(void){return tick++;}
#define __SDMMC_GET_FLAG(r,f) ((r)->STA & (f))
#define __SDMMC_CLEAR_FLAG(r,f) do {(r)->ICR=(f);(r)->STA &= ~(f);}while(0)
static uint32_t data_flag(SD_HandleTypeDef *h,uint32_t mask)
{
 (void)h;
 if(transfer_mode==2) return mask & SDMMC_FLAG_DCRCFAIL;
 if(transfer_mode==3) return mask & SDMMC_FLAG_DTIMEOUT;
 if(transfer_mode==4 && (mask & SDMMC_FLAG_DATAEND)) return SDMMC_FLAG_DATAEND;
 if(!transfer_mode) return 0;
 if(mask & (SDMMC_FLAG_DATAEND | SDMMC_FLAG_RXOVERR)) return remaining==0 ? (mask & SDMMC_FLAG_DATAEND):0;
 if(mask & (SDMMC_FLAG_RXFIFOHF|SDMMC_FLAG_TXFIFOHE|SDMMC_FLAG_RXDAVL)) return remaining?mask:0;
 return 0;
}
#define __HAL_SD_GET_FLAG(h,f) data_flag(h,f)
#define __HAL_SD_CLEAR_FLAG(h,f) do {(h)->Instance->ICR=(f);}while(0)
static uint32_t SDMMC_GetCommandResponse(SDMMC_TypeDef *r){return r->RESPCMD;}
static uint32_t SDMMC_GetResponse(SDMMC_TypeDef *r,uint32_t n){(void)n;return r->RESP1;}
static uint32_t SDMMC_ConfigData(SDMMC_TypeDef *r,SDMMC_DataInitTypeDef *c){remaining=c->DataLength;r->DCTRL=c->DPSM;return 0;}
static uint32_t SDMMC_ReadFIFO(SDMMC_TypeDef *r){(void)r;assert(remaining>=4);remaining-=4;return 0x12345678;}
static uint32_t SDMMC_WriteFIFO(SDMMC_TypeDef *r,uint32_t *v){(void)r;(void)v;assert(remaining>=4);remaining-=4;return 0;}
static uint32_t transfer_command(SDMMC_TypeDef *r,uint32_t a){(void)r;(void)a;commands++;return command_error;}
#define SDMMC_CmdReadMultiBlock transfer_command
#define SDMMC_CmdReadSingleBlock transfer_command
#define SDMMC_CmdWriteMultiBlock transfer_command
#define SDMMC_CmdWriteSingleBlock transfer_command
static uint32_t SDMMC_CmdSendStatus(SDMMC_TypeDef *r,uint32_t a){(void)a;r->RESP1=status_response;return status_error;}
static uint32_t SDMMC_SendCommand(SDMMC_TypeDef *r,SDMMC_CmdInitTypeDef *c){commands++;r->RESPCMD=c->CmdIndex;r->RESP1=0;r->STA=stop_fail?0:SDMMC_FLAG_CMDREND;return 0;}
static uint32_t SDMMC_CmdGoIdleState(SDMMC_TypeDef *r){return transfer_command(r,0);}
static uint32_t SDMMC_CmdOperCond(SDMMC_TypeDef *r){return transfer_command(r,0);}
static uint32_t SDMMC_CmdAppCommand(SDMMC_TypeDef *r,uint32_t a){return transfer_command(r,a);}
static uint32_t SDMMC_CmdAppOperCommand(SDMMC_TypeDef *r,uint32_t a){r->RESP1=voltage_response;return transfer_command(r,a);}
#include "production.c"
static void reset(void)
{
 memset(&regs,0,sizeof regs);memset(&hsd1,0,sizeof hsd1);hsd1.Instance=&regs;
 hsd1.State=HAL_SD_STATE_READY;hsd1.SdCard.LogBlockNbr=1024;hsd1.SdCard.CardType=CARD_SDHC_SDXC;
 tick=remaining=commands=status_error=status_response=command_error=stop_fail=0;transfer_mode=0;voltage_response=0;
 ipsr=primask=basepri=faultmask=0;systick.CTRL=3;
}
static uint32_t response(unsigned type)
{
 uint16_t rca=0;
 switch(type){case 1:return SDMMC_GetCmdResp1(&regs,13,5000);case 2:return SDMMC_GetCmdResp2(&regs);
 case 3:return SDMMC_GetCmdResp3(&regs);case 6:return SDMMC_GetCmdResp6(&regs,13,&rca);
 case 7:return SDMMC_GetCmdResp7(&regs);default:return SDMMC_GetCmdError(&regs);}
}
int main(void)
{
 const unsigned types[]={0,1,2,3,6,7};
 for(unsigned n=0;n<6;n++) {
  unsigned type=types[n];reset();regs.RESPCMD=13;regs.STA=type?SDMMC_FLAG_CMDREND:SDMMC_FLAG_CMDSENT;
  assert(response(type)==SDMMC_ERROR_NONE);
  reset();tick=UINT32_MAX-2500;assert(response(type)==SDMMC_ERROR_TIMEOUT && tick<3000);
  reset();regs.RESPCMD=13;regs.STA=SDMMC_FLAG_CMDREND|SDMMC_FLAG_CMDACT;assert(response(type)==SDMMC_ERROR_TIMEOUT);
  reset();systick.CTRL=0;assert(response(type)==SDMMC_ERROR_TIMEOUT && tick==0);
 }
 for(unsigned n=1;n<6;n++) {
  unsigned type=types[n];reset();regs.STA=SDMMC_FLAG_CTIMEOUT;assert(response(type)==SDMMC_ERROR_CMD_RSP_TIMEOUT);
  reset();regs.STA=SDMMC_FLAG_CCRCFAIL;regs.RESPCMD=13;
  assert(response(type)==(type==3?SDMMC_ERROR_NONE:SDMMC_ERROR_CMD_CRC_FAIL));
 }
 reset();regs.STA=SDMMC_FLAG_CMDREND;regs.RESPCMD=99;
 assert(SDMMC_GetCmdResp1(&regs,13,5000)==SDMMC_ERROR_CMD_CRC_FAIL);
 uint16_t rca=123;regs.STA=SDMMC_FLAG_CMDREND;assert(SDMMC_GetCmdResp6(&regs,13,&rca)==SDMMC_ERROR_CMD_CRC_FAIL && rca==123);
 reset();assert(SDMMC_GetCmdResp1(&regs,13,UINT32_MAX)==SDMMC_ERROR_TIMEOUT && tick==5001);
 reset();assert(SDMMC_GetCmdResp1(&regs,13,0)==SDMMC_ERROR_TIMEOUT && tick==0);
 reset();stop_fail=1;assert(SDMMC_CmdStopTransfer(&regs)==SDMMC_ERROR_TIMEOUT && commands==1 && tick==5001);
 uint32_t *masks[]={&ipsr,&primask,&basepri,&faultmask};
 for(unsigned n=0;n<4;n++){reset();*masks[n]=1;assert(SDMMC_GetCmdResp1(&regs,13,5000)==SDMMC_ERROR_TIMEOUT && tick==0);}
 puts("PASS actual R1/R2/R3/R6/R7/CMD0 bodies: success, missing/active response, hardware timeout, CRC exception, index mismatch, wrap, pre-tick, zero/huge timeout, bounded STOP");
 reset();tick=UINT32_MAX-2500;assert(SD_PowerON(&hsd1)==HAL_SD_ERROR_TIMEOUT && tick<3000);
 reset();voltage_response=0x80000000u|SDMMC_HIGH_CAPACITY;assert(SD_PowerON(&hsd1)==HAL_SD_ERROR_NONE && hsd1.SdCard.CardType==CARD_SDHC_SDXC);
 reset();command_error=SDMMC_ERROR_CMD_RSP_TIMEOUT;assert(SD_PowerON(&hsd1)==command_error && commands==1);
 puts("PASS actual voltage negotiation: ready, command failure, bounded never-ready and tick wrap");
 uint8_t data[1024];memset(data,0x5a,sizeof data);
 for(unsigned write=0;write<2;write++) {
  reset();tick=UINT32_MAX-2;
  HAL_StatusTypeDef r=write?HAL_SD_WriteBlocks(&hsd1,data,0,2,4):HAL_SD_ReadBlocks(&hsd1,data,0,2,4);
  assert(regs.DCTRL==SDMMC_DPSM_ENABLE);
  assert(r==HAL_TIMEOUT && (hsd1.ErrorCode & HAL_SD_ERROR_TIMEOUT) && hsd1.State==HAL_SD_STATE_TIMEOUT && hsd1.Context!=SD_CONTEXT_NONE && commands==1);
  uint32_t context=hsd1.Context;unsigned before=commands;
  r=write?HAL_SD_WriteBlocks(&hsd1,data,0,2,4):HAL_SD_ReadBlocks(&hsd1,data,0,2,4);
  assert(r!=HAL_OK && context==hsd1.Context && commands==before);
  reset();command_error=SDMMC_ERROR_CMD_CRC_FAIL;
  r=write?HAL_SD_WriteBlocks(&hsd1,data,0,1,5000):HAL_SD_ReadBlocks(&hsd1,data,0,1,5000);
  assert(r==HAL_ERROR && hsd1.State==HAL_SD_STATE_ERROR && (hsd1.ErrorCode & command_error) && hsd1.Context!=0 && commands==1);
  reset();transfer_mode=2;
  r=write?HAL_SD_WriteBlocks(&hsd1,data,0,1,5000):HAL_SD_ReadBlocks(&hsd1,data,0,1,5000);
  assert(r==HAL_ERROR && hsd1.State==HAL_SD_STATE_ERROR && (hsd1.ErrorCode & HAL_SD_ERROR_DATA_CRC_FAIL));
  reset();transfer_mode=3;
  r=write?HAL_SD_WriteBlocks(&hsd1,data,0,1,5000):HAL_SD_ReadBlocks(&hsd1,data,0,1,5000);
  assert(r==HAL_ERROR && hsd1.State==HAL_SD_STATE_ERROR && (hsd1.ErrorCode & HAL_SD_ERROR_DATA_TIMEOUT));
  for(unsigned count=1;count<=2;count++) {
   reset();transfer_mode=1;
   r=write?HAL_SD_WriteBlocks(&hsd1,data,0,count,5000):HAL_SD_ReadBlocks(&hsd1,data,0,count,5000);
   assert(r==HAL_OK && hsd1.State==HAL_SD_STATE_READY && hsd1.Context==SD_CONTEXT_NONE && remaining==0 && commands==count);
  }
  reset();transfer_mode=1;stop_fail=1;
  r=write?HAL_SD_WriteBlocks(&hsd1,data,0,2,5000):HAL_SD_ReadBlocks(&hsd1,data,0,2,5000);
  assert(r==HAL_ERROR && hsd1.State==HAL_SD_STATE_ERROR && hsd1.Context!=0 && commands==2);
 }
 reset();transfer_mode=4;assert(HAL_SD_ReadBlocks(&hsd1,data,0,1,1)==HAL_TIMEOUT && hsd1.State==HAL_SD_STATE_TIMEOUT && (hsd1.ErrorCode & HAL_SD_ERROR_TIMEOUT));
 puts("PASS actual HAL read/write: timeout/error bits, retained context/non-ready state, direct retry rejection, FIFO single/multiblock success and failed STOP without replay/reset");
 reset();status_error=HAL_SD_ERROR_CMD_CRC_FAIL;hsd1.ErrorCode=HAL_SD_ERROR_TIMEOUT;
 assert(HAL_SD_GetCardState(&hsd1)==HAL_SD_CARD_ERROR && (hsd1.ErrorCode & HAL_SD_ERROR_TIMEOUT));
 assert(BSP_SD_GetCardState()==SD_TRANSFER_ERROR);
 reset();status_response=HAL_SD_CARD_PROGRAMMING<<9;assert(BSP_SD_GetCardState()==SD_TRANSFER_BUSY);
 reset();status_response=HAL_SD_CARD_TRANSFER<<9;assert(BSP_SD_GetCardState()==SD_TRANSFER_OK);
 hsd1.ErrorCode=HAL_SD_ERROR_TIMEOUT;assert(BSP_SD_GetCardState()==SD_TRANSFER_ERROR);
 for(unsigned state=1;state<=8;state++){reset();status_response=state<<9;assert(BSP_SD_GetCardState()==(state==4?SD_TRANSFER_OK:SD_TRANSFER_BUSY));}
 reset();status_response=15<<9;assert(BSP_SD_GetCardState()==SD_TRANSFER_ERROR);
 reset();transfer_mode=1;assert(BSP_SD_ReadBlocks((uint32_t*)data,0,1,5000)==MSD_OK);
 reset();transfer_mode=1;assert(BSP_SD_WriteBlocks((uint32_t*)data,0,1,5000)==MSD_OK);
 reset();assert(BSP_SD_ReadBlocks((uint32_t*)data,0,1,3)==MSD_TIMEOUT);
 reset();assert(BSP_SD_WriteBlocks((uint32_t*)data,0,1,3)==MSD_TIMEOUT && (hsd1.ErrorCode & HAL_SD_ERROR_TIMEOUT));
 reset();command_error=SDMMC_ERROR_CMD_CRC_FAIL;assert(BSP_SD_WriteBlocks((uint32_t*)data,0,1,5000)==MSD_ERROR);
 puts("PASS actual BSP/HAL status: error sentinel, busy/ready/error distinction, historical error preservation and HAL_TIMEOUT propagation");
}
