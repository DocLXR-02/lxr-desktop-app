/* Existing R21 fixture supplies production panel/menu/storage/load paths. */
static void r22_reset(unsigned source)
{
 reset_r21();menu_voiceCopyVoiceHeld=0;
 menu_activePage=menu_activeVoice=source;
}
static void target_drum(unsigned target)
{
 unsigned calls=voice_calls,copies=track_copies;uint8_t source=menu_activeVoice,page=menu_activePage;
 while(menu_voiceCopyFocus!=VOICE_COPY_DRUM)menu_parseEncoder(menu_voiceCopyFocus<VOICE_COPY_DRUM?1:-1,0);
 if(!menu_voiceCopyEditing)data_click();
 while(menu_voiceCopyTargetVoice!=target)menu_parseEncoder(menu_voiceCopyTargetVoice<target?1:-1,0);
 data_click();
 assert(menu_voiceCopyActive&&menu_voiceCopyTargetVoice==target&&!menu_voiceCopyEditing);
 assert(menu_activeVoice==source&&menu_activePage==page&&voice_calls==calls&&track_copies==copies);
}
static void menu_oracle(unsigned source,unsigned target,uint8_t *expected)
{
 /* Matching production menu field positions define semantic correspondence.
  * This does not use the new remapping table or numerical ID offsets. */
 unsigned count=0;uint8_t seen[END_OF_SOUND_PARAMETERS]={0};
 for(unsigned page=0;page<8;page++){
  uint16_t from[8],to[8];
  memcpy(from,&menuPages[source][page].bot1,sizeof from);
  memcpy(to,&menuPages[target][page].bot1,sizeof to);
  for(unsigned field=0;field<8;field++)if(!is_excluded(from[field])){
   assert(!is_excluded(to[field]));
   expected[to[field]]=activeParameters[from[field]];
   if(!seen[to[field]]){seen[to[field]]=1;count++;}
  }
 }
 assert(count==25);
}
static void r22_nine_pairs(void)
{
 for(unsigned source=0;source<3;source++)for(unsigned target=0;target<3;target++){
  r22_reset(source);
  uint8_t expected[END_OF_SOUND_PARAMETERS],active[NUM_PARAMS],kits[sizeof mainParameters];
  memcpy(expected,mainParameters[1],sizeof expected);menu_oracle(source,target,expected);
  memcpy(active,activeParameters,sizeof active);memcpy(kits,mainParameters,sizeof kits);
  PatternSet *patterns=malloc(sizeof *patterns);memcpy(patterns,&seq_patternSet,sizeof *patterns);
  char names[sizeof savefileNames];memcpy(names,savefileNames,sizeof names);
  uint8_t songs[sizeof seq_songData];memcpy(songs,seq_songData,sizeof songs);
  unsigned writes=api_writes,sectors=physical_writes,dispatch=sends,fx=fx_preparations;
  chord();assert(menu_voiceCopyTargetVoice==source);choose_two();target_drum(target);
  menu_renderVoiceTransfer();char line[17];snprintf(line,sizeof line,"D%u > K02 D%u  OK ",source+1,target+1);
  assert(!memcmp(editDisplayBuffer[1],line,strlen(line)));
  confirm();assert(menu_voiceCopyNotice==1);
  assert(!memcmp(mainParameters[1],expected,sizeof expected));
  memcpy(kits+sizeof expected,expected,sizeof expected);
  assert(!memcmp(mainParameters,kits,sizeof kits));
  assert(!memcmp(activeParameters,active,sizeof active));
  assert(!memcmp(patterns,&seq_patternSet,sizeof *patterns));free(patterns);
  assert(!memcmp(names,savefileNames,sizeof names)&&!memcmp(songs,seq_songData,sizeof songs));
  assert(writes==api_writes&&sectors==physical_writes&&dispatch==sends&&fx==fx_preparations);
  assert(menu_loadedPresetNr[CURRENT_KIT]==9&&menu_selectedPresetNr[SAVE_TYPE_KIT]==23);
  printf("PASS R22 cross-kit D%u->D%u:25 menu-oracle fields, exact all-kit/active/pattern/global preservation, zero transfer dispatch/SD\n",source+1,target+1);
 }
}
static void r22_same_kit_and_invalid(void)
{
 for(unsigned source=0;source<3;source++)for(unsigned target=0;target<3;target++){
  r22_reset(source);uint8_t kits[sizeof mainParameters],active[NUM_PARAMS];
  memcpy(kits,mainParameters,sizeof kits);memcpy(active,activeParameters,sizeof active);
  unsigned io=api_writes,dispatch=sends,fx=fx_preparations;
  chord();target_drum(target);confirm();assert(menu_voiceCopyNotice==2);
  assert(!memcmp(kits,mainParameters,sizeof kits)&&!memcmp(active,activeParameters,sizeof active));
  assert(io==api_writes&&dispatch==sends&&fx==fx_preparations);
 }
 for(unsigned source=0;source<6;source++)for(unsigned target=0;target<8;target++){
  r22_reset(source);PresetVoiceCopy copy;assert(preset_captureVoice(&copy,source));
  if(target>=6||(source!=target&&(source>=3||target>=3)))
   assert(preset_transferVoiceTo(&copy,1,target)==PRESET_VOICE_REJECTED);
 }
 assert(preset_transferVoiceTo(NULL,1,0)==PRESET_VOICE_REJECTED);
 puts("PASS R22 all nine same-kit pairs rejected; invalid/cross-type target and null snapshot rejected");
}
static void r22_lfo_cases(void)
{
 for(unsigned source=0;source<3;source++)for(unsigned target=0;target<3;target++){
  r22_reset(source);
  /* Exercise a high CC2 destination as well as low parameter IDs. */
  uint16_t src_id[3]={PAR_FILTER_DRIVE_1,PAR_FILTER_DRIVE_2,PAR_FILTER_DRIVE_3};
  uint16_t dst_id=src_id[target];
  activeParameters[PAR_AMOUNT_LFO1+source]=63;
  activeParameters[PAR_TARGET_LFO1+source]=target_index(source+1,src_id[source]);
  activeParameters[PAR_RETRIGGER_LFO1+source]=7; /* Preserve chosen other track. */
  uint8_t expected[END_OF_SOUND_PARAMETERS];memcpy(expected,mainParameters[1],sizeof expected);
  menu_oracle(source,target,expected);
  for(unsigned group=0;group<8;group++)expected[preset_voiceLfoGroups[group]+target]=activeParameters[preset_voiceLfoGroups[group]+source];
  expected[PAR_VOICE_LFO1+target]=target+1;
  expected[PAR_TARGET_LFO1+target]=target_index(target+1,dst_id);
  /* Earlier and later competitors include a zero-amount assignment. */
  unsigned early=target==0?1:0,late=5;
  mainParameters[1][PAR_TARGET_LFO1+early]=target_index(target+1,dst_id);
  mainParameters[1][PAR_AMOUNT_LFO1+early]=0;
  mainParameters[1][PAR_TARGET_LFO1+late]=target_index(target+1,dst_id);
  mainParameters[1][PAR_AMOUNT_LFO1+late]=35;
  unsigned other=3;
  mainParameters[1][PAR_TARGET_LFO1+other]=target_index(target+1,PAR_VOL1+target);
  /* Restore non-target LFO fields in the independent expected kit. */
  for(unsigned i=0;i<6;i++)if(i!=target)for(unsigned group=0;group<8;group++){
   unsigned id=preset_voiceLfoGroups[group]+i;expected[id]=mainParameters[1][id];
  }
  expected[PAR_TARGET_LFO1+early]=expected[PAR_TARGET_LFO1+late]=0;
  unsigned io=api_writes,dispatch=sends,fx=fx_preparations;
  chord();choose_two();target_drum(target);confirm();assert(menu_voiceCopyNotice==1);
  assert(!memcmp(expected,mainParameters[1],sizeof expected));
  assert(io==api_writes&&dispatch==sends&&fx==fx_preparations);
  assert(preset_saveProject(0)==FR_OK&&preset_loadProject(0,0)==FR_OK);
  assert(!memcmp(expected,mainParameters[1],sizeof expected));
  preset_loadDrumsetRam(1,0,1);
  assert(voiceArray[target].lfo.modTarget.destination==dst_id);
  assert(engine_values[PAR_RETRIGGER_LFO1+target]==7);
  assert(engine_values[PAR_FILTER_DRIVE_1+target]==expected[PAR_FILTER_DRIVE_1+target]);
  printf("PASS R22 LFO D%u->D%u: decoded high-ID remap, all8 fields, exact conflicts/OFF, independent same-voice destination, project/load dispatch\n",source+1,target+1);
 }
 for(unsigned kind=0;kind<7;kind++){
  r22_reset(0);activeParameters[PAR_AMOUNT_LFO1]=64;
  activeParameters[PAR_TARGET_LFO1]=target_index(1,PAR_COARSE1);
  if(kind==0)activeParameters[PAR_TARGET_LFO1]=0;
  if(kind==1)activeParameters[PAR_AMOUNT_LFO1]=0;
  if(kind==2)activeParameters[PAR_TARGET_LFO1]=target_index(2,PAR_COARSE2);
  if(kind==3)activeParameters[PAR_TARGET_LFO1]=target_index(1,PAR_FX_P1);
  if(kind==4)activeParameters[PAR_TARGET_LFO1]=1;
  if(kind==5)activeParameters[PAR_TARGET_LFO1]=255;
  if(kind==6)activeParameters[PAR_VOICE_LFO1]=2;
  uint8_t prior[8];for(unsigned i=0;i<8;i++)prior[i]=mainParameters[1][preset_voiceLfoGroups[i]+1];
  chord();choose_two();target_drum(1);confirm();assert(menu_voiceCopyNotice==1);
  for(unsigned i=0;i<8;i++)assert(mainParameters[1][preset_voiceLfoGroups[i]+1]==prior[i]);
 }
 puts("PASS R22 OFF/zero/other-voice/FX/global/invalid/voice-mismatch retain destination LFO");
}
static uint16_t semantic_destination(unsigned source,unsigned target,uint16_t id)
{
 for(unsigned page=0;page<8;page++){
  uint16_t from[8],to[8];memcpy(from,&menuPages[source][page].bot1,sizeof from);memcpy(to,&menuPages[target][page].bot1,sizeof to);
  for(unsigned field=0;field<8;field++)if(id!=PAR_NONE&&from[field]==id&&to[field]<END_OF_SOUND_PARAMETERS)return to[field];
 }
 return PAR_NONE;
}
static void r22_all_lfo_destinations(void)
{
 unsigned count=0;
 for(unsigned source=0;source<3;source++)for(unsigned target=0;target<3;target++)
  for(unsigned index=0;index<getNumModTargets();index++){
   uint16_t id=pgm_read_word(&modTargets[index].param);
   uint16_t mapped=semantic_destination(source,target,id);
   if(voiceFromModTargValue(index)!=source+1||mapped==PAR_NONE||id==PAR_NONE)continue;
   r22_reset(source);activeParameters[PAR_AMOUNT_LFO1+source]=64;
   activeParameters[PAR_TARGET_LFO1+source]=index;
   PresetVoiceCopy copy;assert(preset_captureVoice(&copy,source));
   assert(preset_transferVoiceTo(&copy,1,target)==PRESET_VOICE_OK);
   uint8_t encoded=mainParameters[1][PAR_TARGET_LFO1+target];
   assert(encoded<getNumModTargets()&&pgm_read_word(&modTargets[encoded].param)==mapped);
   assert(voiceFromModTargValue(encoded)==target+1&&mainParameters[1][PAR_VOICE_LFO1+target]==target+1);
   count++;
  }
 assert(count==306);
 printf("PASS R22 %u own-LFO destination mappings using independent menu semantics and real target tables, including rate/sync/wave/amount/retrigger/offset/velocity targets\n",count);
}
static void r22_persistence(void)
{
 for(unsigned source=0;source<3;source++)for(unsigned target=0;target<3;target++){
  r22_reset(source);
  armed_step=1;buttonHandler_resetLock1=1;
  uint16_t source_id[3]={PAR_COARSE1,PAR_COARSE2,PAR_COARSE3};
  uint16_t target_id[3]={PAR_COARSE1,PAR_COARSE2,PAR_COARSE3};
  buttonHandler_originalParameter[0]=source_id[source];buttonHandler_originalValue[0]=113;
  activeParameters[source_id[source]]=7; /* Temporary held-STEP value. */
  chord();choose_two();target_drum(target);confirm();assert(menu_voiceCopyNotice==1);
  assert(mainParameters[1][target_id[target]]==113&&activeParameters[source_id[source]]==7);
  /* The unchanged release cleanup restores the editable source base before save. */
  activeParameters[source_id[source]]=buttonHandler_originalValue[0];armed_step=NO_STEP_SELECTED;buttonHandler_resetLock1=0;
  uint8_t expected[END_OF_SOUND_PARAMETERS],source_expected[END_OF_SOUND_PARAMETERS];
  memcpy(expected,mainParameters[1],sizeof expected);memcpy(source_expected,activeParameters,sizeof source_expected);
  assert(preset_saveTmpKit()==FR_OK);
  memset(activeParameters,0,END_OF_SOUND_PARAMETERS);
  assert(preset_LoadTmpKit()==FR_OK&&menu_loadedPresetNr[CURRENT_KIT]==9);
  assert(!memcmp(activeParameters,source_expected,sizeof source_expected));
  menu_selectedPresetNr[SAVE_TYPE_KIT]=1; /* Cursor aliases destination; identity must remain source9. */
  assert(preset_saveProject(0)==FR_OK&&preset_loadProject(0,0)==FR_OK);
  assert(!memcmp(mainParameters[1],expected,sizeof expected));
  assert(!memcmp(mainParameters[9],source_expected,sizeof source_expected));
  activeParameters[PAR_MORPH]=0;
  preset_loadDrumsetRam(1,0,0);while(morph_state)preset_morph_tick();
  assert(engine_values[target_id[target]]==113);
  preset_loadDrumsetRam(1,0,1);assert(engine_values[target_id[target]]==113);
 }
 puts("PASS R22 all nine mappings: STEP base snapshot, unsaved source, source temp recovery, cursor aliases target, real project SD roundtrip, source preserved, normal/immediate loads");
}
static void r22_panel_and_morph(void)
{
 r22_reset(2);chord();choose_two();target_drum(0);menu_renderVoiceTransfer();assert(!memcmp(editDisplayBuffer[1],"D3 > K02 D1",10));
 unsigned calls=voice_calls;panel_press(BUT_COPY);assert(menu_voiceCopyActive);panel_release(BUT_COPY);
 confirm();assert(menu_voiceCopyNotice==1&&voice_calls==calls&&menu_activeVoice==2&&!editModeActive);
 r22_reset(0);chord();choose_two();target_drum(2);panel_press(BUT_MODE1);assert(!menu_voiceCopyActive&&!menu_voiceCopyNotice);panel_release(BUT_MODE1);
 r22_reset(4);chord();choose_two();panel_press(BUT_VOICE_1);assert(!menu_voiceCopyActive&&menu_voiceCopyNotice==3&&menu_activeVoice==4);panel_release(BUT_VOICE_1);
 /* Existing 57->123 Morph case uses D5. This remapped case protects an alias
  * before/at commit while leaving an unrelated target permitted. */
 for(unsigned mode=0;mode<6;mode++){
  r22_reset(0);morphParameters=mainParameters[1];
  activeParameters[PAR_MORPH]=mode==1||mode==2||mode==5?0:127;
  morph_state=mode<2?1:0;
  uint8_t before[sizeof mainParameters],active[NUM_PARAMS];memcpy(before,mainParameters,sizeof before);memcpy(active,activeParameters,sizeof active);
  unsigned dispatch=sends,fx=fx_preparations,writes=api_writes;
  chord();choose_two();target_drum(2);
  if(mode==4)menu_voiceCopyTarget=2;
  if(mode==5)before_commit=begin_morph_at_commit;
  confirm();
  if(mode==2||mode==4)assert(menu_voiceCopyNotice==1);
  else {assert(menu_voiceCopyNotice==4);assert(!memcmp(before,mainParameters,sizeof before));}
  assert(!memcmp(active,activeParameters,sizeof active)&&dispatch==sends&&fx==fx_preparations&&writes==api_writes);
 }
 r22_reset(0);chord();choose_two();target_drum(1);seq_loadPendigFlag=1;confirm();assert(menu_voiceCopyNotice==2);
 r22_reset(0);chord();choose_two();target_drum(1);before_commit=start_clock;confirm();assert(menu_voiceCopyNotice==2);
 puts("PASS R22 encoder destination editing, ignored COPY, VOICE cancellation, non-Drum cancellation, remapped Morph alias/zero/pending/unrelated/commit guard and pending-pattern/clock rejection");
}
#include "navigation_cases.c"
int main(void)
{
 r22_nine_pairs();r22_same_kit_and_invalid();r22_lfo_cases();r22_all_lfo_destinations();r22_persistence();r22_panel_and_morph();
 navigation_suite();
 return 0;
}
