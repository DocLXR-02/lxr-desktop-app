#include <stdint.h>
uint8_t getNumModTargets(void);
void soundengine_process(void);
static void r21_frontSend(uint8_t,uint8_t,uint8_t);
#include "whole.inc"
#include "buttonHandler.h"
#include "menu/copyClearTools.h"
#include "menu/CcNr2Text.h"
#include "menu/screensaver.h"
#include "ledHandler.h"
#include "front.h"
uint8_t menu_activePage, menu_activeVoice, visibleCursor, display_needs_refresh;
volatile uint8_t screensaver_Active;
char editDisplayBuffer[2][17];
static uint8_t editModeActive, lastEncoderButton, host_copy, host_shift, host_mode;
static unsigned clear_actions, edit_actions, navigate_actions;
static uint32_t now, irq_mask;
static void (*before_commit)(void);
uint32_t HAL_GetTick(void){return now;}
uint8_t seq_isRunning(void);
static uint8_t running,seq_loadPendigFlag,morph_state,morph_index;
#include "pending.inc"
uint8_t seq_isRunning(void){return running;}
static uint32_t host_getMask(void){return irq_mask;}
static void host_disable(void){if(before_commit){void (*fn)(void)=before_commit;before_commit=0;fn();}irq_mask=1;}
static void host_setMask(uint32_t mask){irq_mask=mask;}
#define __get_PRIMASK host_getMask
#define __disable_irq host_disable
#define __set_PRIMASK host_setMask
volatile uint16_t buttonHandler_originalParameter[2];
uint8_t buttonHandler_originalValue[2],buttonHandler_resetLock1,buttonHandler_resetLock2;
static int8_t armed_step=NO_STEP_SELECTED;
int8_t buttonHandler_getArmedAutomationStep(void){return armed_step;}
#include "base.inc"
#include "../../Core/Src/Preset/voiceTransfer.inc"
uint8_t copyClear_Mode;
uint8_t buttonHandler_getCopy(void){return host_copy;}
uint8_t buttonHandler_getShift(void){return host_shift;}
uint8_t buttonHandler_getMode(void){return host_mode;}
uint8_t menu_getActiveVoice(void){return menu_activeVoice;}
void copyClear_reset(void){copyClear_Mode=MODE_NONE;}
void menu_repaintAll(void){}
void screensaver_touch(void){}
void copyClear_executeClear(void){clear_actions++;}
uint8_t copyClear_isClearModeActive(void){return copyClear_Mode==MODE_CLEAR;}
uint8_t copyClear_getClearTarget(void){return 0;}
void copyClear_setClearTarget(uint8_t v){(void)v;}
void menu_handleLoadSaveMenu(int8_t i,uint8_t b){(void)i;(void)b;}
void menu_handleSongMenu(int8_t i,uint8_t b){(void)i;(void)b;}
uint8_t seq_getTrackLength(uint8_t voice){return seq_patternSet.length[seq_activePattern][voice];}
void seq_setTrackLength(uint8_t voice,uint8_t length){seq_patternSet.length[seq_activePattern][voice]=length;}
static void menu_encoderChangeParameter(int8_t i){(void)i;edit_actions++;}
static void menu_moveToMenuItem(int8_t i){(void)i;navigate_actions++;}
void menu_repaint(void){}
#include "../../Core/Src/menu/voiceTransfer.inc"
#include "engine.c"
#include "encoder.inc"
#include "panel.c"
#include "menu/menuPages.h"

