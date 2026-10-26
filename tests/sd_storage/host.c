/* Whole production driver, real local BSP declarations; simulated hardware only. */
#include <assert.h>
#include <stdio.h>
#include "FATFS/Target/sd_diskio.c"
SysTick_Type host_systick;
uint32_t host_ipsr,host_primask,host_basepri,host_faultmask;
static uint32_t tick, step, busy, polls, reads, writes, inits, infos;
static uint8_t present, transfer_result, init_result;
static uint32_t last_count, last_sector, last_timeout, remove_at;
uint32_t HAL_GetTick(void) { uint32_t n=tick; tick+=step; return n; }
uint8_t BSP_SD_IsDetected(void) { return polls >= remove_at ? SD_NOT_PRESENT : present; }
uint8_t BSP_SD_Init(void) { ++inits; return init_result; }
uint8_t BSP_SD_GetCardState(void) { ++polls; assert(polls<100000); return polls<=busy?SD_TRANSFER_BUSY:MSD_OK; }
void BSP_SD_GetCardInfo(BSP_SD_CardInfo *i) { ++infos; i->LogBlockNbr=1024; i->LogBlockSize=512; }
uint8_t BSP_SD_ReadBlocks(uint32_t *b,uint32_t s,uint32_t n,uint32_t t)
{ assert(b);++reads;last_count=n;last_sector=s;last_timeout=t;return transfer_result; }
uint8_t BSP_SD_WriteBlocks(uint32_t *b,uint32_t s,uint32_t n,uint32_t t)
{ assert(b);++writes;last_count=n;last_sector=s;last_timeout=t;return transfer_result; }
static BYTE data[128*512];
/* Test-only simulated reboot. Production provides no latch-clear operation. */
static void reboot(void)
{
 Stat=STA_NOINIT;Fault=0;host_systick.CTRL=3;
 host_ipsr=host_primask=host_basepri=host_faultmask=0;
 tick=0;step=1;busy=polls=reads=writes=inits=infos=0;
 present=SD_PRESENT;transfer_result=init_result=MSD_OK;remove_at=UINT32_MAX;
}
static void start(void) { reboot();assert(SD_initialize(0)==0 && inits==1);polls=0; }
static void locked(void)
{
 uint32_t r=reads,w=writes,i=inits,p=polls,inf=infos;DWORD v;
 for(unsigned n=0;n<20;n++) {
  assert(SD_status(0)&STA_NOINIT);assert(SD_initialize(0)&STA_NOINIT);
  assert(SD_read(0,data,0,1)==RES_NOTRDY);assert(SD_write(0,data,0,1)==RES_NOTRDY);
  assert(SD_ioctl(0,CTRL_SYNC,0)==RES_NOTRDY);
  assert(SD_ioctl(0,GET_SECTOR_COUNT,&v)==RES_NOTRDY);
 }
 assert(reads==r && writes==w && inits==i && polls==p && infos==inf);
}
int main(void)
{
 reboot();assert(SD_read(0,data,0,1)==RES_NOTRDY && reads==0);
 present=SD_NOT_PRESENT;assert(SD_initialize(0)==(STA_NOINIT|STA_NODISK) && inits==0);
 present=SD_PRESENT;assert(SD_initialize(0)==0 && inits==1);
 for(unsigned n=1;n<=128;n++) {
  start();assert(SD_read(0,data,1024-n,n)==RES_OK && reads==1);
  assert(last_count==n && last_sector==1024-n && last_timeout==5000);
  assert(SD_write(0,data,1024-n,n)==RES_OK && writes==1 && last_count==n);
  assert(SD_initialize(0)==0 && inits==1);
 }
 puts("PASS all 1..128 sector read/write counts; exact range edge and 5000ms HAL argument");
 start();busy=4;assert(SD_write(0,data,0,1)==RES_OK && polls==5);
 start();busy=UINT32_MAX;assert(SD_read(0,data,0,1)==RES_ERROR && polls==5000);locked();
 start();tick=UINT32_MAX-2500;busy=UINT32_MAX;assert(SD_write(0,data,0,1)==RES_ERROR && polls==5000);locked();
 start();tick=UINT32_MAX-2;busy=4;assert(SD_read(0,data,0,1)==RES_OK && tick<10);
 puts("PASS finite readiness, permanent busy deadline, tick wrap success/timeout; persistent reboot-only fault");
 start();transfer_result=MSD_ERROR;assert(SD_read(0,data,0,1)==RES_ERROR && reads==1 && polls==0);locked();
 start();transfer_result=MSD_ERROR;assert(SD_write(0,data,0,1)==RES_ERROR && writes==1 && polls==0);locked();
 start();present=SD_NOT_PRESENT;assert(SD_write(0,data,0,1)==RES_NOTRDY && writes==0);present=SD_PRESENT;locked();
 reboot();init_result=MSD_ERROR;assert(SD_initialize(0)&STA_NOINIT);init_result=MSD_OK;locked();
 reboot();busy=UINT32_MAX;assert(SD_initialize(0)&STA_NOINIT);locked();
 start();busy=10;remove_at=2;assert(SD_write(0,data,0,1)==RES_NOTRDY && writes==1);remove_at=UINT32_MAX;locked();
 puts("PASS BSP read/write/init failure, ambiguous write blocks subsequent I/O; removal before/during transfer and reinsertion cannot clear latch");
 start();assert(SD_ioctl(0,CTRL_SYNC,0)==RES_OK);polls=0;busy=3;
 assert(SD_ioctl(0,CTRL_SYNC,0)==RES_OK && polls==4);
 polls=0;busy=UINT32_MAX;assert(SD_ioctl(0,CTRL_SYNC,0)==RES_ERROR);locked();
 start();busy=UINT32_MAX;assert(SD_status(0)&STA_NOINIT);locked();
 puts("PASS CTRL_SYNC ready/delayed/timeout/fault; status cannot clear or bypass fault");
 start();DWORD v=0;WORD size=0;
 assert(SD_ioctl(0,GET_SECTOR_COUNT,&v)==RES_OK && v==1024);
 assert(SD_ioctl(0,GET_SECTOR_SIZE,&size)==RES_OK && size==512);
 assert(SD_ioctl(0,GET_BLOCK_SIZE,&v)==RES_OK && v==1);
 assert(SD_ioctl(0,99,&v)==RES_PARERR);
 assert(SD_ioctl(0,GET_SECTOR_COUNT,0)==RES_PARERR);
 for(unsigned n=0;n<2;n++) {
  assert((n?SD_read(0,0,0,1):SD_write(0,0,0,1))==RES_PARERR);
  assert((n?SD_read(0,data,0,0):SD_write(0,data,0,0))==RES_PARERR);
  assert((n?SD_read(0,data,0,129):SD_write(0,data,0,129))==RES_PARERR);
  assert((n?SD_read(0,data,1024,1):SD_write(0,data,1024,1))==RES_PARERR);
  assert((n?SD_read(0,data,UINT32_MAX,2):SD_write(0,data,UINT32_MAX,2))==RES_PARERR);
 }
 assert(SD_read(1,data,0,1)==RES_PARERR && SD_initialize(1)&STA_NOINIT);
 assert(!Fault && reads==0 && writes==0 && SD_status(0)==0);
 puts("PASS invalid pointers/counts/LUN/overflowing ranges/ioctls; invalid input does not poison valid operation");
 uint32_t *context[]={&host_ipsr,&host_primask,&host_basepri,&host_faultmask};
 for(unsigned n=0;n<4;n++) {
  start();*context[n]=1;
  assert(SD_read(0,data,0,1)==RES_NOTRDY && SD_write(0,data,0,1)==RES_NOTRDY);
  assert(SD_status(0)&STA_NOINIT);assert(SD_initialize(0)&STA_NOINIT);
  assert(SD_ioctl(0,CTRL_SYNC,0)==RES_NOTRDY && reads==0 && writes==0 && polls==0);
  *context[n]=0;assert(SD_status(0)==0 && !Fault);
 }
 for(unsigned n=0;n<3;n++){start();host_systick.CTRL=n;assert(SD_read(0,data,0,1)==RES_NOTRDY && !reads);}
 puts("PASS IRQ/masked/suspended tick rejection; no reset/remount/retry dependencies in linked driver");
}
