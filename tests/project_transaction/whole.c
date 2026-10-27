/* Real production FatFs/generic disk/registration and extracted project code.
 * HAL/LCD/sound sends and physical sectors are mocked. No card or hardware. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <setjmp.h>
#define _FF_INTEGER
typedef int32_t INT,LONG;typedef uint32_t UINT,DWORD;typedef uint8_t BYTE;
typedef int16_t SHORT;typedef uint16_t WORD,WCHAR;typedef uint64_t QWORD;
#include "Preset/PresetManager.h"
#include "Soundengine/Sequencer/sequencer.h"
#include "menu/menu.h"
#include "lcd.h"
#include "frontPanelParser_ui.h"
#include "Soundengine/DSPAudio/automationNode.h"
#include "Middlewares/Third_Party/FatFs/src/ff.c"
#include "Middlewares/Third_Party/FatFs/src/diskio.c"
#include "Middlewares/Third_Party/FatFs/src/ff_gen_drv.c"
#define STEPS_PER_PATTERN NUM_STEPS
#define FEXT_SOUND 0
#define FEXT_PAT 1
#define FEXT_ALL 2
#define FEXT_PROJ 3
#define FEXT_SONG 4
PatternSet seq_patternSet;
PatternBackupBuffer patternBackupBuffer;
uint8_t mainParameters[64][END_OF_SOUND_PARAMETERS],activeParameters[NUM_PARAMS];
uint8_t *morphParameters,*seq_activeSong;
uint8_t seq_songData[64][MAX_SONG_LENGTH];
uint8_t seq_activePattern,seq_pendingPattern;
uint8_t menu_selectedPresetNr[NUM_PRESET_LOCATIONS],menu_loadedPresetNr[NUM_CURRENT];
uint8_t menu_playedPattern,menu_shownPattern,lcd_process_display_interrupt;
char currentFolder[13],savefileNames[64][NUM_SAVEFILE_NAMES][8];
FIL SDFile;
TIM_HandleTypeDef htim2;
static FRESULT preset_sharedFailure;
static uint8_t preset_autosavePaused;
static FATFS mounted;
static char drive[4];
static BYTE media[70000][512],saved[sizeof media];
static unsigned physical_writes,physical_reads,sync_calls,write_failure,read_failure,sync_failure;
static unsigned power_cut,rename_calls,unlink_calls,close_calls,open_calls,close_failure,open_failure;
static int power_after,fail_applies,driver_fault;
static FRESULT injected_result;
static jmp_buf power;
enum { EVENT_HEADER=1,EVENT_GLOBAL,EVENT_KIT,EVENT_SONG,EVENT_METADATA,EVENT_SEAL,
       EVENT_OLD_RENAME,EVENT_NEW_RENAME,EVENT_BACKUP_FILE,EVENT_BACKUP_DIR,EVENT_RECORD,EVENT_RESTORE };
static unsigned event_cut,event_after,active_rename,rename_cut,rename_sector,rename_base;
static unsigned rename_write_counts[3],logical_denials,corrupt_stage,short_write,api_writes,fail_sync_rename;
static unsigned rename_failure,unlink_failure,dir_close_failure,dir_closes;
static char current_path[32];static BYTE current_mode;
static uint32_t expected_old,expected_new;
static unsigned cases;
static void event(unsigned id,unsigned after)
{if(event_cut==id && event_after==after)longjmp(power,1);}
static DSTATUS ram_init(BYTE n){(void)n;return driver_fault?STA_NOINIT:0;}
static DSTATUS ram_status(BYTE n){return ram_init(n);}
static DRESULT ram_read(BYTE n,BYTE *p,DWORD sector,UINT count)
{
 (void)n;physical_reads++;assert(sector+count<=70000);
 if(driver_fault || physical_reads==read_failure){driver_fault=1;return RES_ERROR;}
 memcpy(p,media[sector],count*512);return RES_OK;
}
static DRESULT ram_write(BYTE n,const BYTE *p,DWORD sector,UINT count)
{
 (void)n;assert(count==1 && sector+count<=70000);if(driver_fault)return RES_ERROR;physical_writes++;
 if(active_rename==rename_cut && rename_cut && physical_writes-rename_base==rename_sector && !power_after)longjmp(power,1);
 if(physical_writes==power_cut&&!power_after)longjmp(power,1);
 if(physical_writes!=write_failure || fail_applies)memcpy(media[sector],p,count*512);
 if(physical_writes==power_cut&&power_after)longjmp(power,1);
 if(active_rename==rename_cut && rename_cut && physical_writes-rename_base==rename_sector && power_after)longjmp(power,1);
 if(physical_writes==write_failure){driver_fault=1;return RES_ERROR;}return RES_OK;
}
static DRESULT ram_ioctl(BYTE n,BYTE command,void *buffer)
{
 (void)n;(void)buffer;assert(command==CTRL_SYNC);sync_calls++;
 if(driver_fault || sync_calls==sync_failure || (active_rename && active_rename==fail_sync_rename)){driver_fault=1;return RES_ERROR;}return RES_OK;
}
static const Diskio_drvTypeDef ram_driver={ram_init,ram_status,ram_read,ram_write,ram_ioctl};
/* Inject at the public API boundary while retaining real production internals. */
static void alter_stage(void)
{
 FIL f={0};UINT got;FRESULT r;
 if(corrupt_stage==1){r=f_open(&f,"/LXRSTG/00-PATTE.pat",FA_WRITE);assert(r==FR_OK);assert(f_lseek(&f,100)==FR_OK && f_truncate(&f)==FR_OK);}
 else if(corrupt_stage==2){r=f_open(&f,"/LXRSTG/00-PATTE.pat",FA_WRITE);assert(r==FR_OK);BYTE bad=255;assert(f_lseek(&f,3662)==FR_OK && f_write(&f,&bad,1,&got)==FR_OK && got==1);}
 else if(corrupt_stage==3){assert(f_open(&f,"/LXRSTG/README.TXT",FA_CREATE_NEW|FA_WRITE)==FR_OK);assert(f_write(&f,"unknown",7,&got)==FR_OK && got==7);}
 else assert(0);
 assert(f_close(&f)==FR_OK);corrupt_stage=0;
}
static FRESULT host_close(FIL *f)
{
 unsigned id=0;
 if(current_mode&FA_WRITE){
  if(!strcmp(current_path,"/LXRTRANS.DAT"))id=f_size(f)==32?EVENT_HEADER:EVENT_SEAL;
  else if(!strcmp(current_path,"glo.cfg"))id=EVENT_GLOBAL;
  else if(!strcmp(current_path,"00-KITNA.snd"))id=EVENT_KIT;
  else if(!strcmp(current_path,"63-SONGN.sng"))id=EVENT_SONG;
  else if(!strcmp(current_path,"prj.nfo"))id=EVENT_METADATA;
 }
 if(id)event(id,0);close_calls++;
 FRESULT r=close_calls==close_failure?injected_result:f_close(f);
 if(r==FR_OK && id){event(id,1);if(id==EVENT_METADATA && corrupt_stage)alter_stage();}
 return r;
}
static FRESULT host_open(FIL *f,const TCHAR *p,BYTE mode)
{
 open_calls++;assert(strlen(p)<sizeof current_path);strcpy(current_path,p);current_mode=mode;
 if(logical_denials && !strcmp(p,"/LXRTRANS.DAT") && (mode&FA_CREATE_NEW)){logical_denials--;return FR_DENIED;}
 return open_calls==open_failure?injected_result:f_open(f,p,mode);
}
static FRESULT host_write(FIL *f,const void *p,UINT count,UINT *actual)
{api_writes++;if(api_writes==short_write && count)count--;return f_write(f,p,count,actual);}
static FRESULT host_rename(const TCHAR *a,const TCHAR *b)
{
 unsigned id=!strcmp(a,"/LXRSTG")?EVENT_NEW_RENAME:!strcmp(a,"/LXRBAK")?EVENT_RESTORE:EVENT_OLD_RENAME;
 event(id,0);rename_calls++;active_rename=id==EVENT_OLD_RENAME?1:id==EVENT_NEW_RENAME?2:3;rename_base=physical_writes;
 FRESULT r=rename_calls==rename_failure?injected_result:f_rename(a,b);rename_write_counts[active_rename-1]=physical_writes-rename_base;active_rename=0;
 if(r==FR_OK)event(id,1);return r;
}
static FRESULT host_unlink(const TCHAR *p)
{
 unsigned id=!strcasecmp(p,"/LXRBAK/prj.nfo")?EVENT_BACKUP_FILE:!strcmp(p,"/LXRBAK")?EVENT_BACKUP_DIR:!strcmp(p,"/LXRTRANS.DAT")?EVENT_RECORD:0;
 if(id)event(id,0);unlink_calls++;FRESULT r=unlink_calls==unlink_failure?injected_result:f_unlink(p);if(r==FR_OK && id)event(id,1);return r;
}
static FRESULT host_closedir(DIR *d){dir_closes++;return dir_closes==dir_close_failure?injected_result:f_closedir(d);}
#define f_close host_close
#define f_open host_open
#define f_rename host_rename
#define f_unlink host_unlink
#define f_write host_write
#define f_closedir host_closedir
HAL_StatusTypeDef HAL_TIM_Base_Start_IT(TIM_HandleTypeDef *t){(void)t;return HAL_OK;}
HAL_StatusTypeDef HAL_TIM_Base_Stop_IT(TIM_HandleTypeDef *t){(void)t;return HAL_OK;}
void soundengine_process(void){}
void preset_sendDrumsetParameters(void){}
void preset_sendDrumsetParametersImmediate(void){}
void menu_sendAllGlobals(void){}
uint8_t getNumModTargets(void){return 16;}
uint8_t sendDisplayBuffer(void){return 0;}
void lcd_clear(void){}void lcd_home(void){}void lcd_string(const char *p){(void)p;}
void lcd_setcursor(uint8_t x,uint8_t y){(void)x;(void)y;}
void menu_repaintAll(void){}
void menu_showSdError(uint8_t e){(void)e;assert(!"no notification in transaction engine");}
void frontPanel_sendData(uint8_t a,uint8_t b,uint8_t c){(void)a;(void)b;(void)c;}
void seq_setNextPattern(const uint8_t n){assert(n<64);seq_pendingPattern=n;}
void seq_copyPattern(uint8_t a,uint8_t b){(void)a;(void)b;assert(!"active-slot capture expected");}
#include "functions.inc"
#undef f_close
#undef f_open
#undef f_rename
#undef f_unlink
#undef f_write
#undef f_closedir