static void reset_r21(void)
{
 seed(0);memset(menu_loadedPresetNr,0,sizeof menu_loadedPresetNr);
 menu_loadedPresetNr[CURRENT_KIT]=9;menu_selectedPresetNr[SAVE_TYPE_KIT]=23;
 menu_activeVoice=4;menu_activePage=4;host_mode=SELECT_MODE_VOICE;
 host_copy=host_shift=running=lastEncoderButton=editModeActive=0;
 menu_voiceCopyActive=menu_voiceCopyDrain=menu_voiceCopyNotice=menu_voiceCopyCopyHeld=0;
 memset(&buttonHandler_stateMemory,0,sizeof buttonHandler_stateMemory);
 buttonHandler_stateMemory.selectButtonMode=SELECT_MODE_VOICE;
 seq_loadPendigFlag=0;morph_state=morph_index=0;armed_step=NO_STEP_SELECTED;buttonHandler_resetLock1=buttonHandler_resetLock2=0;
 now=100;irq_mask=0;before_commit=0;
 for(unsigned p=0;p<END_OF_SOUND_PARAMETERS;p++){
  activeParameters[p]=(p*7+11)%128;
  mainParameters[1][p]=(p*3+9)%128;
  mainParameters[9][p]=(p*5+7)%128;
 }
 for(unsigned i=0;i<6;i++){
  activeParameters[PAR_TARGET_LFO1+i]=0;
  activeParameters[PAR_VOICE_LFO1+i]=i+1;
  mainParameters[1][PAR_TARGET_LFO1+i]=0;
  mainParameters[1][PAR_VOICE_LFO1+i]=i+1;
 }
 sends=0;
}
static void chord(void)
{
 panel_press(BUT_COPY);assert(copyClear_Mode==MODE_COPY_TRACK);
 menu_parseEncoder(0,1);assert(menu_voiceCopyActive==1 && !editModeActive && copyClear_Mode==MODE_NONE);
 menu_parseEncoder(0,1);assert(menu_voiceCopyActive==1);
 menu_parseEncoder(0,0);assert(menu_voiceCopyActive==1);
 panel_release(BUT_COPY);menu_parseEncoder(0,0);assert(menu_voiceCopyActive==2);
 now+=40;
}
static void choose_two(void){while(menu_voiceCopyTarget!=1)menu_parseEncoder(-1,0);}
static void confirm(void){menu_parseEncoder(0,1);}
static void start_clock(void){running=1;}
static void storage_roundtrip(void)
{
 reset_r21();uint8_t source[NUM_PARAMS],target[END_OF_SOUND_PARAMETERS],stored_source[END_OF_SOUND_PARAMETERS];
 memcpy(source,activeParameters,sizeof source);memcpy(target,mainParameters[1],sizeof target);
 memcpy(stored_source,mainParameters[9],sizeof stored_source);
 PatternSet *patterns=malloc(sizeof *patterns);memcpy(patterns,&seq_patternSet,sizeof *patterns);
 unsigned writes_before=physical_writes,apis=api_writes,send_before=sends,fx_before=fx_preparations;
 char names_before[sizeof savefileNames];memcpy(names_before,savefileNames,sizeof names_before);
 chord();choose_two();menu_renderVoiceTransfer();assert(!memcmp(editDisplayBuffer[0],"V5 > KIT 02?",12));confirm();
 assert(menu_voiceCopyNotice==1 && !menu_voiceCopyActive);
 assert(!memcmp(source,activeParameters,sizeof source));assert(!memcmp(stored_source,mainParameters[9],sizeof stored_source));
 assert(!memcmp(patterns,&seq_patternSet,sizeof *patterns));free(patterns);
 assert(physical_writes==writes_before && api_writes==apis && sends==send_before && fx_preparations==fx_before);
 assert(!memcmp(names_before,savefileNames,sizeof names_before));
 assert(menu_loadedPresetNr[CURRENT_KIT]==9 && menu_selectedPresetNr[SAVE_TYPE_KIT]==23);
 assert(mainParameters[1][PAR_COARSE5]==source[PAR_COARSE5]);
 assert(mainParameters[1][PAR_AUDIO_OUT5]==target[PAR_AUDIO_OUT5]);
 uint8_t expected[END_OF_SOUND_PARAMETERS];memcpy(expected,mainParameters[1],sizeof expected);
 assert(preset_saveTmpKit()==FR_OK);
 FIL file={0};UINT count;uint8_t slot;
 assert(f_open(&file,"/tmp/tmp.snd",FA_READ)==FR_OK);
 assert(f_read(&file,&slot,1,&count)==FR_OK && count==1 && slot==9);assert(f_close(&file)==FR_OK);
 assert(preset_saveProject(0)==FR_OK);assert(!memcmp(mainParameters[9],source,END_OF_SOUND_PARAMETERS));
 assert(preset_loadProject(0,0)==FR_OK);
 assert(!memcmp(mainParameters[1],expected,sizeof expected));assert(!memcmp(mainParameters[9],source,END_OF_SOUND_PARAMETERS));
 preset_loadDrumsetRam(1,0,1);assert(!memcmp(activeParameters,expected,sizeof expected));
 assert(menu_loadedPresetNr[CURRENT_KIT]==1);
 puts("PASS Kit10 unsaved V5 -> Kit2: production COPY/DATA, exact active/cursor/pattern preservation, zero SD/dispatch at transfer, temp identity, real-FatFs SAVE PROJECT/reload/load");
}

