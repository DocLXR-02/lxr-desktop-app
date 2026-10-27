/* Full production FatFs/disk/S1 units via the established fixture, RAM sectors only. */
#define main previous_integrity_main
#include "../fatfs_integrity/host.c"
#undef main
static BYTE saved_media[sizeof media];
int main(void)
{
 setup();
 /* Drop only the fixture's artificial pre-opened lock before mounting RAM media. */
 memset(Files,0,sizeof Files);
 FATFS mounted;memset(&mounted,0,sizeof mounted);
 BYTE *boot=media[0];boot[0]=0xeb;boot[1]=0x3c;boot[2]=0x90;
 memcpy(boot+BS_FilSysType,"FAT16   ",8);
 fixture_put16(boot+BPB_BytsPerSec,512);boot[BPB_SecPerClus]=1;
 fixture_put16(boot+BPB_RsvdSecCnt,1);boot[BPB_NumFATs]=2;
 fixture_put16(boot+BPB_RootEntCnt,512);fixture_put16(boot+BPB_TotSec16,8192);
 fixture_put16(boot+BPB_FATSz16,32);boot[BPB_Media]=0xf8;fixture_put16(boot+BS_55AA,0xaa55);
 media[1][0]=media[33][0]=0xf8;media[1][1]=media[1][2]=media[1][3]=0xff;
 memcpy(media[33],media[1],512);
 assert(FATFS_UnLinkDriver(drive)==0 && FATFS_LinkDriver(&fake_driver,drive)==0);
 assert(f_mount(&mounted,drive,1)==FR_OK);
 FIL save;memset(&save,0,sizeof save);UINT count;
 const char payload[]="Previously valid kit bytes";
 assert(f_open(&save,"GOOD.SND",FA_CREATE_ALWAYS|FA_WRITE)==FR_OK);
 assert(f_write(&save,payload,sizeof payload,&count)==FR_OK && count==sizeof payload);
 assert(f_close(&save)==FR_OK);
 memcpy(saved_media,media,sizeof media);unsigned original_writes=writes;
 for(unsigned attempt=0;attempt<9;attempt++){
  assert(f_open(&save,"MISSING/GOOD.SND",FA_CREATE_ALWAYS|FA_WRITE)==FR_NO_PATH && !save.obj.fs);
  assert(f_open(&save,"BAD|NAME.SND",FA_CREATE_ALWAYS|FA_WRITE)==FR_INVALID_NAME && !save.obj.fs);
  assert(f_open(&save,"GOOD.SND",FA_CREATE_NEW|FA_WRITE)==FR_EXIST && !save.obj.fs);
 }
 /* Read-only overwrite denial is before truncation in configured f_open. */
 media[65][DIR_Attr]|=AM_RDO;mounted.winsect=0xffffffffu;
 for(unsigned n=0;n<9;n++)assert(f_open(&save,"GOOD.SND",FA_CREATE_ALWAYS|FA_WRITE)==FR_DENIED && !save.obj.fs);
 media[65][DIR_Attr]&=(BYTE)~AM_RDO;mounted.winsect=0xffffffffu;
 assert(writes==original_writes && !memcmp(saved_media,media,sizeof media));
 assert(f_open(&save,"GOOD.SND",FA_READ)==FR_OK);char actual[sizeof payload];
 assert(f_read(&save,actual,sizeof actual,&count)==FR_OK && count==sizeof payload && !memcmp(payload,actual,sizeof payload));
 assert(f_close(&save)==FR_OK);
 assert(f_open(&save,"NEW.SND",FA_CREATE_ALWAYS|FA_WRITE)==FR_OK);
 assert(f_write(&save,payload,sizeof payload,&count)==FR_OK && count==sizeof payload);
 assert(f_close(&save)==FR_OK);
 /* Full static SFN root, fixture bytes only; no format/recovery operation. */
 memset(media[65],0x41,32*512);mounted.winsect=0xffffffffu;
 memcpy(saved_media,media,sizeof media);original_writes=writes;
 for(unsigned n=0;n<9;n++)assert(f_open(&save,"FULL.SND",FA_CREATE_ALWAYS|FA_WRITE)==FR_DENIED && !save.obj.fs);
 assert(writes==original_writes && !memcmp(saved_media,media,sizeof media));
 puts("PASS complete production FatFs: nine missing-path/invalid-name/existing-file/readonly-target/full-root failures make zero disk writes, preserve all RAM media/valid file bytes, then explicit create/write/close succeeds");
}