static void put16(BYTE *p,WORD n){p[0]=n;p[1]=n>>8;}
static void put32(BYTE *p,DWORD n){preset_txnPut32(p,n);}
static void reboot(void)
{
 memset(&disk,0,sizeof disk);memset(Files,0,sizeof Files);memset(&mounted,0,sizeof mounted);memset(&SDFile,0,sizeof SDFile);
 preset_sharedFailure=FR_OK;preset_autosavePaused=preset_projectRecovered=0;driver_fault=0;
 physical_writes=physical_reads=sync_calls=write_failure=read_failure=sync_failure=0;
 power_cut=rename_calls=unlink_calls=close_calls=open_calls=close_failure=open_failure=0;
 event_cut=event_after=active_rename=rename_cut=rename_sector=logical_denials=corrupt_stage=short_write=api_writes=fail_sync_rename=0;
 rename_failure=unlink_failure=dir_close_failure=dir_closes=0;
 memset(rename_write_counts,0,sizeof rename_write_counts);current_path[0]=0;current_mode=0;
 injected_result=FR_DISK_ERR;assert(FATFS_LinkDriver(&ram_driver,drive)==0);assert(f_mount(&mounted,drive,1)==FR_OK);
}
static void ram_contents(const char *label,unsigned value)
{
 memset(activeParameters,0,sizeof activeParameters);memset(mainParameters,0,sizeof mainParameters);memset(&seq_patternSet,0,sizeof seq_patternSet);
 memset(seq_songData,SONG_END,sizeof seq_songData);memset(savefileNames,0,sizeof savefileNames);
 seq_activePattern=seq_pendingPattern=menu_playedPattern=menu_shownPattern=0;memset(menu_selectedPresetNr,0,sizeof menu_selectedPresetNr);
 for(unsigned i=0;i<64;i++){
  memcpy(savefileNames[i][PROJECT_NAMES],label,8);memcpy(savefileNames[i][KIT_NAMES],"KITNAME!",8);
  memcpy(savefileNames[i][PATTERN_NAMES],"PATTERN!",8);memcpy(savefileNames[i][SONG_NAMES],"SONGNAME",8);
  for(unsigned t=0;t<7;t++)seq_patternSet.length[i][t]=16;
 }
 activeParameters[0]=value;morphParameters=mainParameters[0];seq_activeSong=seq_songData[0];
}
static void seed(unsigned fat32)
{
 memset(media,0,sizeof media);BYTE *b=media[0];b[0]=0xeb;b[1]=0x3c;b[2]=0x90;
 put16(b+BPB_BytsPerSec,512);b[BPB_SecPerClus]=1;b[BPB_NumFATs]=2;b[BPB_Media]=0xf8;put16(b+BS_55AA,0xaa55);
 if(fat32){
  memcpy(b+BS_FilSysType32,"FAT32   ",8);put16(b+BPB_RsvdSecCnt,32);put32(b+BPB_TotSec32,70000);
  put32(b+BPB_FATSz32,547);put32(b+BPB_RootClus32,2);put16(b+BPB_FSInfo32,1);
  put32(media[32],0x0ffffff8);put32(media[32]+4,0xffffffff);put32(media[32]+8,0x0fffffff);memcpy(media[579],media[32],512);
 }else{
  memcpy(b+BS_FilSysType,"FAT16   ",8);put16(b+BPB_RsvdSecCnt,1);put16(b+BPB_RootEntCnt,512);put16(b+BPB_TotSec16,8192);put16(b+BPB_FATSz16,32);
  media[1][0]=0xf8;media[1][1]=media[1][2]=media[1][3]=0xff;memcpy(media[33],media[1],512);
 }
 reboot();assert(f_mkdir("/tmp")==FR_OK);assert(f_chdir("/tmp")==FR_OK);
 ram_contents("OLDVALID",0);
}
static void no_transaction(void)
{FILINFO i;assert(f_stat(PROJECT_STAGE,&i)==FR_NO_FILE && f_stat(PROJECT_BACKUP,&i)==FR_NO_FILE && f_stat(PROJECT_RECORD,&i)==FR_NO_FILE);}
static void name_is(const char *folder,const char *name)
{char path[32];snprintf(path,sizeof path,"%s/prj.nfo",folder);FIL f={0};BYTE data[8];UINT got;assert(f_open(&f,path,FA_READ)==FR_OK);assert(f_read(&f,data,8,&got)==FR_OK&&got==8&&!memcmp(data,name,8));assert(f_close(&f)==FR_OK);}
static void normal(void)
{
 assert((preset_txnCrc(0xffffffffu,"123456789",9)^0xffffffffu)==0xcbf43926u);
 for(unsigned kind=0;kind<2;kind++){
  seed(kind);assert(preset_saveProject(0)==FR_OK);name_is("/proj00","OLDVALID");no_transaction();
  DWORD initial_free;FATFS *p;assert(f_getfree("0:",&initial_free,&p)==FR_OK);
  for(unsigned n=0;n<20;n++){
   memcpy(savefileNames[0][PROJECT_NAMES],"NEWVALID",8);activeParameters[0]=n+10;
   assert(preset_saveProject(0)==FR_OK);name_is("/proj00","NEWVALID");no_transaction();
   uint32_t root,hash;uint16_t count;assert(preset_txnValidate("/proj00",1,&root,&hash,&count)==FR_OK && count==194);
   DWORD available;assert(f_getfree("0:",&available,&p)==FR_OK && available==initial_free);
  }
  assert(preset_loadProject(0,0)==FR_OK);
 }
 puts("PASS whole production project/FatFs FAT16+FAT32: new save +20 replacements each, exact194-file layout, constant free clusters, no stage/backup/record accumulation, valid load and CRC vector");
}
static uint32_t fingerprint(const char *folder)
{
 uint32_t root,hash;uint16_t count;
 assert(preset_txnValidate(folder,1,&root,&hash,&count)==FR_OK && count==194);return hash;
}
static void fill_root(void)
{
 for(unsigned i=0;i<40;i++){
  char name[24];snprintf(name,sizeof name,"/H%07u.TXT",i);FIL f={0};
  assert(f_open(&f,name,FA_CREATE_NEW|FA_WRITE)==FR_OK && f_close(&f)==FR_OK);
 }
}
static void save_fixture(unsigned fat32)
{
 seed(fat32);assert(preset_saveProject(0)==FR_OK);no_transaction();fill_root();
 expected_old=fingerprint("/proj00");memcpy(saved,media,sizeof media);
}
static void restore(void)
{memcpy(media,saved,sizeof media);reboot();assert(f_chdir("/tmp")==FR_OK);ram_contents("NEWVALID",77);}
static void good_after_reboot(void)
{
 reboot();char folder[8];assert(preset_txnLoadFolder(0,folder)==FR_OK);
 uint32_t found=fingerprint(folder);assert(found==expected_old || found==expected_new);
 assert(!physical_writes);
 if(preset_projectRecoveryActive()){
  assert(preset_loadProject(0,1)==FR_OK && !physical_writes);
  assert(!memcmp(savefileNames[0][PROJECT_NAMES],found==expected_old?"OLDVALID":"NEWVALID",8));
 }
 cases++;
}
static void failure_result(FRESULT result)
{
 assert(result!=FR_OK);
 if(preset_sharedFailure!=FR_OK){unsigned before=physical_writes;assert(preset_saveProject(0)!=FR_OK && preset_saveGlobals()!=FR_OK && before==physical_writes);}
 good_after_reboot();
}
static void faults(void)
{
 cases=0;
 for(unsigned fat32=0;fat32<2;fat32++){
  save_fixture(fat32);restore();assert(preset_saveProject(0)==FR_OK);
  unsigned total_writes=physical_writes,total_syncs=sync_calls,total_opens=open_calls,total_closes=close_calls,total_reads=physical_reads;
  printf("Profile FAT%u: %u sector writes, %u reads, %u syncs, %u opens, %u closes\n",fat32?32:16,total_writes,total_reads,total_syncs,total_opens,total_closes);
  expected_new=fingerprint("/proj00");
  const unsigned disk_points[]={1,2,3,10,100,500,1000};
  for(unsigned j=0;j<sizeof disk_points/sizeof *disk_points;j++)for(fail_applies=0;fail_applies<=1;fail_applies++){
   assert(disk_points[j]<=total_writes);restore();write_failure=disk_points[j];failure_result(preset_saveProject(0));
  }
  restore();write_failure=total_writes;failure_result(preset_saveProject(0));
  const unsigned file_points[]={1,2,194,195,196,197,200,300,390,500};
  for(unsigned j=0;j<sizeof file_points/sizeof *file_points;j++){
   assert(file_points[j]<=total_opens && file_points[j]<=total_closes);
   restore();open_failure=file_points[j];failure_result(preset_saveProject(0));
   restore();close_failure=file_points[j];failure_result(preset_saveProject(0));
  }
  restore();open_failure=total_opens;failure_result(preset_saveProject(0));
  restore();close_failure=total_closes;failure_result(preset_saveProject(0));
  const unsigned positions[]={1,2,10,100,500};
  for(unsigned j=0;j<sizeof positions/sizeof *positions;j++){
   assert(positions[j]<=total_reads);
   restore();read_failure=physical_reads+positions[j];failure_result(preset_saveProject(0));
  }
  unsigned sync_positions[]={1,2,10,100,total_syncs};
  for(unsigned j=0;j<5;j++){restore();sync_failure=sync_positions[j];failure_result(preset_saveProject(0));}
  for(unsigned n=1;n<=3;n++){
   restore();corrupt_stage=n;FRESULT r=preset_saveProject(0);assert(r!=FR_OK && rename_calls==0);failure_result(r);
  }
  for(unsigned n=1;n<=2;n++){
   restore();fail_sync_rename=n;failure_result(preset_saveProject(0));
   restore();rename_failure=n;injected_result=FR_DENIED;assert(preset_saveProject(0)==FR_DENIED);
   assert(!preset_sharedFailure);rename_failure=0;assert(preset_saveProject(0)==FR_OK);no_transaction();
   assert(fingerprint("/proj00")==expected_new);cases++;
  }
  restore();logical_denials=9;unsigned before=physical_writes;
  for(unsigned n=0;n<9;n++)assert(preset_saveProject(0)==FR_DENIED && physical_writes==before && !preset_sharedFailure);
  assert(preset_saveProject(0)==FR_OK);no_transaction();cases++;
  restore();short_write=1;failure_result(preset_saveProject(0));
  restore();short_write=100;failure_result(preset_saveProject(0));
  restore();dir_close_failure=2;failure_result(preset_saveProject(0));
  restore();unlink_failure=1;injected_result=FR_DENIED;assert(preset_saveProject(0)==FR_DENIED && fingerprint("/proj00")==expected_new);
  unlink_failure=0;assert(preset_saveProject(0)==FR_OK);no_transaction();cases++;
 }
 printf("PASS %u whole-FatFs fault/retry cases: writes before/after acceptance, reads/opens/closes/sync, partial writes, bad staged size/kit/unknown contents, rename/cleanup errors, nine safe failures then success; no false success or unsafe replay\n",cases);
}
static void boundaries(void)
{
 cases=0;
 for(unsigned fat32=0;fat32<2;fat32++){
  save_fixture(fat32);restore();assert(preset_saveProject(0)==FR_OK);expected_new=fingerprint("/proj00");
  for(unsigned id=EVENT_HEADER;id<=EVENT_RECORD;id++)for(unsigned after=0;after<=1;after++){
   restore();event_cut=id;event_after=after;
   if(!setjmp(power)){(void)preset_saveProject(0);assert(!"required logical boundary not reached");}
   good_after_reboot();
  }
  /* Repeated interruption while a user Save restores a validated old reference. */
  restore();event_cut=EVENT_OLD_RENAME;event_after=1;
  if(!setjmp(power)){(void)preset_saveProject(0);assert(0);}
  memcpy(media,saved,sizeof media); /* Build the gap again without changing old fixture. */
  reboot();ram_contents("NEWVALID",77);event_cut=EVENT_OLD_RENAME;event_after=1;
  if(!setjmp(power)){(void)preset_saveProject(0);assert(0);}
  for(unsigned n=0;n<3;n++){
   reboot();ram_contents("NEWVALID",77);event_cut=EVENT_RESTORE;event_after=0;
   if(!setjmp(power)){(void)preset_saveProject(0);assert(0);}
   good_after_reboot();
  }
  reboot();ram_contents("NEWVALID",77);assert(preset_saveProject(0)==FR_OK);no_transaction();assert(fingerprint("/proj00")==expected_new);
 }
 printf("PASS %u logical before/after transaction cuts and repeated interrupted user recovery: complete old/new fingerprint retained; recovery load is read-only\n",cases);
}
static void sector(void)
{
 cases=0;unsigned aliases=0;
 for(unsigned fat32=0;fat32<2;fat32++){
  save_fixture(fat32);restore();assert(preset_saveProject(0)==FR_OK);expected_new=fingerprint("/proj00");
  unsigned counts[2]={rename_write_counts[0],rename_write_counts[1]};assert(counts[0]>=2 && counts[1]>=2);
  for(unsigned phase=1;phase<=2;phase++)for(unsigned n=1;n<=counts[phase-1];n++)for(unsigned after=0;after<=1;after++){
   restore();rename_cut=phase;rename_sector=n;power_after=after;
   if(!setjmp(power)){(void)preset_saveProject(0);assert(!"rename sector cut not reached");}
   good_after_reboot();
   ProjectTransaction t;assert(preset_txnRead(&t)==FR_OK);char canonical[8];preset_projectPath(canonical,8,0);
   uint32_t c,s,b;assert(preset_txnIds(canonical,&c,&s,&b)==FR_OK);
   if((c && (c==s || c==b)) || (s && s==b)){
    unsigned before=physical_writes;ram_contents("NEWVALID",77);assert(preset_saveProject(0)==FR_INT_ERR && physical_writes==before);aliases++;
   }
  }
 }
 printf("PASS %u real FAT16/FAT32 cross-sector rename cuts (atomic512-byte writes), %u alias states detected and held without deletion; complete old/new retained\n",cases,aliases);
}
/* Logical edge cases use the same real volume, not a model filesystem. */
static void edge(void)
{
 cases=0;
 for(unsigned fat32=0;fat32<2;fat32++){
  save_fixture(fat32);restore();expected_new=0;
  /* Fill remaining space through FatFs; S1-style driver stays healthy. */
  FIL filler={0};UINT got;static BYTE bytes[512];
  assert(f_open(&filler,"/FULL.DAT",FA_CREATE_NEW|FA_WRITE)==FR_OK);
  do {assert(f_write(&filler,bytes,sizeof bytes,&got)==FR_OK);}while(got==sizeof bytes);
  assert(f_close(&filler)==FR_OK);unsigned before=physical_writes;
  assert(preset_saveProject(0)==FR_DENIED && physical_writes==before && !preset_sharedFailure);
  no_transaction();assert(fingerprint("/proj00")==expected_old);
  assert(f_unlink("/FULL.DAT")==FR_OK && preset_saveProject(0)==FR_OK);no_transaction();cases++;
  /* Unknown existing data is not transaction-owned and is never removed. */
  restore();FIL f={0};
  assert(f_open(&f,"/proj00/README.TXT",FA_CREATE_NEW|FA_WRITE)==FR_OK && f_write(&f,"unknown",7,&got)==FR_OK && got==7 && f_close(&f)==FR_OK);
  before=physical_writes;assert(preset_saveProject(0)==FR_DENIED && before==physical_writes);no_transaction();
  assert(f_open(&f,"/proj00/README.TXT",FA_READ)==FR_OK && f_size(&f)==7 && f_close(&f)==FR_OK);cases++;
  restore();assert(f_mkdir(PROJECT_STAGE)==FR_OK);before=physical_writes;
  assert(preset_saveProject(0)==FR_EXIST && before==physical_writes && fingerprint("/proj00")==expected_old);cases++;
  restore();assert(f_open(&f,PROJECT_RECORD,FA_CREATE_NEW|FA_WRITE)==FR_OK && f_close(&f)==FR_OK);
  before=physical_writes;assert(preset_saveProject(0)==FR_INT_ERR && before==physical_writes);
  char folder[8];assert(preset_txnLoadFolder(0,folder)==FR_OK && preset_projectRecoveryActive() && physical_writes==before);cases++;
  /* A failed recovery still disables boot tmp creation/overlay. */
  assert(f_unlink("/proj00/prj.nfo")==FR_OK);before=physical_writes;
  assert(preset_txnLoadFolder(0,folder)!=FR_OK && preset_projectRecoveryActive() && physical_writes==before);cases++;
  restore();before=physical_writes;
  for(unsigned n=64;n<256;n++)assert(preset_saveProject(n)==FR_INVALID_PARAMETER);
  assert(before==physical_writes);cases++;
  /* First-ever save has no previous project. Incomplete data cannot be loaded,
   * but a fully closed strict stage survives even before the seal is written. */
  const unsigned events[]={EVENT_HEADER,EVENT_GLOBAL,EVENT_METADATA,EVENT_SEAL,EVENT_NEW_RENAME,EVENT_RECORD};
  for(unsigned j=0;j<sizeof events/sizeof *events;j++)for(unsigned after=0;after<2;after++){
   seed(fat32);ram_contents("NEWVALID",77);event_cut=events[j];event_after=after;
   if(!setjmp(power)){(void)preset_saveProject(0);assert(!"first-save cut not reached");}
   reboot();FRESULT r=preset_txnLoadFolder(0,folder);
   if((events[j]==EVENT_METADATA && after) || events[j]>=EVENT_SEAL){
    assert(r==FR_OK);uint32_t hash=fingerprint(folder);
    assert(!physical_writes);
    /* Explicit Save can safely preserve/publish the sole complete stage. */
    ram_contents("NEWVALID",77);assert(preset_saveProject(0)==FR_OK && fingerprint("/proj00")==hash);no_transaction();
   }else{
    /* Before header close there may not yet be a durable ownership record. */
    if(r==FR_OK){FILINFO info;assert(f_stat("/proj00/prj.nfo",&info)!=FR_OK);}
    else assert(preset_projectRecoveryActive());
    assert(!physical_writes);
   }
   cases++;
  }
 }
 printf("PASS %u real-FatFs full-card/retry, unknown/collision/invalid record/index, failed-recovery overlay guard and first-save interruption/rescue cases\n",cases);
}
int main(int argc,char **argv)
{
 setvbuf(stdout,0,_IONBF,0);assert(argc==2);
 if(!strcmp(argv[1],"normal"))normal();else if(!strcmp(argv[1],"faults"))faults();else if(!strcmp(argv[1],"boundaries"))boundaries();else if(!strcmp(argv[1],"sector"))sector();else if(!strcmp(argv[1],"edge"))edge();else assert(0);
}
