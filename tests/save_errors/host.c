/* Host-only filesystem/controller mocks around extracted production save code. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "Preset/PresetManager.h"
#include "Soundengine/Sequencer/sequencer.h"
#include "menu/menu.h"
#include "lcd.h"
#define STEPS_PER_PATTERN NUM_STEPS
#define PRESET_SAVE_TIME 2000
#define FEXT_SOUND 0
#define FEXT_PAT 1
#define FEXT_ALL 2
#define FEXT_PROJ 3
#define FEXT_SONG 4
PatternSet seq_patternSet;
uint8_t seq_songData[NUM_KIT_SLOTS][MAX_SONG_LENGTH];
uint8_t mainParameters[NUM_KIT_SLOTS][END_OF_SOUND_PARAMETERS];
uint8_t activeParameters[NUM_PARAMS];
uint8_t *morphParameters;
char savefileNames[NUM_KIT_SLOTS][NUM_SAVEFILE_NAMES][8];
uint8_t menu_selectedPresetNr[NUM_PRESET_LOCATIONS],menu_loadedPresetNr[NUM_CURRENT];
uint8_t menu_playedPattern,menu_activePage,menu_activePatternBank,lcd_process_display_interrupt;
uint32_t sucessTimer;
volatile uint32_t systick_ticks;
FIL SDFile;
TIM_HandleTypeDef htim2;
static FATFS host_fs;
static FRESULT preset_sharedFailure;
static uint8_t preset_autosavePaused;
uint8_t preset_kitDirty,preset_patDirty;
uint32_t preset_kitTimer,preset_patTimer;
static BYTE output[65536],oracle[65536];
static size_t used;
static unsigned opens,writes,closes,fail_at,short_at,full_error_count,starts,stops,notifications,repaints,resets,directory_calls;
static FRESULT open_error,write_error,close_error,last_notification,ram_error;
static uint32_t now=3000;
static char last_name[64],oracle_name[64];
FRESULT f_open(FIL *f,const TCHAR *name,BYTE flags)
{
 assert(f==&SDFile && !f->obj.fs);opens++;strcpy(last_name,name);
 if(open_error!=FR_OK)return open_error;
 assert(flags==(FA_CREATE_ALWAYS|FA_WRITE) || flags==FA_READ);
 f->obj.fs=&host_fs;f->err=0;f->fptr=0;return FR_OK;
}
FRESULT f_write(FIL *f,const void *data,UINT count,UINT *actual)
{
 assert(f==&SDFile && f->obj.fs && used+count<=sizeof output);writes++;
 *actual=writes==short_at?count-1:count;
 if(writes==fail_at && !full_error_count)*actual=0;
 memcpy(output+used,data,*actual);used+=*actual;f->fptr+=*actual;
 if(writes==fail_at){f->err=(BYTE)write_error;return write_error;}
 return FR_OK;
}
FRESULT f_close(FIL *f)
{
 assert(f==&SDFile && f->obj.fs);closes++;
 if(close_error!=FR_OK)return close_error;
 if(f->err)return (FRESULT)f->err;
 f->obj.fs=0;return FR_OK;
}
uint32_t HAL_GetTick(void){return now;}
HAL_StatusTypeDef HAL_TIM_Base_Start_IT(TIM_HandleTypeDef *t){(void)t;starts++;return HAL_OK;}
HAL_StatusTypeDef HAL_TIM_Base_Stop_IT(TIM_HandleTypeDef *t){(void)t;stops++;return HAL_OK;}
void menu_repaintAll(void){repaints++;}
void menu_repaint(void){repaints++;}
void menu_resetSaveParameters(void){resets++;}
void menu_showSdError(uint8_t error){notifications++;last_notification=(FRESULT)error;}
FRESULT preset_savePatternRAM(uint8_t p){(void)p;return ram_error;}
void preset_saveDrumsetRam(uint8_t p,uint8_t morph){(void)p;(void)morph;}
FRESULT f_stat(const TCHAR *name,FILINFO *info){(void)name;(void)info;directory_calls++;return FR_OK;}
FRESULT f_mkdir(const TCHAR *name){(void)name;directory_calls++;return FR_OK;}
FRESULT f_chdir(const TCHAR *name){(void)name;directory_calls++;return FR_OK;}
FRESULT f_findfirst(DIR *dir,FILINFO *info,const TCHAR *path,const TCHAR *mask){(void)dir;(void)info;(void)path;(void)mask;directory_calls++;return FR_NO_FILE;}
FRESULT f_closedir(DIR *dir){(void)dir;directory_calls++;return FR_OK;}
FRESULT delete_node(TCHAR *path,UINT size,FILINFO *info){(void)path;(void)size;(void)info;directory_calls++;return FR_OK;}
#include "functions.inc"
static void session(void)
{
 /* Test-only simulated reboot/controller reset. There is no production clear. */
 memset(&SDFile,0,sizeof SDFile);preset_sharedFailure=FR_OK;preset_autosavePaused=0;
 used=opens=writes=closes=fail_at=short_at=full_error_count=starts=stops=notifications=repaints=resets=directory_calls=0;
 open_error=close_error=last_notification=ram_error=FR_OK;write_error=FR_DENIED;
 preset_patDirty=preset_kitDirty=0;preset_patTimer=preset_kitTimer=0;
 menu_playedPattern=0;menu_loadedPresetNr[CURRENT_KIT]=63;menu_activePage=SAVE_PAGE;menu_activePatternBank=0;
 sucessTimer=123;systick_ticks=456;last_name[0]=0;
}
static FRESULT save(unsigned kind,unsigned slot,unsigned baseline)
{
 menu_playedPattern=(uint8_t)slot;menu_selectedPresetNr[SAVE_TYPE_KIT]=(uint8_t)slot;
 if(baseline){switch(kind){
 case 0:return baseline_preset_saveTmpKit();case 1:return baseline_preset_saveDrumsetSD(slot,0);
 case 2:return baseline_preset_saveDrumsetSD(slot,1);case 3:return baseline_preset_saveTmpPat();
 case 4:return baseline_preset_savePatternSD(slot);case 5:return baseline_preset_saveSongSD(slot);
 case 6:return baseline_preset_saveGlobals();default:return baseline_preset_saveLastUsedProjectNrToSD(slot);}}
 switch(kind){case 0:return preset_saveTmpKit();case 1:return preset_saveDrumsetSD(slot,0);
 case 2:return preset_saveDrumsetSD(slot,1);case 3:return preset_saveTmpPat();
 case 4:return preset_savePatternSD(slot);case 5:return preset_saveSongSD(slot);
 case 6:return preset_saveGlobals();default:return preset_saveLastUsedProjectNrToSD(slot);}
}
static void contents(void)
{
 uint32_t random=0x12345678;
 void *data[]={&seq_patternSet,mainParameters,activeParameters,savefileNames,seq_songData};
 size_t sizes[]={sizeof seq_patternSet,sizeof mainParameters,sizeof activeParameters,sizeof savefileNames,sizeof seq_songData};
 for(unsigned i=0;i<5;i++)for(size_t n=0;n<sizes[i];n++){
  random=random*1664525u+1013904223u;((BYTE*)data[i])[n]=(BYTE)(random>>24);
 }
 morphParameters=mainParameters[NUM_KIT_SLOTS-1];
}
static void equivalence(void)
{
 unsigned cases=0;
 for(unsigned seed=0;seed<3;seed++) {
  contents();activeParameters[PAR_MORPH]=(BYTE)(seed==2?255:seed*127);
  for(unsigned kind=0;kind<8;kind++)for(unsigned slot=0;slot<NUM_KIT_SLOTS;slot++){
   session();assert(save(kind,slot,1)==FR_OK);size_t expected=used;unsigned calls=writes;
   memcpy(oracle,output,used);strcpy(oracle_name,last_name);
   session();assert(save(kind,slot,0)==FR_OK && used==expected && writes==calls);
   assert(!memcmp(output,oracle,used) && !strcmp(last_name,oracle_name));
   assert(opens==1 && closes==1 && !SDFile.obj.fs && starts==stops);cases++;
  }
 }
 printf("PASS %u byte-for-byte S3 oracle saves: eight variants, all 64 slots, Morph endpoints/midpoint, identical names/write counts and balanced timer hooks\n",cases);
}
static void quarantined(FRESULT expected)
{
 unsigned old_open=opens,old_write=writes,old_close=closes;
 BYTE error=SDFile.err;FATFS *owner=SDFile.obj.fs;
 assert(preset_saveGlobals()==expected);
 assert(preset_openShared(&SDFile,"read",FA_READ)==expected);
 assert(preset_closeShared(&SDFile)==expected);
 char name[13];assert(preset_makeFileNameLoad(name,0,FEXT_PAT)==expected);
 preset_initTmpFolder();assert(preset_clearTmpFolder()==expected);
 assert(opens==old_open && writes==old_write && closes==old_close && !directory_calls);
 assert(SDFile.err==error && SDFile.obj.fs==owner);
}
static void failures(void)
{
 unsigned positions=0;
 for(unsigned kind=0;kind<8;kind++){
  session();assert(save(kind,3,0)==FR_OK);unsigned count=writes;
  session();open_error=FR_NO_PATH;assert(save(kind,3,0)==FR_NO_PATH && opens==1 && !writes && !closes && !SDFile.obj.fs && starts==stops);
  assert(preset_sharedFailure==FR_OK);open_error=FR_OK;assert(save(kind,3,0)==FR_OK && opens==2 && closes==1);
  session();open_error=FR_DISK_ERR;assert(save(kind,3,0)==FR_DISK_ERR && !writes && !closes);quarantined(FR_DISK_ERR);
  for(unsigned pos=1;pos<=count;pos++)for(unsigned mode=0;mode<4;mode++){
   session();close_error=FR_NOT_READY;
   if(mode==2)short_at=pos;else{fail_at=pos;full_error_count=mode==1;}
   if(mode==3)write_error=FR_DISK_ERR;
   FRESULT expected=mode>=2?FR_DISK_ERR:FR_DENIED;
   assert(save(kind,3,0)==expected && writes==pos && !closes && SDFile.obj.fs && starts==stops);
   assert(preset_sharedFailure==expected);quarantined(expected);positions++;
  }
  session();close_error=FR_NOT_READY;assert(save(kind,3,0)==FR_NOT_READY && writes==count && closes==1 && SDFile.obj.fs && starts==stops);
  quarantined(FR_NOT_READY);
 }
 session();assert(preset_savePatternSD(255)==FR_INVALID_PARAMETER && preset_saveDrumsetSD(255,0)==FR_INVALID_PARAMETER && preset_saveSongSD(255)==FR_INVALID_PARAMETER && !opens);
 menu_playedPattern=255;assert(preset_saveTmpPat()==FR_INVALID_PARAMETER && !opens);
 session();SDFile.obj.fs=&host_fs;assert(preset_saveGlobals()==FR_LOCKED && !opens && !writes && !closes);
 printf("PASS %u injected write/short-write positions: every payload call, full-count errors, open/close failures, first-error preservation, no payload/close/reuse/cleanup after quarantine\n",positions);
}
static void ui_and_background(void)
{
 for(unsigned kind=0;kind<3;kind++)for(unsigned failure=0;failure<3;failure++){
  session();menu_selectedPresetNr[SAVE_TYPE_KIT]=3;menu_selectedPresetNr[SAVE_TYPE_PATTERN]=3;
  if(failure==1)short_at=2;if(failure==2)close_error=FR_NOT_READY;
  exercise_menu_save(kind==0?SAVE_TYPE_PATTERN:kind==1?SAVE_TYPE_KIT:SAVE_TYPE_GLO);
  if(failure){assert(notifications==1 && !resets && sucessTimer==123 && menu_loadedPresetNr[CURRENT_KIT]==63 && menu_activePage==SAVE_PAGE);}
  else{assert(!notifications && resets==1);if(kind==1)assert(menu_activePage==KIT_SAVE_PAGE_OK && menu_loadedPresetNr[CURRENT_KIT]==3 && sucessTimer==456);}
  assert(starts==stops);
 }
 session();ram_error=FR_INT_ERR;exercise_menu_save(SAVE_TYPE_PATTERN);assert(notifications==1 && !opens && !resets);
 session();ram_error=FR_INT_ERR;exercise_button_save(3);assert(notifications==1 && !opens);
 for(unsigned failure=0;failure<3;failure++){
  session();menu_activePage=PATTERN_SAVE_PAGE;menu_selectedPresetNr[SAVE_TYPE_PATTERN]=42;
  if(failure==1)short_at=3;if(failure==2)close_error=FR_DISK_ERR;
  exercise_button_save(3);
  if(failure)assert(notifications==1 && menu_activePage==PATTERN_SAVE_PAGE && menu_selectedPresetNr[SAVE_TYPE_PATTERN]==42 && sucessTimer==123);
  else assert(!notifications && menu_activePage==PATTERN_SAVE_PAGE_OK && menu_selectedPresetNr[SAVE_TYPE_PATTERN]==3 && sucessTimer==456);
  assert(starts==stops);
 }
 session();preset_patDirty=preset_kitDirty=1;short_at=1;preset_processTimer();
 assert(notifications==1 && opens==1 && preset_patDirty && preset_kitDirty && preset_autosavePaused);
 preset_patDirty=preset_kitDirty=1;preset_processTimer();assert(notifications==1 && opens==1 && preset_patDirty && preset_kitDirty);
 session();preset_patDirty=preset_kitDirty=1;preset_processTimer();assert(!notifications && opens==2 && closes==2 && !preset_patDirty && !preset_kitDirty);
 session();preset_kitDirty=1;short_at=2;preset_processTimer();assert(notifications==1 && opens==1 && preset_kitDirty && preset_autosavePaused);
 session();preset_patDirty=1;open_error=FR_NO_PATH;preset_processTimer();assert(notifications==1 && opens==1 && preset_patDirty && preset_autosavePaused);preset_processTimer();assert(opens==1);
 open_error=FR_OK;assert(preset_saveGlobals()==FR_OK && preset_patDirty && preset_autosavePaused);
 session();short_at=2;preset_createGlobals();assert(notifications==1 && last_notification==FR_DISK_ERR && writes==2 && !closes);
 puts("PASS actual menu case/button bodies and timer service: success-only state, write/close/RAM error UI, stopped background retries; no UI/hardware execution");
}
static void logical_retries(void)
{
 const FRESULT retryable[]={FR_NO_FILE,FR_NO_PATH,FR_INVALID_NAME,FR_EXIST,FR_INVALID_DRIVE,
  FR_NOT_ENABLED,FR_NO_FILESYSTEM,FR_WRITE_PROTECTED,FR_LOCKED,FR_TOO_MANY_OPEN_FILES,FR_INVALID_PARAMETER,FR_NOT_READY,FR_DENIED};
 for(unsigned k=0;k<8;k++)for(unsigned e=0;e<sizeof retryable/sizeof *retryable;e++){
  session();contents();BYTE before[32];memcpy(before,activeParameters,sizeof before);
  memcpy(output,"Existing valid bytes",20);used=20;
  open_error=retryable[e];
  for(unsigned n=0;n<9;n++)assert(save(k,2,0)==open_error && !writes && !closes && !SDFile.obj.fs && preset_sharedFailure==FR_OK);
  assert(used==20 && !memcmp(output,"Existing valid bytes",20) && !memcmp(before,activeParameters,sizeof before));
  open_error=FR_OK;used=0;assert(save(k,2,0)==FR_OK && opens==10 && closes==1);
 }
 session();open_error=FR_TIMEOUT;assert(preset_saveGlobals()==FR_TIMEOUT);quarantined(FR_TIMEOUT);
 puts("PASS S4-L: 936 recoverable open failures then 104 successful explicit saves; no accumulated latch/data loss; ambiguous TIMEOUT and all payload failures still held");
}
int main(void){setvbuf(stdout,0,_IONBF,0);contents();equivalence();failures();ui_and_background();logical_retries();}