static uint8_t is_excluded(unsigned id)
{
 return id==PAR_NONE || id>=END_OF_SOUND_PARAMETERS ||
  (id>=PAR_FREQ_LFO1 && id<=PAR_AMOUNT_LFO6) ||
  (id>=PAR_VOLUME_MOD_ON_OFF1 && id<=PAR_OFFSET_LFO6) ||
  (id>=PAR_AUDIO_OUT1 && id<=PAR_AUDIO_OUT6);
}
static void all_engines(void)
{
 for(unsigned track=0;track<7;track++){
  reset_r21();menu_activeVoice=menu_activePage=track;
  uint8_t before[64][END_OF_SOUND_PARAMETERS],source[NUM_PARAMS],mask[END_OF_SOUND_PARAMETERS]={0};
  memcpy(before,mainParameters,sizeof before);memcpy(source,activeParameters,sizeof source);
  unsigned first=track<5?track:5,last=track<5?track:6;
  for(unsigned page=first;page<=last;page++)for(unsigned sub=0;sub<8;sub++){
   uint16_t ids[8];memcpy(ids,&menuPages[page][sub].bot1,sizeof ids);
   for(unsigned j=0;j<8;j++)if(!is_excluded(ids[j]))mask[ids[j]]=1;
  }
  chord();choose_two();confirm();assert(menu_voiceCopyNotice==1);
  for(unsigned kit=0;kit<64;kit++)for(unsigned id=0;id<END_OF_SOUND_PARAMETERS;id++)
   assert(mainParameters[kit][id]==(kit==1&&mask[id]?source[id]:before[kit][id]));
  assert(!memcmp(source,activeParameters,sizeof source));
  if(track>=5)assert(mask[PAR_VELOD6_CLOSED]&&mask[PAR_VELOD6_OPEN]);
 }
 puts("PASS all six engines and both HH buttons: independent production menu-ID oracle, exact every-kit-byte mask, both decays, all other voice/FX/output/velocity/morph/placeholder/transport bytes preserved");
}
static uint8_t target_index(unsigned voice,unsigned id)
{
 for(unsigned i=0;i<getNumModTargets();i++)
  if(pgm_read_word(&modTargets[i].param)==id && (voice==0||voiceFromModTargValue(i)==voice))return i;
 assert(!"missing fixture target");return 0;
}
static void lfo_cases(void)
{
 for(unsigned kind=0;kind<8;kind++){
  reset_r21();uint8_t index=target_index(5,PAR_COARSE5);
  activeParameters[PAR_AMOUNT_LFO5]=29;
  if(kind==0)index=0;else if(kind==1)activeParameters[PAR_AMOUNT_LFO5]=0;
  else if(kind==2)index=target_index(1,PAR_COARSE1);
  else if(kind==3)index=target_index(5,PAR_FX_P1);
  else if(kind==4)index=1;else if(kind==5)index=255;
  else if(kind==6)activeParameters[PAR_VOICE_LFO5]=2;
  activeParameters[PAR_TARGET_LFO5]=index;
  uint8_t previous[8];for(unsigned j=0;j<8;j++)previous[j]=mainParameters[1][preset_voiceLfoGroups[j]+4];
  chord();choose_two();confirm();assert(menu_voiceCopyNotice==1);
  for(unsigned j=0;j<8;j++)assert(mainParameters[1][preset_voiceLfoGroups[j]+4]==
   (kind==7?activeParameters[preset_voiceLfoGroups[j]+4]:previous[j]));
 }
 reset_r21();
 uint8_t dest=target_index(5,PAR_COARSE5),other=target_index(5,PAR_FINE5);
 activeParameters[PAR_TARGET_LFO5]=dest;activeParameters[PAR_AMOUNT_LFO5]=37;
 activeParameters[PAR_RETRIGGER_LFO5]=2;
 mainParameters[1][PAR_TARGET_LFO1]=dest;mainParameters[1][PAR_AMOUNT_LFO1]=13;
 mainParameters[1][PAR_TARGET_LFO6]=dest;mainParameters[1][PAR_AMOUNT_LFO6]=0;
 mainParameters[1][PAR_TARGET_LFO2]=other;
 mainParameters[1][PAR_TARGET_LFO5]=target_index(1,PAR_COARSE1);
 uint8_t before[END_OF_SOUND_PARAMETERS];memcpy(before,mainParameters[1],sizeof before);
 chord();choose_two();confirm();assert(menu_voiceCopyNotice==1);
 assert(mainParameters[1][PAR_TARGET_LFO1]==0&&mainParameters[1][PAR_TARGET_LFO6]==0);
 assert(mainParameters[1][PAR_TARGET_LFO2]==other);
 for(unsigned lfo=0;lfo<6;lfo++)if(lfo!=4)for(unsigned j=0;j<8;j++){
  unsigned id=preset_voiceLfoGroups[j]+lfo;
  if(id!=PAR_TARGET_LFO1&&id!=PAR_TARGET_LFO6)assert(mainParameters[1][id]==before[id]);
 }
 assert(preset_saveProject(0)==FR_OK && preset_loadProject(0,0)==FR_OK);
 assert(mainParameters[1][PAR_TARGET_LFO1]==0&&mainParameters[1][PAR_TARGET_LFO6]==0);
 assert(mainParameters[1][PAR_TARGET_LFO2]==other&&mainParameters[1][PAR_TARGET_LFO5]==dest);
 assert(mainParameters[1][PAR_RETRIGGER_LFO5]==2);
 preset_loadDrumsetRam(1,0,1);
 assert(voiceArray[0].lfo.modTarget.destination==PAR_NONE && hatVoice.lfo.modTarget.destination==PAR_NONE);
 assert(voiceArray[1].lfo.modTarget.destination==PAR_FINE5 && cymbalVoice.lfo.modTarget.destination==PAR_COARSE5);
 assert(engine_values[PAR_COARSE5]==activeParameters[PAR_COARSE5]);
 assert(!engine_seen[PAR_NRPN_FINE]&&!engine_seen[PAR_NRPN_COARSE]&&!engine_seen[PAR_BANK_CHANGE]);
 /* Observe the saved retrigger byte from the normal CC2 load sweep, then
  * exercise the actual LFO retrigger function with target-pattern events. */
 cymbalVoice.lfo.retrigger=engine_values[PAR_RETRIGGER_LFO5];
 cymbalVoice.lfo.phase=345;cymbalVoice.lfo.phaseOffset=123;
 lfo_retrigger(0);assert(cymbalVoice.lfo.phase==345);
 lfo_retrigger(1);assert(cymbalVoice.lfo.phase==123);
 puts("PASS LFO OFF/zero/other-voice/FX/global/invalid/voice-field mismatch skips, complete self LFO, exact-destination takeover from earlier/later zero-amount assignments, same-voice different destination retained, real project roundtrip and other-track retrigger bytes");
}
static void rejection_cases(void)
{
 for(unsigned kind=0;kind<10;kind++){
  reset_r21();chord();choose_two();
  uint8_t before[64][END_OF_SOUND_PARAMETERS];memcpy(before,mainParameters,sizeof before);
  if(kind==0)menu_voiceCopyTarget=9;
  if(kind==1)menu_voiceCopyTarget=255;
  if(kind==2)menu_loadedPresetNr[CURRENT_PROJECT]=1;
  if(kind==3){preset_loadDrumsetRam(2,0,0);preset_loadDrumsetRam(9,0,0);}
  if(kind==4)running=1;
  if(kind==5)seq_pendingPattern=1;
  if(kind==6)before_commit=start_clock;
  if(kind==7)menu_voiceCopy.values[PAR_COARSE5]=128;
  if(kind==8)menu_loadedPresetNr[CURRENT_KIT]=2;
  if(kind==9)seq_loadPendigFlag=1;
  confirm();assert(menu_voiceCopyNotice==2&&!menu_voiceCopyActive);
  assert(!memcmp(before,mainParameters,sizeof before));assert(irq_mask==0);
  menu_renderVoiceTransfer();assert(!memcmp(editDisplayBuffer[1],"rejected",8));
 }
 reset_r21();seq_pendingPattern=1;panel_press(BUT_COPY);menu_parseEncoder(0,1);
 assert(!menu_voiceCopyActive&&menu_voiceCopyNotice==2);
 reset_r21();chord();choose_two();uint8_t target[END_OF_SOUND_PARAMETERS];memcpy(target,mainParameters[1],sizeof target);
 panel_press(BUT_COPY);assert(menu_voiceCopyNotice==3&&!menu_voiceCopyActive);
 panel_release(BUT_COPY);assert(!memcmp(target,mainParameters[1],sizeof target));
 for(unsigned j=0;j<END_OF_SOUND_PARAMETERS;j++)assert(menu_voiceCopy.values[j]==0);
 puts("PASS same/invalid kit, project/kit/pattern change, A/B/A reload, external transport and commit-boundary interrupt rejection, invalid source byte, cancel, truthful LCD, snapshot disposal and interrupt mask restore");
}
static void button_edges(void)
{
 reset_r21();uint8_t target[END_OF_SOUND_PARAMETERS];memcpy(target,mainParameters[1],sizeof target);
 panel_press(BUT_COPY);menu_parseEncoder(0,1);
 for(unsigned i=0;i<10;i++){now++;menu_parseEncoder(0,i%2);}
 assert(menu_voiceCopyActive==1&&!memcmp(target,mainParameters[1],sizeof target));
 menu_parseEncoder(0,0);panel_release(BUT_COPY);menu_parseEncoder(0,0);choose_two();
 now+=1;menu_parseEncoder(0,1);assert(menu_voiceCopyActive==2);
 menu_parseEncoder(0,0);now+=40;confirm();assert(menu_voiceCopyNotice==1);
 for(unsigned i=0;i<20;i++){now++;menu_parseEncoder(0,i%2);}
 assert(!editModeActive && menu_voiceCopyNotice==1);
 now+=40;menu_parseEncoder(0,0);now+=40;menu_parseEncoder(0,0);
 menu_parseEncoder(0,1);assert(editModeActive);menu_parseEncoder(1,1);assert(edit_actions);
 menu_parseEncoder(0,0);menu_parseEncoder(0,1);assert(!editModeActive);menu_parseEncoder(1,1);assert(navigate_actions);
 reset_r21();panel_press(BUT_COPY);unsigned copies=track_copies;panel_press(BUT_VOICE_3);assert(track_copies==copies+1);panel_release(BUT_COPY);assert(copyClear_Mode==MODE_NONE);
 panel_press(BUT_SHIFT);panel_press(BUT_COPY);assert(copyClear_Mode==MODE_CLEAR);unsigned clears=clear_actions;menu_parseEncoder(0,1);assert(clear_actions==clears+1&&!menu_voiceCopyActive);panel_release(BUT_COPY);panel_release(BUT_SHIFT);
 reset_r21();panel_press(BUT_REC);assert(buttonHandler_stateMemory.seqRecording);unsigned steps=step_calls;panel_press(BUT_SEQ1);assert(step_calls==steps+1);
 reset_r21();chord();panel_press(BUT_START_STOP);assert(running);choose_two();confirm();assert(menu_voiceCopyNotice==2);
 puts("PASS entry press/release and repeated/bouncing edges, held DATA, fresh confirmation, post-success bounce drain, normal DATA/edit/navigation, COPY/voice dispatch, SHIFT+CLEAR, REC/STEP dispatch, START/STOP rejection");
}


