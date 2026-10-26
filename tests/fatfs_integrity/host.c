/* Whole production FatFs/generic disk/registration/S1 driver; RAM media only. */
#include <stdint.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
/* Match target widths on LP64 hosts without changing production integer.h. */
#define _FF_INTEGER
typedef int32_t INT,LONG;
typedef uint32_t UINT,DWORD;
typedef uint8_t BYTE;
typedef int16_t SHORT;
typedef uint16_t WORD,WCHAR;
typedef uint64_t QWORD;
#include "Middlewares/Third_Party/FatFs/src/ff.c"
#include "Middlewares/Third_Party/FatFs/src/diskio.c"
#include "Middlewares/Third_Party/FatFs/src/ff_gen_drv.c"
#include "FATFS/Target/sd_diskio.c"
_Static_assert(sizeof(DWORD)==4 && sizeof(WORD)==2,"target integer widths");
SysTick_Type host_systick={3};
uint32_t host_ipsr,host_primask,host_basepri,host_faultmask;
static uint32_t tick,physical_writes,physical_inits,physical_fail_write;
static uint8_t card_present=SD_PRESENT,bsp_result=MSD_OK;
uint32_t HAL_GetTick(void){return tick++;}
uint8_t BSP_SD_IsDetected(void){return card_present;}
uint8_t BSP_SD_Init(void){physical_inits++;return bsp_result;}
uint8_t BSP_SD_GetCardState(void){return MSD_OK;}
void BSP_SD_GetCardInfo(BSP_SD_CardInfo *i){i->LogBlockNbr=256;i->LogBlockSize=512;}
uint8_t BSP_SD_ReadBlocks(uint32_t *p,uint32_t s,uint32_t n,uint32_t timeout)
{(void)p;(void)s;(void)n;(void)timeout;return bsp_result;}
uint8_t BSP_SD_WriteBlocks(uint32_t *p,uint32_t s,uint32_t n,uint32_t timeout)
{(void)p;(void)s;(void)n;(void)timeout;physical_writes++;return physical_writes==physical_fail_write?MSD_ERROR:bsp_result;}
static BYTE media[256][512];
static DWORD sectors[16];
static unsigned writes,reads,syncs,fail_write,init_calls,status_calls;
static DRESULT sync_result;
static DSTATUS available;
static DSTATUS fake_init(BYTE lun)
{assert(lun==0);assert(!disk.is_initialized[0]);init_calls++;return available;}
static DSTATUS fake_status(BYTE lun){assert(lun==0);status_calls++;return available;}
static DRESULT fake_read(BYTE lun,BYTE *p,DWORD s,UINT n)
{assert(lun==0 && s+n<=256);reads++;memcpy(p,media[s],n*512);return RES_OK;}
static DRESULT fake_write(BYTE lun,const BYTE *p,DWORD s,UINT n)
{
 assert(lun==0 && n==1 && s<256 && writes<16);sectors[writes++]=s;
 if(writes==fail_write)return RES_ERROR; /* This mock deliberately has no S1 latch. */
 memcpy(media[s],p,512);return RES_OK;
}
static DRESULT fake_ioctl(BYTE lun,BYTE cmd,void *p)
{assert(lun==0);(void)p;assert(cmd==CTRL_SYNC);syncs++;return sync_result;}
static const Diskio_drvTypeDef fake_driver={fake_init,fake_status,fake_read,fake_write,fake_ioctl};
static FATFS fs;
static FIL file;
static char drive[4];
static void setup(void)
{
 memset(&disk,0,sizeof disk);memset(&fs,0,sizeof fs);memset(&file,0,sizeof file);
 memset(media,0,sizeof media);memset(sectors,0,sizeof sectors);
 writes=reads=syncs=fail_write=init_calls=status_calls=0;available=0;sync_result=RES_OK;
 assert(FATFS_LinkDriver(&fake_driver,drive)==0 && !disk.is_initialized[0]);
 assert(disk_initialize(0)==0 && disk.is_initialized[0] && init_calls==1);
 fs.fs_type=FS_FAT16;fs.id=7;fs.drv=0;fs.csize=1;fs.n_fats=2;
 fs.fatbase=8;fs.fsize=16;fs.n_fatent=1024;fs.database=128;fs.volbase=64;
 fs.winsect=100;
#if !_FS_READONLY
 fs.free_clst=123;fs.last_clst=42;
#endif
 file.obj.fs=&fs;file.obj.id=7;file.obj.sclust=2;file.obj.objsize=512;
 file.flag=FA_READ;
#if !_FS_READONLY
 file.dir_sect=100;file.dir_ptr=fs.win;
#endif
#if _FS_LOCK
 memset(Files,0,sizeof Files);Files[0].fs=&fs;Files[0].ctr=1;file.obj.lockid=1;
#endif
}
#if !_FS_READONLY
static void mirrors(void)
{
 for(unsigned copies=1;copies<=4;copies++) {
  for(unsigned failure=0;failure<=copies;failure++) {
   setup();fs.n_fats=copies;fs.winsect=9;fs.wflag=1;memset(fs.win,0xa5,512);fail_write=failure;
   assert(sync_window(&fs)==(failure?FR_DISK_ERR:FR_OK));
   assert(writes==(failure?failure:copies));assert(fs.wflag==(failure?1:0));
   assert(fs.winsect==9 && fs.win[0]==0xa5);
   for(unsigned n=0;n<writes;n++)assert(sectors[n]==9+n*16);
   if(!failure)for(unsigned n=0;n<copies;n++)assert(!memcmp(media[9+n*16],fs.win,512));
   assert(!syncs); /* No automatic retry or later copy after failure. */
  }
 }
 setup();fs.winsect=100;fs.wflag=1;assert(sync_window(&fs)==FR_OK && writes==1 && !fs.wflag);
 setup();assert(sync_window(&fs)==FR_OK && writes==0);
 puts("PASS actual sync_window: 1..4 FATs, all primary/later failures, stop ordering, dirty retention, no internal replay; non-FAT/clean windows");
}
static void fsinfo(void)
{
 setup();fs.fs_type=FS_FAT32;fs.fsi_flag=1;assert(sync_fs(&fs)==FR_OK);
 assert(writes==1 && sectors[0]==65 && !fs.fsi_flag && syncs==1);
 assert(ld_word(media[65]+BS_55AA)==0xaa55 && ld_dword(media[65]+FSI_LeadSig)==0x41615252);
 assert(ld_dword(media[65]+FSI_StrucSig)==0x61417272 && ld_dword(media[65]+FSI_Free_Count)==123 && ld_dword(media[65]+FSI_Nxt_Free)==42);
 setup();fs.fs_type=FS_FAT32;fs.fsi_flag=1;fail_write=1;
 assert(sync_fs(&fs)==FR_DISK_ERR && fs.fsi_flag==1 && writes==1 && syncs==0 && fs.winsect==65);
 /* A separate explicit retry in this non-latching stub is distinct from auto replay. */
 fail_write=0;assert(sync_fs(&fs)==FR_OK && writes==2 && !fs.fsi_flag && syncs==1);
 for(unsigned flag=0;flag<=128;flag+=128){setup();fs.fs_type=FS_FAT32;fs.fsi_flag=flag;assert(sync_fs(&fs)==FR_OK && !writes && syncs==1 && fs.fsi_flag==flag);}
 setup();fs.fsi_flag=1;assert(sync_fs(&fs)==FR_OK && !writes && syncs==1 && fs.fsi_flag==1);
 setup();sync_result=RES_ERROR;assert(sync_fs(&fs)==FR_DISK_ERR && syncs==1);
 setup();fs.wflag=1;fs.winsect=9;fail_write=2;fs.fs_type=FS_FAT32;fs.fsi_flag=1;
 assert(sync_fs(&fs)==FR_DISK_ERR && writes==2 && fs.wflag && fs.fsi_flag && !syncs);
 puts("PASS actual sync_fs: exact FSInfo layout, write/error/pending flag, explicit retry distinction, no-update/other FAT, CTRL_SYNC and no success masking");
}
static void public_sync(void)
{
 for(unsigned kind=0;kind<5;kind++) {
  setup();file.flag=FA_WRITE|FA_MODIFIED;
  switch(kind){
  case 0:fs.winsect=9;fs.wflag=1;fail_write=1;break;
  case 1:fs.winsect=9;fs.wflag=1;fail_write=2;break;
  case 2:fs.fs_type=FS_FAT32;fs.fsi_flag=1;fail_write=2;break;
  case 3:sync_result=RES_ERROR;break;
  default:file.flag|=FA_DIRTY;file.sect=150;fail_write=1;break;
  }
  assert(f_sync(&file)==FR_DISK_ERR && file.err==FR_DISK_ERR && (file.flag&FA_MODIFIED));
  fail_write=0;sync_result=RES_OK;
  FATFS other=fs;other.wflag=0;other.fsi_flag=0;
  assert(sync_fs(&other)==FR_OK); /* Unrelated success cannot erase the file error. */
  unsigned before=writes,oldsync=syncs;
  assert(f_sync(&file)==FR_DISK_ERR && f_close(&file)==FR_DISK_ERR);
  assert(writes==before && syncs==oldsync && file.obj.fs==&fs);
#if _FS_LOCK
  assert(Files[0].fs==&fs && Files[0].ctr==1);
#endif
 }
 setup();file.flag=FA_WRITE|FA_MODIFIED;fs.fs_type=FS_FAT32;fs.fsi_flag=1;
 assert(f_sync(&file)==FR_OK && !(file.flag&FA_MODIFIED) && !file.err);
 assert(f_close(&file)==FR_OK && file.obj.fs==0);
 setup();file.flag=FA_WRITE|FA_MODIFIED;fs.fs_type=FS_FAT32;fs.fsi_flag=1;fail_write=2;
 assert(f_close(&file)==FR_DISK_ERR && file.err==FR_DISK_ERR && file.obj.fs==&fs);
 /* Same public failure path through the real S1 disk driver, not the RAM stub. */
 setup();disk.drv[0]=&SD_Driver;disk.is_initialized[0]=0;
 Stat=STA_NOINIT;Fault=0;physical_writes=physical_inits=0;physical_fail_write=2;
 card_present=SD_PRESENT;bsp_result=MSD_OK;assert(disk_initialize(0)==0);
 file.flag=FA_WRITE|FA_MODIFIED;fs.winsect=9;fs.wflag=1;
 assert(f_sync(&file)==FR_DISK_ERR && Fault && file.err==FR_DISK_ERR && physical_writes==2);
 assert(f_close(&file)==FR_INVALID_OBJECT && file.obj.fs==&fs && physical_writes==2);
 physical_fail_write=0;
 puts("PASS whole public sync/close through S1: mirror failure latches driver, first FR_DISK_ERR then invalid-object close, no extra physical writes");
 puts("PASS full production f_sync/f_close: primary/mirror/FSInfo/data/sync errors stay sticky, modified state and handle retained, no implicit replay with healthy stub; success closes normally");
}
#endif
static void initialization(void)
{
 setup();assert(disk_initialize(0)==0 && init_calls==1 && status_calls==1);
 available=STA_NOINIT;assert(disk_initialize(0)&STA_NOINIT);assert(!disk.is_initialized[0] && init_calls==1);
 assert(disk_initialize(0)&STA_NOINIT);assert(init_calls==2 && !disk.is_initialized[0]);
 available=STA_NOINIT|STA_NODISK;assert(disk_initialize(0)==available && !disk.is_initialized[0]);
 available=STA_NODISK;assert(disk_initialize(0)==(STA_NOINIT|STA_NODISK) && !disk.is_initialized[0]);
 available=0;assert(disk_initialize(0)==0 && disk.is_initialized[0]);
 available=STA_PROTECT;assert(disk_initialize(0)==STA_PROTECT && disk.is_initialized[0]);
 available=0;unsigned i=init_calls;assert(FATFS_UnLinkDriver(drive)==0);
 assert(disk_initialize(0)&STA_NOINIT);assert(FATFS_LinkDriver(&fake_driver,drive)==0);
 assert(!disk.is_initialized[0] && disk_initialize(0)==0 && init_calls==i+1);
 for(unsigned n=1;n<256;n++)assert(disk_initialize((BYTE)n)&STA_NOINIT);
 /* Actual S1 driver behind the actual generic wrapper and registration code. */
 memset(&disk,0,sizeof disk);Stat=STA_NOINIT;Fault=0;physical_writes=physical_inits=tick=0;
 card_present=SD_NOT_PRESENT;bsp_result=MSD_OK;physical_fail_write=0;assert(FATFS_LinkDriver(&SD_Driver,drive)==0);
 assert(disk_initialize(0)==(STA_NOINIT|STA_NODISK) && !physical_inits && !disk.is_initialized[0]);
 card_present=SD_PRESENT;assert(disk_initialize(0)==0 && physical_inits==1 && disk.is_initialized[0]);
 BYTE buffer[512]={0};bsp_result=MSD_ERROR;
 assert(disk_write(0,buffer,0,1)==RES_ERROR && physical_writes==1 && Fault);
 assert(disk_initialize(0)&STA_NOINIT);assert(!disk.is_initialized[0]);
 for(unsigned n=0;n<10;n++)assert(disk_initialize(0)&STA_NOINIT);
 assert(FATFS_UnLinkDriver(drive)==0 && FATFS_LinkDriver(&SD_Driver,drive)==0);
 assert(disk_initialize(0)&STA_NOINIT);
 assert(disk_write(0,buffer,0,1)==RES_NOTRDY && physical_writes==1 && physical_inits==1);
 puts("PASS whole generic disk/registration/S1: successful/failed/cache status, absent then available, protected, healthy unlink/relink and all invalid drive bytes; serious SD latch survives init/remount with no physical replay");
}
static void fixture_put16(BYTE *p,WORD value){p[0]=(BYTE)value;p[1]=(BYTE)(value>>8);}
static void mount_lifecycle(void)
{
 setup();FATFS mounted;memset(&mounted,0,sizeof mounted);
 /* Minimal RAM FAT16 boot sector; no format/create operation is invoked. */
 BYTE *boot=media[0];boot[0]=0xeb;boot[1]=0x3c;boot[2]=0x90;
 memcpy(boot+BS_FilSysType,"FAT16   ",8);
 fixture_put16(boot+BPB_BytsPerSec,512);boot[BPB_SecPerClus]=1;
 fixture_put16(boot+BPB_RsvdSecCnt,1);boot[BPB_NumFATs]=2;
 fixture_put16(boot+BPB_RootEntCnt,512);fixture_put16(boot+BPB_TotSec16,8192);
 fixture_put16(boot+BPB_FATSz16,32);boot[BPB_Media]=0xf8;fixture_put16(boot+BS_55AA,0xaa55);
 assert(FATFS_UnLinkDriver(drive)==0 && FATFS_LinkDriver(&fake_driver,drive)==0);
 unsigned before=init_calls;
 assert(f_mount(&mounted,drive,1)==FR_OK && init_calls==before+1);
 assert(mounted.fs_type==FS_FAT16 && mounted.n_fats==2 && mounted.fatbase==1 && mounted.database==97);
 assert(f_mount(0,drive,0)==FR_OK && f_mount(&mounted,drive,1)==FR_OK && init_calls==before+1);
 assert(f_mount(0,drive,0)==FR_OK);
 assert(FATFS_UnLinkDriver(drive)==0 && FATFS_LinkDriver(&fake_driver,drive)==0);
 available=STA_NOINIT|STA_NODISK;
 assert(f_mount(&mounted,drive,1)==FR_NOT_READY && !disk.is_initialized[0]);
 available=0;assert(f_mount(&mounted,drive,1)==FR_OK && disk.is_initialized[0]);
 assert(f_mount(0,drive,0)==FR_OK);
 puts("PASS actual f_mount: valid RAM FAT16 geometry, healthy repeated mount/cache status, absent then available without fabricated initialization");
}
static void read_only_operation(void)
{
 setup();BYTE data[512];UINT count;
 for(unsigned n=0;n<512;n++)media[128][n]=(BYTE)n;
 assert(f_read(&file,data,512,&count)==FR_OK && count==512 && !memcmp(data,media[128],512));
 assert(reads==1 && !writes && f_close(&file)==FR_OK && !writes);
 puts("PASS actual f_read and read-only file close: payload unchanged, no metadata writes");
}
int main(void)
{
#if !_FS_READONLY
 mirrors();fsinfo();public_sync();
#endif
 initialization();mount_lifecycle();read_only_operation();
}
