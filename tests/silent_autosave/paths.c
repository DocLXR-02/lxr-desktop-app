/* Full production FatFs; RAM-only geometry. Paths are extracted from actual writers. */
#define main earlier_integrity_main
#include "../fatfs_integrity/host.c"
#undef main
#include "paths.inc"
static void dir_entry(BYTE *p,const char *name,BYTE attr,WORD cluster,DWORD size)
{
 memcpy(p,name,11);p[DIR_Attr]=attr;fixture_put16(p+DIR_FstClusLO,cluster);
 p[DIR_FileSize]=(BYTE)size;
}
int main(void)
{
 setup();memset(Files,0,sizeof Files);FATFS mounted;memset(&mounted,0,sizeof mounted);
 BYTE *boot=media[0];boot[0]=0xeb;boot[1]=0x3c;boot[2]=0x90;memcpy(boot+BS_FilSysType,"FAT16   ",8);
 fixture_put16(boot+BPB_BytsPerSec,512);boot[BPB_SecPerClus]=1;fixture_put16(boot+BPB_RsvdSecCnt,1);
 boot[BPB_NumFATs]=2;fixture_put16(boot+BPB_RootEntCnt,512);fixture_put16(boot+BPB_TotSec16,8192);
 fixture_put16(boot+BPB_FATSz16,32);boot[BPB_Media]=0xf8;fixture_put16(boot+BS_55AA,0xaa55);
 media[1][0]=0xf8;media[1][1]=media[1][2]=media[1][3]=0xff;
 for(unsigned c=2;c<=6;c++)if(c!=4)fixture_put16(media[1]+2*c,0xffff);
 memcpy(media[33],media[1],512);
 dir_entry(media[65],"TMP        ",AM_DIR,2,0);dir_entry(media[65]+32,"PROJ       ",AM_DIR,3,0);
 dir_entry(media[98],"TMP     SND",AM_ARC,5,8);dir_entry(media[98]+32,"TMP     PAT",AM_ARC,6,8);
 memcpy(media[100],"kit kept",8);memcpy(media[101],"pat kept",8);
 assert(FATFS_UnLinkDriver(drive)==0 && FATFS_LinkDriver(&fake_driver,drive)==0);
 assert(f_mount(&mounted,drive,1)==FR_OK && f_chdir("/proj")==FR_OK);
 BYTE directory[512];memcpy(directory,media[98],512);
 for(unsigned kind=0;kind<2;kind++){
  FIL f;memset(&f,0,sizeof f);UINT n;
  assert(f_open(&f,actual_autosave_paths[kind],FA_CREATE_ALWAYS|FA_WRITE)==FR_OK);
  assert(f_write(&f,"new temp",8,&n)==FR_OK && n==8 && f_close(&f)==FR_OK);
  assert(mounted.cdir==3 && !memcmp(directory,media[98],512));
  assert(f_open(&f,kind?"tmp.pat":"tmp.snd",FA_READ)==FR_OK);char data[8];
  assert(f_read(&f,data,8,&n)==FR_OK && n==8 && !memcmp(data,kind?"pat kept":"kit kept",8));
  assert(f_close(&f)==FR_OK);
 }
 puts("PASS complete production FatFs path resolution: actual autosave paths stay under /tmp while CWD=/proj; current directory, project entries and existing kit/pattern bytes preserved");
}