static void legacy_and_load(void)
{
 reset_r21();uint8_t legacy[100];memset(legacy,21,sizeof legacy);
 FIL file={0};UINT count;
 assert(f_open(&file,"/tmp/02-LEGCY.snd",FA_CREATE_NEW|FA_WRITE)==FR_OK);
 assert(f_write(&file,"LEGACY!!",8,&count)==FR_OK&&count==8);
 assert(f_write(&file,legacy,sizeof legacy,&count)==FR_OK&&count==sizeof legacy);
 assert(f_close(&file)==FR_OK);
 assert(preset_loadDrumsetSD(2,0)==FR_OK);
 assert(mainParameters[2][PAR_FX_P1]==0 && mainParameters[2][PAR_FILTER_DRIVE_5]==0);
 PresetVoiceCopy copy;assert(preset_captureVoice(&copy,4));assert(preset_transferVoice(&copy,2));
 assert(mainParameters[2][PAR_COARSE5]==activeParameters[PAR_COARSE5] && mainParameters[2][PAR_FX_P1]==0);
 seq_activePattern=1;seq_patternSet.kitNr[1]=2;activeParameters[PAR_PATTERN_MODE]=1;
 #include "automatic.inc"
 assert(menu_loadedPresetNr[CURRENT_KIT]==2&&menu_selectedPresetNr[SAVE_TYPE_KIT]==2);
 assert(engine_values[PAR_COARSE5]==legacy[PAR_COARSE5] || engine_values[PAR_COARSE5]==copy.values[PAR_COARSE5]);
 assert(engine_values[PAR_FILTER_DRIVE_5]==copy.values[PAR_FILTER_DRIVE_5]);
 memset(engine_seen,0,sizeof engine_seen);
 preset_loadDrumsetRam(2,0,0);while(morph_state)preset_morph_tick();
 assert(engine_values[PAR_AMOUNT_LFO6]==activeParameters[PAR_AMOUNT_LFO6]);
 assert(engine_values[PAR_FILTER_DRIVE_1]==activeParameters[PAR_FILTER_DRIVE_1]);
 assert(!engine_seen[PAR_BANK_CHANGE]&&!engine_seen[PAR_NRPN_FINE]&&!engine_seen[PAR_NRPN_COARSE]&&!engine_seen[PAR_NRPN_DATA_ENTRY_COARSE]);
 puts("PASS short compatible kit, zero tail preservation, actual pattern-linked auto-load block, destination decoding and normal/immediate low CC127/CC2 boundary without reserved transport sends");
}

