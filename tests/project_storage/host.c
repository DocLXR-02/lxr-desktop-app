/* Actual extracted production functions. Filesystem/HAL/UI below are host mocks. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <fnmatch.h>
#include <stdint.h>
#define _FF_INTEGER
typedef int32_t INT,LONG;
typedef uint32_t UINT,DWORD;
typedef uint8_t BYTE;
typedef int16_t SHORT;
typedef uint16_t WORD,WCHAR;
typedef uint64_t QWORD;
#include "Preset/PresetManager.h"
#include "Soundengine/Sequencer/sequencer.h"
#include "menu/menu.h"
#include "lcd.h"
#include "frontPanelParser_ui.h"
#include "Soundengine/DSPAudio/automationNode.h"
#define STEPS_PER_PATTERN NUM_STEPS
#define FEXT_SOUND 0
#define FEXT_PAT 1
#define FEXT_ALL 2
#define FEXT_PROJ 3
#define FEXT_SONG 4
PatternSet seq_patternSet;
PatternBackupBuffer patternBackupBuffer;
uint8_t mainParameters[NUM_KIT_SLOTS][END_OF_SOUND_PARAMETERS],activeParameters[NUM_PARAMS];
uint8_t *morphParameters,*seq_activeSong;
uint8_t seq_songData[NUM_KIT_SLOTS][MAX_SONG_LENGTH];
uint8_t seq_activePattern,seq_pendingPattern;
uint8_t menu_selectedPresetNr[NUM_PRESET_LOCATIONS],menu_loadedPresetNr[NUM_CURRENT];
uint8_t menu_playedPattern,menu_shownPattern,lcd_process_display_interrupt;
char currentFolder[13],savefileNames[NUM_KIT_SLOTS][NUM_SAVEFILE_NAMES][8];
FIL SDFile;
TIM_HandleTypeDef htim2;
static FATFS host_fs;
static FRESULT preset_sharedFailure;
static uint8_t preset_autosavePaused;

typedef struct {char path[80]; BYTE data[4096]; unsigned size; BYTE attr,exists;} Entry;
static Entry entries[512];
static char cwd[80];
static unsigned opens,reads,writes,closes,stats,mkdirs,chdirs,unlinks,dirreads,dircloses;
static unsigned starts,stops,sends,audio_calls,index_writes;
static unsigned open_fail,read_fail,read_short,write_fail,write_short,close_fail;
static FRESULT open_result,read_result,write_result,close_result,stat_result,chdir_result,find_result;
static FRESULT dir_result,dir_close_result,unlink_result;
static const char *bad_entry;
static void resolve(char out[80],const char *path)
{
 int n=!path[0]?snprintf(out,80,"%s",cwd):path[0]=='/'?snprintf(out,80,"%s",path):snprintf(out,80,"%s%s%s",cwd,strcmp(cwd,"/")?"/":"",path);
 assert(n>=0 && n<80);
}
static int lookup(const char *path)
{for(unsigned i=0;i<512;i++)if(entries[i].exists && !strcasecmp(path,entries[i].path))return i;return -1;}
static int add(const char *path,BYTE attr,const void *data,unsigned size)
{
 assert(lookup(path)<0 && size<=4096);
 for(unsigned i=0;i<512;i++)if(!entries[i].exists){
  memset(entries+i,0,sizeof entries[i]);strcpy(entries[i].path,path);entries[i].exists=1;
  entries[i].attr=attr;entries[i].size=size;if(size)memcpy(entries[i].data,data,size);return i;
 }
 assert(!"host file table full");return -1;
}
static void reset(void)
{
 memset(entries,0,sizeof entries);add("/tmp",AM_DIR,0,0);strcpy(cwd,"/tmp");
 memset(&SDFile,0,sizeof SDFile);memset(&host_fs,0,sizeof host_fs);host_fs.fs_type=2;
 preset_sharedFailure=FR_OK;preset_autosavePaused=0;
 opens=reads=writes=closes=stats=mkdirs=chdirs=unlinks=dirreads=dircloses=0;
 starts=stops=sends=audio_calls=index_writes=0;
 open_fail=read_fail=read_short=write_fail=write_short=close_fail=0;
 open_result=read_result=write_result=close_result=FR_DISK_ERR;
 stat_result=chdir_result=find_result=dir_result=dir_close_result=unlink_result=FR_OK;
 bad_entry=0;lcd_process_display_interrupt=1;
 memset(mainParameters,0x42,sizeof mainParameters);memset(activeParameters,0,sizeof activeParameters);
 memset(seq_songData,SONG_END,sizeof seq_songData);memset(savefileNames,0,sizeof savefileNames);
 memset(&seq_patternSet,0,sizeof seq_patternSet);
 for(unsigned p=0;p<64;p++){
  memcpy(savefileNames[p][KIT_NAMES],"KICKNAME",8);memcpy(savefileNames[p][PATTERN_NAMES],"PATTERN!",8);
  memcpy(savefileNames[p][SONG_NAMES],"SONGNAME",8);memcpy(savefileNames[p][PROJECT_NAMES],"PROJECT!",8);
  for(unsigned t=0;t<7;t++)seq_patternSet.length[p][t]=16;
 }
 seq_activePattern=seq_pendingPattern=menu_playedPattern=menu_shownPattern=0;
 memset(menu_selectedPresetNr,0,sizeof menu_selectedPresetNr);memset(menu_loadedPresetNr,0,sizeof menu_loadedPresetNr);
 morphParameters=mainParameters[0];seq_activeSong=seq_songData[0];
}
FRESULT f_open(FIL *file,const TCHAR *path,BYTE mode)
{
 assert(file==&SDFile && !file->obj.fs);opens++;
 if(opens==open_fail)return open_result;
 char absolute[80];resolve(absolute,path);int i=lookup(absolute);
 if(i>=0 && (mode&FA_CREATE_NEW))return FR_EXIST;
 if(i<0){if(!(mode&(FA_CREATE_ALWAYS|FA_CREATE_NEW|FA_OPEN_ALWAYS)))return FR_NO_FILE;i=add(absolute,AM_ARC,0,0);}
 if(entries[i].attr&AM_DIR)return FR_DENIED;
 if(mode&FA_CREATE_ALWAYS)entries[i].size=0;
 memset(file,0,sizeof *file);file->obj.fs=&host_fs;file->clust=i+1;file->flag=mode;file->obj.objsize=entries[i].size;
 return FR_OK;
}
FRESULT f_read(FIL *file,void *data,UINT size,UINT *actual)
{
 assert(file==&SDFile && file->obj.fs && !(file->flag&FA_WRITE));reads++;
 Entry *e=entries+file->clust-1;assert(file->fptr<=e->size);
 *actual=size<e->size-file->fptr?size:e->size-file->fptr;
 if(reads==read_short && *actual)(*actual)--;
 memcpy(data,e->data+file->fptr,*actual);file->fptr+=*actual;
 return reads==read_fail?read_result:FR_OK;
}
FRESULT f_write(FIL *file,const void *data,UINT size,UINT *actual)
{
 assert(file==&SDFile && file->obj.fs && (file->flag&FA_WRITE));writes++;
 Entry *e=entries+file->clust-1;assert(file->fptr+size<=sizeof e->data);
 *actual=writes==write_short && size?size-1:size;
 if(writes==write_fail)*actual=0;
 memcpy(e->data+file->fptr,data,*actual);file->fptr+=*actual;e->size=file->obj.objsize=file->fptr;
 if(!strcmp(e->path,"/proj.idx"))index_writes++;
 if(writes==write_fail){file->err=write_result;return write_result;}
 return FR_OK;
}
FRESULT f_close(FIL *file)
{
 assert(file==&SDFile && file->obj.fs);closes++;
 if(closes==close_fail)return close_result;
 if(file->err)return (FRESULT)file->err;
 file->obj.fs=0;return FR_OK;
}
FRESULT f_lseek(FIL *file,DWORD offset)
{assert(file->obj.fs);if(offset>file->obj.objsize)return FR_INT_ERR;file->fptr=offset;return FR_OK;}
FRESULT f_stat(const TCHAR *path,FILINFO *info)
{
 stats++;if(stat_result!=FR_OK)return stat_result;
 char absolute[80];resolve(absolute,path);int i=lookup(absolute);if(i<0)return FR_NO_FILE;
 memset(info,0,sizeof *info);info->fsize=entries[i].size;info->fattrib=entries[i].attr;return FR_OK;
}
FRESULT f_mkdir(const TCHAR *path)
{
 mkdirs++;char absolute[80];resolve(absolute,path);if(lookup(absolute)>=0)return FR_EXIST;
 add(absolute,AM_DIR,0,0);return FR_OK;
}
FRESULT f_chdir(const TCHAR *path)
{
 chdirs++;if(chdir_result!=FR_OK)return chdir_result;
 char absolute[80];resolve(absolute,path);int i=lookup(absolute);
 if(strcmp(absolute,"/") && (i<0 || !(entries[i].attr&AM_DIR)))return FR_NO_PATH;
 strcpy(cwd,absolute);return FR_OK;
}
static int child(const char *directory,const char *path)
{size_t n=strlen(directory);return !strncasecmp(directory,path,n) && path[n]=='/' && !strchr(path+n+1,'/');}
FRESULT f_opendir(DIR *dir,const TCHAR *path)
{
 char absolute[80];resolve(absolute,path);int i=lookup(absolute);
 if(i<0 || !(entries[i].attr&AM_DIR))return FR_NO_PATH;
 memset(dir,0,sizeof *dir);dir->obj.fs=&host_fs;dir->obj.sclust=i+1;return FR_OK;
}
FRESULT f_readdir(DIR *dir,FILINFO *info)
{
 dirreads++;memset(info,0,sizeof *info);if(dir_result!=FR_OK)return dir_result;
 if(bad_entry){assert(strlen(bad_entry)<sizeof info->fname);strcpy(info->fname,bad_entry);bad_entry=0;return FR_OK;}
 while(dir->dptr<512){unsigned i=dir->dptr++;
  if(entries[i].exists && child(entries[dir->obj.sclust-1].path,entries[i].path)){
   const char *base=strrchr(entries[i].path,'/')+1;assert(strlen(base)<sizeof info->fname);
   strcpy(info->fname,base);info->fattrib=entries[i].attr;return FR_OK;
  }
 }
 return FR_OK;
}
FRESULT f_closedir(DIR *dir){dircloses++;dir->obj.fs=0;return dir_close_result;}
static void lower(char *out,const char *s)
{do{*out=*s>='A' && *s<='Z'?*s+32:*s;out++;}while(*s++);}
FRESULT f_findfirst(DIR *dir,FILINFO *info,const TCHAR *path,const TCHAR *mask)
{
 FRESULT r=f_opendir(dir,path);if(r!=FR_OK)return r;if(find_result!=FR_OK)return find_result;
 char wanted[32];lower(wanted,mask);
 do {r=f_readdir(dir,info);if(r!=FR_OK || !info->fname[0])return r;
  char actual[13];lower(actual,info->fname);if(!fnmatch(wanted,actual,0))return FR_OK;
 }while(1);
}
FRESULT f_unlink(const TCHAR *path)
{
 unlinks++;assert(!strncmp(path,"/tmp/",5));int i=lookup(path);assert(i>=0 && !(entries[i].attr&AM_DIR));
 if(unlink_result!=FR_OK)return unlink_result;entries[i].exists=0;return FR_OK;
}
HAL_StatusTypeDef HAL_TIM_Base_Start_IT(TIM_HandleTypeDef *timer){(void)timer;starts++;return HAL_OK;}
HAL_StatusTypeDef HAL_TIM_Base_Stop_IT(TIM_HandleTypeDef *timer){(void)timer;stops++;return HAL_OK;}
void soundengine_process(void){audio_calls++;}
void preset_sendDrumsetParameters(void){sends++;}
void preset_sendDrumsetParametersImmediate(void){sends++;}
void menu_sendAllGlobals(void){sends++;}
uint8_t getNumModTargets(void){return 16;}
uint8_t sendDisplayBuffer(void){return 0;}
void lcd_clear(void){} void lcd_home(void){} void lcd_string(const char *p){(void)p;}
void lcd_setcursor(uint8_t x,uint8_t y){(void)x;(void)y;}
void menu_repaintAll(void){}
void menu_showSdError(uint8_t e){(void)e;assert(!"no new notification path expected");}
void frontPanel_sendData(uint8_t a,uint8_t b,uint8_t c){(void)a;(void)b;(void)c;}
void seq_setNextPattern(const uint8_t slot){assert(slot<64);seq_pendingPattern=slot;}
void seq_copyPattern(uint8_t a,uint8_t b){(void)a;(void)b;assert(!"project save only commits active slot");}
#include "functions.inc"

static void paths(void)
{
 reset();char p[21],before[21];memset(p,'!',sizeof p);memcpy(before,p,sizeof p);
 for(unsigned n=0;n<256;n++){
  assert(preset_projectPath(p,sizeof p,n)==(n<64?FR_OK:FR_INVALID_PARAMETER));
  if(n<64){char expected[8];snprintf(expected,sizeof expected,"/proj%02u",n);assert(!strcmp(p,expected));}
 }
 assert(preset_projectPath(p,7,0)==FR_INVALID_PARAMETER);
 assert(preset_childPath(p,sizeof p,"/proj00","prj.nfo")==FR_OK && !strcmp(p,"/proj00/prj.nfo"));
 assert(preset_childPath(p,sizeof p,"/proj63","00-ABCDE.SND")==FR_OK && strlen(p)==20);
 assert(preset_childPath(p,20,"/proj63","00-ABCDE.SND")==FR_INVALID_NAME);
 const char *bad[]={"",".","..","../X","00-A/B.SND","123456789.SND","X.LONG","A..B",".SND","A.","A+B.SND"};
 for(unsigned i=0;i<sizeof bad/sizeof *bad;i++){
  memset(p,'!',sizeof p);assert(preset_childPath(p,sizeof p,"/proj00",bad[i])==FR_INVALID_NAME);
  assert(!memcmp(p,before,sizeof p));
 }
 assert(!opens && !writes && !unlinks);
 puts("PASS capacity-checked project/SFN paths: all256 indices, exact maximum8.3, old overflowing metadata path and unchanged output on rejection");
}
static void invalid_indices(void)
{
 reset();
 for(unsigned n=64;n<256;n++){
  assert(preset_saveProject(n)==FR_INVALID_PARAMETER && preset_loadProject(n,1)==FR_INVALID_PARAMETER);
  assert(preset_createProject(n)==FR_INVALID_PARAMETER && preset_loadSongSD(n)==FR_INVALID_PARAMETER);
  assert(preset_loadDrumsetSD(n,0)==FR_INVALID_PARAMETER && preset_saveLastUsedProjectNrToSD(n)==FR_INVALID_PARAMETER);
  preset_loadProjectNames(n);preset_loadDrumsetRam(n,0,1);preset_saveDrumsetRam(n,0);preset_init(n);
 }
 assert(!opens && !stats && !writes && !sends && !audio_calls && !unlinks);
 seq_activePattern=255;assert(preset_saveProject(0)==FR_INVALID_PARAMETER && preset_loadProject(0,0)==FR_INVALID_PARAMETER);
 seq_activePattern=0;menu_selectedPresetNr[SAVE_TYPE_KIT]=255;assert(preset_saveProject(0)==FR_INVALID_PARAMETER);
 menu_selectedPresetNr[SAVE_TYPE_KIT]=0;seq_patternSet.kitNr[63]=255;assert(preset_saveProject(0)==FR_INT_ERR && !stats);
 uint8_t value=255,slot=99;add("/proj.idx",AM_ARC,&value,1);
 assert(preset_loadLastUsedProjectNrFromSD(&slot)==FR_INT_ERR && slot==0 && !SDFile.obj.fs);
 assert(preset_loadLastUsedProjectNrFromSD(0)==FR_INVALID_PARAMETER);
 for(unsigned n=0;n<256;n++){
  reset();BYTE tail[8]={16,16,16,16,16,16,16,n};add("/tmp/tail",AM_ARC,tail,8);
  assert(preset_openShared(&SDFile,"tail",FA_READ)==FR_OK);seq_patternSet.kitNr[0]=7;seq_patternSet.length[0][0]=31;
  assert(preset_readPatternLengths(0,3)==(n<64));
  assert(seq_patternSet.kitNr[0]==(n<64?n:7) && seq_patternSet.length[0][0]==(n<64?16:31));
  assert(preset_finishRead(FR_OK)==FR_OK);
 }
 reset();BYTE tmp[248]={255};add("/tmp/tmp.snd",AM_ARC,tmp,sizeof tmp);
 assert(preset_LoadTmpKit()==FR_INT_ERR && !sends && menu_selectedPresetNr[SAVE_TYPE_KIT]==0);
 puts("PASS invalid project/kit consumers and all256 associated-kit values: no OOB, no publication or I/O on rejected public indices");
}
static void cleanup(void)
{
 reset();const char *owned[]={"tmp.snd","TMP.PAT","00-ABCDE.SND","63-SONG.SNG","p000.pat"};
 char p[80];for(unsigned i=0;i<5;i++){snprintf(p,sizeof p,"/tmp/%s",owned[i]);add(p,AM_ARC,"data",4);}
 add("/tmp/README.TXT",AM_ARC,"keep",4);add("/tmp/64-NAME.SND",AM_ARC,"keep",4);
 add("/tmp/SUBDIR",AM_DIR,0,0);add("/tmp/SUBDIR/tmp.snd",AM_ARC,"keep",4);
 assert(preset_clearTmpFolder()==FR_OK && unlinks==5 && lookup("/tmp")>=0);
 assert(lookup("/tmp/README.TXT")>=0 && lookup("/tmp/64-NAME.SND")>=0 && lookup("/tmp/SUBDIR/tmp.snd")>=0);
 reset();bad_entry="00-A/B.SND";assert(preset_clearTmpFolder()==FR_INVALID_NAME && !unlinks && dirreads==1 && dircloses==1);
 reset();dir_close_result=FR_DISK_ERR;assert(preset_clearTmpFolder()==FR_DISK_ERR);
 reset();add("/tmp/tmp.snd",AM_ARC,"data",4);unlink_result=FR_DENIED;assert(preset_clearTmpFolder()==FR_DENIED && unlinks==1 && lookup("/tmp/tmp.snd")>=0);
 puts("PASS nonrecursive temp cleanup: maximum SFN, owned whitelist, unknown files/subdirectories retained, invalid path causes no unlink, close/unlink errors retained");
}
static void protection(void)
{
 reset();add("/proj00",AM_DIR,0,0);add("/proj00/prj.nfo",AM_ARC,"OLDVALID",8);
 add("/proj00/README.TXT",AM_ARC,"unknown",7);add("/proj00/SUBDIR",AM_DIR,0,0);
 static Entry before[512];memcpy(before,entries,sizeof entries);
 for(unsigned i=0;i<10;i++)assert(preset_saveProject(0)==FR_EXIST);
 assert(!memcmp(before,entries,sizeof entries) && !opens && !writes && !unlinks && !mkdirs && !chdirs);
 assert(preset_sharedFailure==FR_OK && starts==stops);
 assert(preset_saveProject(63)==FR_OK && starts==stops && !SDFile.obj.fs);
 unsigned payload=0,count=0;for(unsigned i=0;i<512;i++)if(entries[i].exists && child("/proj63",entries[i].path)){payload+=entries[i].size;count++;}
 assert(count==194 && payload==255398 && !strcmp(cwd,"/tmp") && lookup("/proj00/prj.nfo")>=0);
 assert(!memcmp(entries[lookup("/proj00/prj.nfo")].data,"OLDVALID",8));
 assert(preset_saveProject(63)==FR_EXIST);
 reset();stat_result=FR_DISK_ERR;assert(preset_saveProject(1)==FR_DISK_ERR && !mkdirs && !opens);
 reset();add("/proj01",AM_ARC,"unknown",7);assert(preset_saveProject(1)==FR_EXIST && !writes && !mkdirs);
 reset();add("/proj01",AM_DIR,0,0);assert(preset_saveProject(1)==FR_EXIST && !writes && !mkdirs);
 reset();chdir_result=FR_NO_PATH;assert(preset_saveProject(1)==FR_NO_PATH && !opens && !writes);
 puts("PASS overwrite containment: ten truthful FR_EXIST results preserve all old/unknown data; safe slot63 saves194 files/255398 bytes; wrong CWD never receives payload");
}
static void project_failures(void)
{
 const unsigned payload_positions[]={1,2,3,249,250,300,500,5000,19000,19394};
 const unsigned file_positions[]={1,2,3,4,5,33,97,193,194};unsigned cases=0;
 for(unsigned kind=0;kind<4;kind++)for(unsigned j=0;j<(kind<2?10:9);j++){
  reset();add("/proj00",AM_DIR,0,0);add("/proj00/prj.nfo",AM_ARC,"OLDVALID",8);
  add("/tmp/tmp.snd",AM_ARC,"recovery",8);
  unsigned pos=kind<2?payload_positions[j]:file_positions[j];
  if(kind==0)write_fail=pos;else if(kind==1)write_short=pos;else if(kind==2)close_fail=pos;else open_fail=pos;
  FRESULT r=preset_saveProject(1);
  unsigned calls=kind<2?writes:kind==2?closes:opens;
  assert(calls==pos); /* Every selected failure point must actually be reached. */
  assert(r!=FR_OK && lookup("/tmp/tmp.snd")>=0 && !unlinks && starts==stops);
  assert(!memcmp(entries[lookup("/proj00/prj.nfo")].data,"OLDVALID",8));
  if(preset_sharedFailure!=FR_OK){unsigned prior=writes;assert(preset_saveProject(2)==r && writes==prior);}
  cases++;
 }
 reset();write_short=1;assert(preset_saveProject(1)==FR_DISK_ERR && writes==1 && !closes);
 puts("PASS injected project nfo/global/early/late child open/write/short-write/close failures: first error, no later payload, no old-project or temp deletion, unsafe replay blocked");
 assert(cases==38);printf("Project failure scenarios exercised: %u\n",cases);
}
static void reads_and_legacy(void)
{
 /* Short metadata fails before RAM, CWD or temp recovery changes. */
 for(unsigned n=0;n<8;n++){
  reset();add("/proj00",AM_DIR,0,0);add("/proj00/prj.nfo",AM_ARC,"PROJECT!",n);
  add("/tmp/tmp.snd",AM_ARC,"recover",7);
  assert(preset_loadProject(0,1)==FR_INT_ERR && lookup("/tmp/tmp.snd")>=0 && !unlinks && !writes && !chdirs);
  assert(mainParameters[0][0]==0x42 && starts==stops);
 }
 reset();assert(preset_loadProject(1,1)==FR_NO_FILE && !mkdirs && !unlinks);
 reset();add("/proj00",AM_DIR,0,0);assert(preset_loadProject(0,1)==FR_INT_ERR && !mkdirs && !unlinks);
 for(unsigned failure=0;failure<3;failure++){
  reset();add("/proj00",AM_DIR,0,0);add("/proj00/prj.nfo",AM_ARC,"PROJECT!",8);
  if(failure==0)open_fail=1;else if(failure==1)read_fail=1;else close_fail=1;
  assert(preset_loadProject(0,1)==FR_DISK_ERR && !writes && !unlinks && starts==stops);
 }
 reset();add("/proj00",AM_DIR,0,0);add("/proj00/prj.nfo",AM_ARC,"PROJECT!",8);
 memset(seq_songData,0x69,sizeof seq_songData);memset(&seq_patternSet,0x33,sizeof seq_patternSet);seq_activePattern=0;
 memset(&activeParameters[PAR_BEGINNING_OF_GLOBALS],0x69,NUM_PARAMS-PAR_BEGINNING_OF_GLOBALS);
 assert(preset_loadProject(0,0)==FR_OK && index_writes==1 && !unlinks);
 assert(activeParameters[PAR_MIDI_MODE]==0);
 for(unsigned slot=0;slot<64;slot++){
  for(unsigned t=0;t<7;t++)assert(seq_patternSet.length[slot][t]==16);
  assert(seq_patternSet.kitNr[slot]==0 && seq_patternSet.shuffle[slot]==0);
  for(unsigned i=0;i<64;i++)assert(seq_songData[slot][i]==SONG_END);
  assert(mainParameters[slot][END_OF_SOUND_PARAMETERS-1]==0);
 }
 /* Named legacy song filenames use the same complete72-byte payload. */
 for(unsigned n=0;n<72;n++){
  reset();BYTE song[72]={0};memcpy(song,"SONGNAME",8);add("/tmp/p000.sng",AM_ARC,song,n);
  memset(seq_songData[0],0x69,64);assert(preset_loadSongSD(0)!=FR_OK);
  for(unsigned i=0;i<64;i++)assert(seq_songData[0][i]==0x69);
 }
 reset();BYTE song[72]={0};memcpy(song,"SONGNAME",8);song[8]=12;add("/tmp/p000.sng",AM_ARC,song,72);
 assert(preset_loadSongSD(0)==FR_OK && seq_songData[0][0]==12);
 reset();BYTE kit[12]={0};memcpy(kit,"OLDKIT!!",8);kit[8]=7;add("/tmp/p000.snd",AM_ARC,kit,12);
 assert(preset_loadDrumsetSD(0,0)==FR_OK && mainParameters[0][0]==7 && mainParameters[0][246]==0);
 reset();BYTE index=63;add("/proj.idx",AM_ARC,&index,1);uint8_t slot=0;
 close_fail=1;assert(preset_loadLastUsedProjectNrFromSD(&slot)==FR_DISK_ERR && slot==0);
 reset();find_result=FR_DISK_ERR;assert(preset_loadSongSD(0)==FR_DISK_ERR && !opens);
 reset();add("/proj00",AM_DIR,0,0);add("/proj00/prj.nfo",AM_ARC,"PROJECT!",8);
 strcpy(cwd,"/");preset_loadProjectNames(0);assert(!memcmp(savefileNames[0][PROJECT_NAMES],"PROJECT!",8) && !strcmp(cwd,"/"));
 puts("PASS short metadata/songs, read/open/close/search errors and absolute name lookup; intentionally incomplete projects get deterministic defaults; old kit/song filenames preserved");
}
static void reader_closes(void)
{
 for(unsigned type=0;type<6;type++){
  reset();FRESULT r;
  switch(type){
  case 0:r=preset_saveSongSD(0);break;
  case 1:r=preset_saveDrumsetSD(0,0);break;
  case 2:r=preset_savePatternSD(0);break;
  case 3:r=preset_saveTmpKit();break;
  case 4:r=preset_saveTmpPat();break;
  default:r=preset_saveGlobals();break;
  }
  assert(r==FR_OK);close_fail=closes+1;
  switch(type){
  case 0:r=preset_loadSongSD(0);break;
  case 1:r=preset_loadDrumsetSD(0,0);break;
  case 2:r=preset_loadPatternSD(0);break;
  case 3:r=preset_LoadTmpKit();break;
  case 4:r=preset_LoadTmpPat();break;
  default:r=preset_loadGlobalFile();break;
  }
  assert(r==FR_DISK_ERR && preset_sharedFailure==FR_DISK_ERR && SDFile.obj.fs);
  unsigned before=writes;assert(preset_saveProject(1)==FR_DISK_ERR && writes==before);
 }
 reset();add("/proj00",AM_DIR,0,0);add("/proj00/prj.nfo",AM_ARC,"NEWNAME!",8);
 memcpy(savefileNames[0][PROJECT_NAMES],"OLDNAME!",8);close_fail=1;
 preset_loadProjectNames(0);assert(!memcmp(savefileNames[0][PROJECT_NAMES],"OLDNAME!",8) && preset_sharedFailure==FR_DISK_ERR);
 reset();memset(mainParameters[0],0x69,END_OF_SOUND_PARAMETERS);preset_defaultKit(0);
 assert(!memcmp(mainParameters[0],initKit+8,INIT_KIT_SIZE-8) && mainParameters[0][END_OF_SOUND_PARAMETERS-1]==0);
 puts("PASS required reader closes: kit/pattern/song/temp/global/name failure never reports success or bypasses retained ownership; init defaults bounded to actual payload");
}
int main(void)
{
 setvbuf(stdout,0,_IONBF,0);paths();invalid_indices();cleanup();protection();project_failures();reads_and_legacy();reader_closes();
 puts("PASS all S4b-1 production-body groups under ASan/UBSan (filesystem, HAL, LCD and sound sending are mocked)");
 return 0;
}
