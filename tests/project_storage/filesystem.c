/* Whole production FatFs/generic disk/S1 fixture, synthetic RAM FAT16 only. */
#define main previous_integrity_main
#include "tests/fatfs_integrity/host.c"
#undef main
#define NUM_KIT_SLOTS 64
#include "gate.inc"
static void seed(void)
{
 setup();memset(Files,0,sizeof Files);
 BYTE *b=media[0];b[0]=0xeb;b[1]=0x3c;b[2]=0x90;memcpy(b+BS_FilSysType,"FAT16   ",8);
 fixture_put16(b+BPB_BytsPerSec,512);b[BPB_SecPerClus]=1;fixture_put16(b+BPB_RsvdSecCnt,1);b[BPB_NumFATs]=2;
 fixture_put16(b+BPB_RootEntCnt,512);fixture_put16(b+BPB_TotSec16,8192);fixture_put16(b+BPB_FATSz16,32);
 b[BPB_Media]=0xf8;fixture_put16(b+BS_55AA,0xaa55);
 media[1][0]=0xf8;media[1][1]=media[1][2]=media[1][3]=0xff;
 fixture_put16(media[1]+4,0xffff);fixture_put16(media[1]+6,0xffff);memcpy(media[33],media[1],512);
 memcpy(media[65],"PROJ00     ",11);media[65][DIR_Attr]=AM_DIR;fixture_put16(media[65]+DIR_FstClusLO,2);
 memcpy(media[97],"PRJ     NFO",11);media[97][DIR_Attr]=AM_ARC;fixture_put16(media[97]+DIR_FstClusLO,3);
 media[97][DIR_FileSize]=8;memcpy(media[98],"OLDVALID",8);
}
int main(void)
{
 seed();FATFS mounted;memset(&mounted,0,sizeof mounted);assert(f_mount(&mounted,drive,1)==FR_OK);
 static BYTE original[sizeof media];memcpy(original,media,sizeof media);
 char folder[8],path[21];assert(preset_projectPath(folder,sizeof folder,0)==FR_OK);
 for(unsigned i=0;i<10;i++)assert(preset_createProjectFolder(folder)==FR_EXIST);
 assert(writes==0 && !memcmp(original,media,sizeof media));
 assert(preset_childPath(path,sizeof path,folder,"prj.nfo")==FR_OK);
 FIL f;memset(&f,0,sizeof f);BYTE data[8];UINT count;
 assert(f_open(&f,path,FA_READ)==FR_OK && f_read(&f,data,8,&count)==FR_OK && count==8 && !memcmp(data,"OLDVALID",8) && f_close(&f)==FR_OK);
 assert(writes==0 && !memcmp(original,media,sizeof media));
 assert(preset_projectPath(folder,sizeof folder,63)==FR_OK && preset_createProjectFolder(folder)==FR_OK);
 FILINFO info;assert(f_stat(folder,&info)==FR_OK && (info.fattrib&AM_DIR));
 assert(preset_createProjectFolder(folder)==FR_EXIST);
 assert(!memcmp(media[98],"OLDVALID",8));
 puts("PASS whole production FatFs: ten overwrite rejections perform zero RAM-sector writes; previous metadata still readable; absent slot63 creation succeeds without deleting old project");
 return 0;
}