static void begin_morph_at_commit(void){preset_morph(0);}
static void morph_alias(void)
{
 for(unsigned mode=0;mode<6;mode++){
  reset_r21();morphParameters=mainParameters[1];
  assert(mainParameters[1][PAR_COARSE5]==57 && activeParameters[PAR_COARSE5]==123);
  activeParameters[PAR_MORPH]=mode==1||mode==3||mode==5?0:127;
  morph_state=mode<2||mode==4?1:0;morph_index=3;
  uint8_t before[64][END_OF_SOUND_PARAMETERS],source[NUM_PARAMS];
  memcpy(before,mainParameters,sizeof before);memcpy(source,activeParameters,sizeof source);
  uint8_t state_before=morph_state,index_before=morph_index;
  unsigned io=api_writes,sectors=physical_writes,dispatches=sends,fx=fx_preparations;
  unsigned values_before[END_OF_SOUND_PARAMETERS];memcpy(values_before,engine_seen,sizeof values_before);
  chord();choose_two();if(mode==4)menu_voiceCopyTarget=2;
  if(mode==5)before_commit=begin_morph_at_commit;
  confirm();
  if(mode==3||mode==4){
   assert(menu_voiceCopyNotice==1);
   assert(mainParameters[mode==4?2:1][PAR_COARSE5]==123);
  }else{
   assert(menu_voiceCopyNotice==4);menu_renderVoiceTransfer();
   assert(!memcmp(editDisplayBuffer[1],"Morph active",12));
   assert(!memcmp(before,mainParameters,sizeof before));
  }
  assert(!memcmp(source,activeParameters,sizeof source));
  if(mode!=5)assert(morph_state==state_before&&morph_index==index_before);
  assert(io==api_writes&&sectors==physical_writes&&dispatches==sends&&fx==fx_preparations);
  assert(!memcmp(values_before,engine_seen,sizeof values_before));
  assert(morphParameters==mainParameters[1]);
  printf("PASS Morph57/123 mode%u: amount%u pending%u, %s, zero transfer side effects\n",mode,
    activeParameters[PAR_MORPH],morph_state,mode==3||mode==4?"allowed":"Morph active rejection");
 }
 /* Let the zero-amount pending sweep complete through its production path. */
 reset_r21();morphParameters=mainParameters[1];preset_morph(0);
 chord();choose_two();confirm();assert(menu_voiceCopyNotice==4);
 while(morph_state)preset_morph_tick();
 now+=100;menu_parseEncoder(0,0);now+=100;menu_parseEncoder(0,0);
 chord();choose_two();confirm();assert(menu_voiceCopyNotice==1&&mainParameters[1][PAR_COARSE5]==123);
 puts("PASS zero-amount pending sweep rejected until actual completion, then transfer succeeds");
}
int main(int argc,char **argv){(void)argv;paramToModTargetInit();if(argc>1){morph_alias();return 0;}storage_roundtrip();all_engines();lfo_cases();rejection_cases();button_edges();legacy_and_load();morph_alias();
 reset_r21();armed_step=1;buttonHandler_resetLock1=1;buttonHandler_originalParameter[0]=PAR_COARSE5;
 buttonHandler_originalValue[0]=32;activeParameters[PAR_COARSE5]=111;
 chord();choose_two();confirm();assert(mainParameters[1][PAR_COARSE5]==32&&activeParameters[PAR_COARSE5]==111);
 puts("PASS held STEP snapshot excludes temporary parameter locks without changing active buffer or recording automation");return 0;}
