/* TEST02: drive production public panel/encoder dispatchers independently of
 * choose_two/target_drum/confirm. Counters wrap the real transaction and LEDs;
 * data/engine/storage snapshots detect actions before explicit OK. */
typedef struct {
 uint8_t kits[sizeof mainParameters],active[sizeof activeParameters];
 uint8_t engine[sizeof engine_values];
 unsigned seen[sizeof engine_seen/sizeof *engine_seen];
 PatternSet patterns;
 unsigned calls,dispatch,fx,writes,sectors;
} NavigationSnapshot;
static NavigationSnapshot *navigation_snapshot(void)
{
 NavigationSnapshot *s=malloc(sizeof *s);assert(s);
 memcpy(s->kits,mainParameters,sizeof s->kits);memcpy(s->active,activeParameters,sizeof s->active);
 memcpy(s->engine,engine_values,sizeof s->engine);memcpy(s->seen,engine_seen,sizeof s->seen);
 memcpy(&s->patterns,&seq_patternSet,sizeof s->patterns);
 s->calls=transfer_calls;s->dispatch=sends;s->fx=fx_preparations;s->writes=api_writes;s->sectors=physical_writes;
 return s;
}
static void navigation_unchanged(const NavigationSnapshot *s)
{
 assert(!memcmp(s->kits,mainParameters,sizeof s->kits)&&!memcmp(s->active,activeParameters,sizeof s->active));
 assert(!memcmp(s->engine,engine_values,sizeof s->engine)&&!memcmp(s->seen,engine_seen,sizeof s->seen));
 assert(!memcmp(&s->patterns,&seq_patternSet,sizeof s->patterns));
 assert(s->calls==transfer_calls&&s->dispatch==sends&&s->fx==fx_preparations&&s->writes==api_writes&&s->sectors==physical_writes);
}
static void navigation_row(const char *expected,uint8_t cursor)
{
 assert(strlen(expected)==16);
 memset(editDisplayBuffer,0x5a,sizeof editDisplayBuffer);
 assert(menu_renderVoiceTransfer());
 assert(!memcmp(editDisplayBuffer[0],"copy voice to:  ",16));
 assert(!memcmp(editDisplayBuffer[1],expected,16)&&visibleCursor==cursor);
 assert(editDisplayBuffer[0][16]==0x5a&&editDisplayBuffer[1][16]==0x5a);
 assert(!memchr(editDisplayBuffer[0],0,16)&&!memchr(editDisplayBuffer[1],0,16));
}
static void navigation_stages(unsigned stage)
{
 if(stage==0)return; /* Entry chord still held. */
 chord();
 if(stage==2)data_click(); /* Kit editor. */
 if(stage>=3){menu_parseEncoder(1,0);if(stage==4)data_click();}
 if(stage==5)menu_parseEncoder(1,0); /* OK for a Drum source. */
}
static void navigation_entry_gates(void)
{
 for(unsigned source=0;source<7;source++)for(unsigned blocked=0;blocked<6;blocked++){
  r22_reset(source);
  if(blocked==0)running=1;
  if(blocked==1)buttonHandler_setMode(SELECT_MODE_PERF);
  if(blocked==2)host_shift=1;
  if(blocked==3)menu_activePage=LOAD_PAGE;
  if(blocked==4)screensaver_Active=1;
  if(blocked!=5)panel_press(BUT_COPY);
  unsigned count=transfer_calls;
  menu_parseEncoder(0,1);menu_parseEncoder(0,0);
  assert(!menu_voiceCopyActive&&transfer_calls==count);
  screensaver_Active=0;
 }
 r22_reset(0);panel_press(BUT_COPY);menu_parseEncoder(0,1);
 menu_parseEncoder(0,0);panel_release(BUT_COPY);menu_parseEncoder(0,0);
 unsigned count=transfer_calls;
 menu_parseEncoder(0,1); /* Chord release cannot immediately edit/confirm. */
 assert(!menu_voiceCopyEditing&&menu_voiceCopyActive&&transfer_calls==count);
 menu_parseEncoder(0,0);now+=39;menu_parseEncoder(0,1);assert(!menu_voiceCopyEditing);
 menu_parseEncoder(0,0);now+=40;menu_parseEncoder(0,1);assert(menu_voiceCopyEditing&&transfer_calls==count);
 puts("PASS TEST02 all seven source entry gates: stopped/VOICE/unshifted/COPY/Voice-page/screensaver; initial chord and sub-40ms DATA bounce never edit or transfer");
}
static void navigation_fields(void)
{
 for(unsigned source=0;source<7;source++){
  r22_reset(source);NavigationSnapshot *s=navigation_snapshot();chord();
  unsigned voice=source<6?source:5;
  assert(menu_voiceCopyFocus==VOICE_COPY_KIT&&!menu_voiceCopyEditing&&menu_voiceCopyTarget==9&&menu_voiceCopyTargetVoice==voice);
  for(unsigned n=0;n<4;n++)menu_parseEncoder(-1,0);
  assert(menu_voiceCopyFocus==VOICE_COPY_KIT);
  char row[17];snprintf(row,sizeof row,source<3?"D%u > K09 D%u  OK ":"V%u > K09     OK ",voice+1,voice+1);
  navigation_row(row,VIS_CURS(1,5,1));
  data_click();assert(menu_voiceCopyEditing);
  snprintf(row,sizeof row,source<3?"D%u > [K09] D%u OK":"V%u > [K09]   OK ",voice+1,voice+1);
  navigation_row(row,0);
  for(unsigned n=0;n<70;n++)menu_parseEncoder(-1,0);
  assert(menu_voiceCopyTarget==0);
  snprintf(row,sizeof row,source<3?"D%u > [K00] D%u OK":"V%u > [K00]   OK ",voice+1,voice+1);navigation_row(row,0);
  for(unsigned n=0;n<70;n++)menu_parseEncoder(1,0);
  assert(menu_voiceCopyTarget==63);
  snprintf(row,sizeof row,source<3?"D%u > [K63] D%u OK":"V%u > [K63]   OK ",voice+1,voice+1);navigation_row(row,0);
  data_click();assert(!menu_voiceCopyEditing&&menu_voiceCopyFocus==VOICE_COPY_KIT);
  menu_parseEncoder(1,0);
  if(source<3){
   assert(menu_voiceCopyFocus==VOICE_COPY_DRUM);
   snprintf(row,sizeof row,"D%u > K63 D%u  OK ",voice+1,voice+1);navigation_row(row,VIS_CURS(1,9,1));
   data_click();assert(menu_voiceCopyEditing);
   for(unsigned n=0;n<5;n++)menu_parseEncoder(-1,0);
   assert(menu_voiceCopyTargetVoice==0);
   snprintf(row,sizeof row,"D%u > K63 [D1] OK",voice+1);navigation_row(row,0);
   for(unsigned n=0;n<5;n++)menu_parseEncoder(1,0);
   assert(menu_voiceCopyTargetVoice==2);
   snprintf(row,sizeof row,"D%u > K63 [D3] OK",voice+1);navigation_row(row,0);
   data_click();assert(!menu_voiceCopyEditing);menu_parseEncoder(1,0);
  }
  assert(menu_voiceCopyFocus==VOICE_COPY_OK);
  for(unsigned n=0;n<4;n++)menu_parseEncoder(1,0);
  assert(menu_voiceCopyFocus==VOICE_COPY_OK);
  snprintf(row,sizeof row,source<3?"D%u > K63 D3 >OK ":"V%u > K63    >OK ",voice+1);navigation_row(row,0);
  menu_parseEncoder(-1,0);assert(menu_voiceCopyFocus==(source<3?VOICE_COPY_DRUM:VOICE_COPY_KIT));
  if(source<3)menu_parseEncoder(-1,0);
  assert(menu_voiceCopyFocus==VOICE_COPY_KIT&&menu_activeVoice==source&&menu_activePage==source);
  navigation_unchanged(s);free(s);
 }
 puts("PASS TEST02 seven source pages: exact 16-cell rows/guards/cursor, immutable source, bounded bidirectional Kit/Drum/OK navigation/editing, K00/K63/D1/D3, non-Drum omission, no premature transaction or data/engine/SD effects");
}
static void navigation_mode_cancel(void)
{
 for(unsigned source=0;source<7;source++)for(unsigned stage=0;stage<6;stage++){
  if(source>=3&&(stage==3||stage==4))continue;
  r22_reset(source);unsigned modes=mode_calls,voices=voice_calls;
  if(stage==0){panel_press(BUT_COPY);menu_parseEncoder(0,1);}
  else if(source<3)navigation_stages(stage);
  else {chord();if(stage==2)data_click();if(stage==5)menu_parseEncoder(1,0);}
  NavigationSnapshot *s=navigation_snapshot();
  panel_press(BUT_MODE1);
  assert(!menu_voiceCopyActive&&!menu_voiceCopyNotice&&!editModeActive&&menu_voiceCopyDrain);
  assert(buttonHandler_getMode()==SELECT_MODE_VOICE&&menu_activePage==source&&menu_activeVoice==source&&mode_calls==modes&&voice_calls==voices);
  /* Held/repeated mode and DATA, long holds then falling-edge/bounce. */
  panel_press(BUT_MODE1);now+=100;menu_parseEncoder(1,1);panel_release(BUT_MODE1);
  now+=1;panel_press(BUT_MODE1);panel_release(BUT_MODE1);
  now+=100;menu_parseEncoder(1,1);assert(menu_voiceCopyDrain&&!editModeActive);
  panel_release(BUT_COPY);menu_parseEncoder(0,0);now+=39;menu_parseEncoder(0,0);
  assert(menu_voiceCopyDrain&&!editModeActive);now+=1;menu_parseEncoder(0,0);
  assert(!menu_voiceCopyDrain&&!editModeActive&&mode_calls==modes&&menu_activePage==source);
  navigation_unchanged(s);free(s);
  /* A fresh normal DATA click works after the guard; fresh normal VOICE
   * exercises the actual production Voice case, whose STEP toggle is intact. */
  menu_parseEncoder(0,1);assert(editModeActive);menu_parseEncoder(0,0);
  panel_press(BUT_MODE1);assert(mode_calls==modes+1&&menu_activePage==SEQ_PAGE);
  panel_release(BUT_MODE1);
 }
 puts("PASS TEST02 VOICE cancels every stage/source (including held entry/open HH), actual mode setter/normal Voice-to-STEP positive control, held/repeated/bounced mode and DATA drained, exact RAM/engine/pattern/SD preservation");
}
static void navigation_copy_and_instruments(void)
{
 for(unsigned stage=1;stage<6;stage++){
  r22_reset(0);navigation_stages(stage);NavigationSnapshot *s=navigation_snapshot();
  uint8_t focus=menu_voiceCopyFocus,editing=menu_voiceCopyEditing;
  unsigned copies=track_copies,voices=voice_calls;
  for(unsigned n=0;n<3;n++){
   panel_press(BUT_COPY);panel_press(BUT_COPY);panel_release(BUT_COPY);
   assert(menu_voiceCopyActive&&menu_voiceCopyFocus==focus&&menu_voiceCopyEditing==editing);
   assert(!blink_leds&&copyClear_Mode==MODE_NONE&&track_copies==copies&&voice_calls==voices);
  }
  panel_press(BUT_COPY);panel_press(BUT_MODE1);panel_release(BUT_MODE1);
  now+=100;menu_parseEncoder(0,0);assert(menu_voiceCopyDrain);
  panel_release(BUT_COPY);now+=39;menu_parseEncoder(0,0);assert(menu_voiceCopyDrain);
  now+=1;menu_parseEncoder(0,0);assert(!menu_voiceCopyDrain);
  navigation_unchanged(s);free(s);
  panel_press(BUT_COPY);assert(copyClear_Mode==MODE_COPY_TRACK);
  panel_release(BUT_COPY);assert(copyClear_Mode==MODE_NONE&&!blink_leds);
 }
 for(unsigned key=BUT_VOICE_1;key<=BUT_VOICE_7;key++)for(unsigned stage=1;stage<6;stage++){
  r22_reset(0);navigation_stages(stage);NavigationSnapshot *s=navigation_snapshot();
  unsigned voices=voice_calls,copies=track_copies;uint8_t target=menu_voiceCopyTargetVoice;
  panel_press(key);panel_press(key);assert(!menu_voiceCopyActive&&menu_voiceCopyNotice==3);
  now+=100;menu_parseEncoder(0,0);assert(menu_voiceCopyDrain);
  panel_release(key);menu_parseEncoder(0,0);now+=40;menu_parseEncoder(0,0);
  assert(!menu_voiceCopyDrain&&!editModeActive&&menu_activeVoice==0&&menu_voiceCopyTargetVoice==target&&voice_calls==voices&&track_copies==copies);
  navigation_unchanged(s);free(s);
 }
 puts("PASS TEST02 COPY ignored in every editor/focus, held COPY drained at cancel, actual copy reset clears blink state and normal COPY restored; all seven instrument buttons cancel/consume across stages without retarget/track copy");
}
static void navigation_ok_and_outside(void)
{
 r22_reset(0);chord();
 data_click();for(unsigned n=0;n<8;n++)menu_parseEncoder(-1,0);data_click(); /* Kit2. */
 menu_parseEncoder(1,0);data_click();menu_parseEncoder(1,0);menu_parseEncoder(1,0);data_click(); /* D3. */
 menu_parseEncoder(1,0);assert(menu_voiceCopyFocus==VOICE_COPY_OK&&menu_voiceCopyTarget==1&&menu_voiceCopyTargetVoice==2);
 unsigned count=transfer_calls,writes=api_writes,dispatch=sends,fx=fx_preparations;
 panel_press(BUT_COPY); /* COPY is ignored even at explicit OK. */
 menu_parseEncoder(0,1);assert(menu_voiceCopyNotice==1&&transfer_calls==count+1);
 assert(mainParameters[1][PAR_COARSE3]==activeParameters[PAR_COARSE1]);
 for(unsigned n=0;n<5;n++){menu_parseEncoder(1,1);now++;menu_parseEncoder(0,0);now++;}
 assert(transfer_calls==count+1&&menu_voiceCopyDrain&&!editModeActive);
 now+=100;menu_parseEncoder(0,0);assert(menu_voiceCopyDrain);panel_release(BUT_COPY);
 now+=40;menu_parseEncoder(0,0);assert(!menu_voiceCopyDrain);
 assert(api_writes==writes&&sends==dispatch&&fx_preparations==fx);
 /* The unchanged public encoder still routes save/load editing to its own
  * handler. This boundary assertion does not emulate the name editor. */
 for(unsigned page=LOAD_PAGE;page<=SAVE_PAGE;page++){
  menu_activePage=page;unsigned calls=load_save_calls;
  menu_parseEncoder(1,0);assert(load_save_calls==calls+1&&load_save_inc==1&&!load_save_button);
  menu_parseEncoder(0,1);assert(load_save_button==1);menu_parseEncoder(0,0);
 }
 unsigned modes=mode_calls;panel_press(BUT_MODE2);panel_release(BUT_MODE2);
 assert(mode_calls==modes+1&&buttonHandler_getMode()==SELECT_MODE_PERF);
 puts("PASS TEST02 explicit OK commits selected Kit2/Drum3 once even with ignored COPY held; DATA hold/bounce cannot repeat, no FX/MIDI/SD transfer dispatch; unchanged generic save/load DATA routing");
}
static void slot_label_oracle(void)
{
 /* Literal independent oracle: never reuse the renderer or its arithmetic. */
 static const char labels[64][4]={
  "K00","K01","K02","K03","K04","K05","K06","K07",
  "K08","K09","K10","K11","K12","K13","K14","K15",
  "K16","K17","K18","K19","K20","K21","K22","K23",
  "K24","K25","K26","K27","K28","K29","K30","K31",
  "K32","K33","K34","K35","K36","K37","K38","K39",
  "K40","K41","K42","K43","K44","K45","K46","K47",
  "K48","K49","K50","K51","K52","K53","K54","K55",
  "K56","K57","K58","K59","K60","K61","K62","K63"
 };
 unsigned transfers=0,projects=0;
 for(unsigned source=0;source<7;source++)for(unsigned kit=0;kit<64;kit++){
  r22_reset(source);
  /* Pick a distinct source identity for target9, never shift the target. */
  if(kit==9)menu_loadedPresetNr[CURRENT_KIT]=8;
  activeParameters[PAR_MORPH]=0; /* Idle alias target0 must remain permitted. */
  /* Valid preset precondition: immediate load must not normalize zero IDs. */
  for(unsigned lfo=0;lfo<6;lfo++)mainParameters[kit][PAR_VOICE_LFO1+lfo]=lfo+1;
  NavigationSnapshot *s=navigation_snapshot();
  uint8_t expected[END_OF_SOUND_PARAMETERS];memcpy(expected,mainParameters[kit],sizeof expected);
  unsigned voice=source<6?source:5;
  if(source<3)menu_oracle(source,source,expected);
  else for(unsigned page=voice;page<=(source>=5?6:voice);page++)for(unsigned sub=0;sub<8;sub++){
   uint16_t ids[8];memcpy(ids,&menuPages[page][sub].bot1,sizeof ids);
   for(unsigned j=0;j<8;j++)if(!is_excluded(ids[j]))expected[ids[j]]=activeParameters[ids[j]];
  }
  chord();data_click();
  while(menu_voiceCopyTarget!=kit)menu_parseEncoder(menu_voiceCopyTarget<kit?1:-1,0);
  assert(menu_voiceCopyTarget==kit);
  char row[17];snprintf(row,sizeof row,source<3?"D%u > [K00] D%u OK":"V%u > [K00]   OK ",voice+1,voice+1);
  memcpy(row+6,labels[kit],3);navigation_row(row,0);
  data_click();
  snprintf(row,sizeof row,source<3?"D%u > K00 D%u  OK ":"V%u > K00     OK ",voice+1,voice+1);
  memcpy(row+5,labels[kit],3);navigation_row(row,VIS_CURS(1,5,1));
  while(menu_voiceCopyFocus!=VOICE_COPY_OK)menu_parseEncoder(1,0);
  snprintf(row,sizeof row,source<3?"D%u > K00 D%u >OK ":"V%u > K00    >OK ",voice+1,voice+1);
  memcpy(row+5,labels[kit],3);navigation_row(row,0);
  navigation_unchanged(s);data_click();assert(menu_voiceCopyNotice==1&&transfer_calls==s->calls+1);
  for(unsigned k=0;k<64;k++)assert(!memcmp(mainParameters[k],k==kit?expected:s->kits+k*sizeof expected,sizeof expected));
  assert(!memcmp(s->active,activeParameters,sizeof s->active)&&!memcmp(&s->patterns,&seq_patternSet,sizeof s->patterns));
  assert(!memcmp(s->engine,engine_values,sizeof s->engine)&&!memcmp(s->seen,engine_seen,sizeof s->seen));
  assert(s->dispatch==sends&&s->fx==fx_preparations&&s->writes==api_writes&&s->sectors==physical_writes);
  free(s);transfers++;
  if((source==0||source==4)&&(kit==0||kit==1||kit==9||kit==63)){
   /* Real project writer, filename formatter and FatFs. Construct the file
    * path from the literal oracle, not preset_makeFileNameSave(). */
   assert(preset_saveProject(0)==FR_OK);
   char path[]="/proj00/00-KITNA.snd";memcpy(path+8,labels[kit]+1,2);
   FIL file={0};UINT count;uint8_t data[8+END_OF_SOUND_PARAMETERS];
   uint8_t extension[FX_KIT_EXTENSION_BYTES];
   assert(f_open(&file,path,FA_READ)==FR_OK&&
          f_size(&file)==sizeof(data)+sizeof(extension));
   assert(f_read(&file,data,sizeof data,&count)==FR_OK&&count==sizeof data);
   assert(f_read(&file,extension,sizeof extension,&count)==FR_OK&&
          count==sizeof extension&&fxKit_validate(extension));
   assert(f_close(&file)==FR_OK);
   assert(!memcmp(data,"KITNAME!",8)&&!memcmp(data+8,expected,sizeof expected));
   memset(mainParameters[kit],0,sizeof expected);
   assert(preset_loadProject(0,0)==FR_OK&&!memcmp(mainParameters[kit],expected,sizeof expected));
   preset_loadDrumsetRam(kit,0,1);assert(menu_loadedPresetNr[CURRENT_KIT]==kit);
   assert(!memcmp(activeParameters,expected,sizeof expected));projects++;
   printf("PASS TEST03 source V%u label %s -> RAM index %u -> %s -> real project reload/kit load\n",source+1,labels[kit],kit,path);
  }
 }
 for(unsigned source=0;source<7;source++){
  r22_reset(source);NavigationSnapshot *s=navigation_snapshot();chord();
  assert(menu_voiceCopyTarget==9);menu_renderVoiceTransfer();assert(!memcmp(editDisplayBuffer[1]+5,"K09",3));
  confirm();assert(menu_voiceCopyNotice==2&&transfer_calls==s->calls+1);
  s->calls++; /* One rejected API attempt, no committed transaction. */
  navigation_unchanged(s);free(s);
 }
 assert(transfers==448&&projects==8);
 puts("PASS TEST03 independent literal K00-K63 oracle: 448 seven-source label/editor/OK and exact RAM-slot cases; eight real FatFs project/.snd roundtrips for 00/01/09/63, no shifted kit; seven same-kit K09 rejects without side effects");
}
static void navigation_suite(void)
{
 navigation_entry_gates();navigation_fields();navigation_mode_cancel();navigation_copy_and_instruments();navigation_ok_and_outside();
 slot_label_oracle();
}
